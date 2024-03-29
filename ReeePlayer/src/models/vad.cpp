#include "vad.h"

#include <QString>
#include <QTcpServer>
#include <QTcpSocket>
#include <QProcess>
#include <QtEndian>
#include <QFile>
#include <QFileInfo>

const char* STOP_CMD = "\1";
constexpr int32_t VAD_FILE_VERSION = 1;
constexpr int sampling_rate = 16000;
constexpr int window_size_samples = 1536;
constexpr int vad_chunk_length_ms = window_size_samples * 1000 / sampling_rate;

VAD::VAD(std::unique_ptr<VADData> vad_data)
    : m_vad_data(std::move(vad_data))
{
    connect(m_vad_data.get(), &VADData::progress_updated, [this](int, int) {process_data(); });
    connect(m_vad_data.get(), &VADData::progress_updated, this, &VAD::progress_updated);
    process_data();
}

int VAD::next_interval(int t) const
{
    return next_interval_in_chunks(t / vad_chunk_length_ms) * vad_chunk_length_ms;
}

int VAD::next_interval_in_chunks(int chunk) const
{
    if (chunk >= m_processed_vad_data.size())
        return m_processed_vad_data.size();

    bool cur_is_voice = chunk_is_voice(chunk);
    auto it = std::find(m_processed_vad_data.begin() + chunk, m_processed_vad_data.end(), !cur_is_voice);
    int chunks_diff = std::distance(m_processed_vad_data.begin(), it);
    return chunks_diff;
}

bool VAD::is_voice(int t) const
{
    return chunk_is_voice(t / vad_chunk_length_ms);
}

bool VAD::chunk_is_voice(int chunk) const
{
    // No data? Assume voice
    if (chunk >= m_processed_vad_data.size())
        return true;

    return m_processed_vad_data[chunk];
}

uint8_t VAD::chunk_prob(int ch) const
{
    if (ch < 0 || ch >= m_vad_data->get_data().size())
        return 0;
    return m_vad_data->get_data()[ch];
}

int VAD::num_chunks() const
{
    return m_vad_data->get_data().size();
}

int VAD::num_processed_chunks() const
{
    return m_processed_vad_data.size();
}

int VAD::rewind(int t, int delta) const
{
    int chunk = t / vad_chunk_length_ms;
    if (chunk >= m_processed_vad_data.size())
        chunk = m_processed_vad_data.size() - 1;

    int chunks_delta = std::abs(delta / vad_chunk_length_ms);
    int cd = 0;
    int sign = delta > 0 ? 1 : -1;
    int cur_chunk = chunk;
    for (int c = chunk; c >= 0 && c < m_processed_vad_data.size() && cd < chunks_delta; c += sign)
    {
        if (m_processed_vad_data[c])
            ++cd;
        cur_chunk = c;
    }
    return cur_chunk * vad_chunk_length_ms;
}

void VAD::apply_settings(std::shared_ptr<VADSettings> settings)
{
    m_settings = settings;
    reprocess_data();
}

void VAD::process_data()
{
    if (!m_settings)
        return;

    const int min_interval_in_chunks = m_settings->get_min_non_voice_interval() / vad_chunk_length_ms;
    const int margin_after_in_chunks = m_settings->get_margin_after() / vad_chunk_length_ms;
    const int margin_before_in_chunks = m_settings->get_margin_before() / vad_chunk_length_ms;

    const std::vector<uint8_t>& data = m_vad_data->get_data();

    int processed_size = m_processed_vad_data.size();
    if (processed_size == data.size())
        return;

    m_processed_vad_data.resize(data.size(), true);

    if (processed_size > data.size())
    {
        return;
    }

    int start_chunk = std::max(0, processed_size - min_interval_in_chunks);
    int last_voice_chunk = start_chunk - 1;
    for (int chunk = start_chunk; chunk < data.size(); chunk++)
    {
        if (data[chunk] >= m_settings->get_voice_prob() || chunk == data.size() - 1)
        {
            int silent_length_ch = (chunk - last_voice_chunk - 1);
            int total_len_ch = min_interval_in_chunks +
                margin_after_in_chunks +
                margin_before_in_chunks;

            if (silent_length_ch > total_len_ch)
            {
                std::fill(m_processed_vad_data.begin() + (last_voice_chunk + margin_after_in_chunks + 1),
                    m_processed_vad_data.begin() + chunk - margin_before_in_chunks,
                    false);
            }
            last_voice_chunk = chunk;
        }
    }
}

void VAD::reprocess_data()
{
    m_processed_vad_data.clear();
    process_data();
}

bool is_vad_ready(const QString& filename)
{
    QString vad_filename = get_vad_file(filename);
    if (QFileInfo(vad_filename).exists())
        return false;

    QFile vad_file(vad_filename);
    if (!vad_file.open(QIODevice::ReadOnly))
        return false;

    QDataStream in(&vad_file);

    int32_t version = 1;
    in >> version;
    if (in.status() != QDataStream::Status::Ok || version != VAD_FILE_VERSION)
        return false;

    int32_t chunks;
    int32_t total_chunks;

    in >> chunks;
    in >> total_chunks;

    if (in.status() != QDataStream::Status::Ok || chunks < total_chunks)
        return false;

    return true;
}

QString get_vad_file(const QString& filename)
{
    QFileInfo fi(filename);
    return fi.absolutePath() + "/" + fi.completeBaseName() + ".vad";
}

uint8_t VADSettings::get_voice_prob() const
{
    return m_voice_prob;
}

void VADSettings::set_voice_prob(uint8_t voice_prob)
{
    m_voice_prob = voice_prob;
}

int VADSettings::get_min_non_voice_interval() const
{
    return m_min_non_voice_interval;
}

void VADSettings::set_min_non_voice_interval(int min_non_voice_interval)
{
    m_min_non_voice_interval = min_non_voice_interval;
}

int VADSettings::get_margin_before() const
{
    return m_margin_before;
}

void VADSettings::set_margin_before(int margin_before)
{
    m_margin_before = margin_before;
}

int VADSettings::get_margin_after() const
{
    return m_margin_after;
}

void VADSettings::set_margin_after(int margin_after)
{
    m_margin_after = margin_after;
}

VADData::VADData(const QString& vad_file)
    : m_vad_file(vad_file)
{
}

void VADData::new_conn()
{
    qDebug("new_conn");
    if (m_sock)
        return;
    m_sock = m_server->nextPendingConnection();
    connect(m_sock, &QTcpSocket::readyRead, this, &VADData::ready_read);
}

VADData::~VADData()
{
    stop();
}

bool VADData::load_vad()
{
    reset();

    QFile file(m_vad_file);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    QDataStream in(&file);

    int32_t version = 1;
    in >> version;
    if (in.status() != QDataStream::Status::Ok || version != VAD_FILE_VERSION)
        return false;

    int32_t chunks;

    in >> chunks;
    in >> m_total_num_chunks;

    bool res = true;
    if (in.status() == QDataStream::Status::Ok)
    {
        m_vad_data.reserve(chunks);
        while (!in.atEnd())
        {
            uint8_t p;
            in >> p;
            m_vad_data.push_back(p);
        }
    }
    else
        res = false;

    res = res && is_ready();
    if (!res)
    {
        m_total_num_chunks = 0;
        m_vad_data.clear();
    }

    return res;
}

const std::vector<uint8_t>& VADData::get_data() const
{
    return m_vad_data;
}

bool VADData::is_ready() const
{
    return m_total_num_chunks >= 0 && m_vad_data.size() == m_total_num_chunks;
}

bool VADData::extract(const QString& wav_file)
{
    if (wav_file.isEmpty())
        return false;

    m_server = std::make_unique<QTcpServer>();

    if (!m_server->listen(QHostAddress::LocalHost))
        return false;

    connect(m_server.get(), &QTcpServer::newConnection, this, &VADData::new_conn);
    const int first_chunk = m_vad_data.size();

    QStringList args;
    args
        << wav_file
        << QString::number(m_server->serverPort())
        << QString::number(first_chunk);

    m_vad_process = std::make_unique<QProcess>();

    m_vad_process->start("pyutils/vad.exe", args);
    return m_vad_process->waitForStarted(10000);
}

void VADData::stop()
{
    if (m_sock)
    {
        m_sock->write(STOP_CMD);
        m_sock->waitForBytesWritten();
        m_server->close();
        m_sock = nullptr;
        if (m_new_data_arrived && is_ready())
        {
            save_data();
        }
    }
}

void VADData::ready_read()
{
    int old_bytes_received = m_bytes_received;
    QByteArray b = m_sock->readAll();

    m_bytes_received += b.size();
    qDebug("bytes received: %d", b.size());

    QString d;
    for (int i = 0; i < std::min(10, (int)b.size()); ++i)
    {
        d += QString("%1 ").arg((uchar)b[i], 2, 16, QChar('0'));
    }
    qDebug("Data: %s", qPrintable(d));

    if (old_bytes_received < 4)
    {
        m_buffer.append(b);
        if (m_bytes_received >= 4)
        {
            m_total_num_chunks = qFromBigEndian<int>(m_buffer.data());
            m_vad_data.reserve(m_total_num_chunks);
            for (int i = 4; i < m_buffer.size(); i++)
                m_vad_data.push_back(m_buffer[i]);

            if (m_bytes_received >= 5)
                m_new_data_arrived = true;

            m_buffer.clear();
        }
    }
    else
    {
        m_new_data_arrived = true;
        for (int i = 0; i < b.size(); i++)
            m_vad_data.push_back(b[i]);
    }

    emit progress_updated(m_vad_data.size(), m_total_num_chunks);

    if (is_ready())
    {
        stop();
    }
}

void VADData::save_data()
{
    QFile file(m_vad_file);
    if (!file.open(QIODevice::WriteOnly))
        return;

    QDataStream out(&file);
    out << VAD_FILE_VERSION;
    out << static_cast<uint32_t>(m_vad_data.size());
    out << static_cast<uint32_t>(m_total_num_chunks);
    for (uint8_t v : m_vad_data)
        out << v;
}

void VADData::reset()
{
    m_new_data_arrived = false;
    m_vad_data.clear();
    m_total_num_chunks = -1;
    m_bytes_received = 0;
}
