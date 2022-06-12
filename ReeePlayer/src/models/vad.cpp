#include "vad.h"

const char* STOP_CMD = "\1";
constexpr int32_t VAD_FILE_VERSION = 1;
constexpr int sampling_rate = 16000;
constexpr int window_size_samples = 1536;
constexpr int vad_chunk_length_ms = window_size_samples * 1000 / sampling_rate;

VAD::VAD(const QString& vad_file)
{
    m_vad_file = vad_file;
    load_data();
}

void VAD::new_conn()
{
    qDebug("new_conn");
    if (m_sock != nullptr)
        return;
    m_sock = m_server->nextPendingConnection();
    connect(m_sock, &QTcpSocket::readyRead, this, &VAD::ready_read);
}

VAD::~VAD()
{
    stop();
}

bool VAD::is_ready() const
{
    return m_total_num_chunks >= 0 && m_vad_data.size() == m_total_num_chunks;
}

bool VAD::run(const QString& wav_filename)
{
    if (wav_filename.isEmpty())
        return false;

    m_server = std::make_unique<QTcpServer>();

    if (!m_server->listen(QHostAddress::LocalHost))
        return false;

    connect(m_server.get(), &QTcpServer::newConnection, this, &VAD::new_conn);
    const int first_chunk = m_vad_data.size();

    QStringList args;
    args
        << wav_filename
        << QString::number(m_server->serverPort())
        << QString::number(first_chunk);

    m_vad_process = std::make_unique<QProcess>();

    m_vad_process->start("pyutils/vad.exe", args);
    return m_vad_process->waitForStarted(10000);
}

void VAD::stop()
{
    if (m_sock)
    {
        m_sock->write(STOP_CMD);
        m_sock->waitForBytesWritten();
        m_server->close();
        m_sock = nullptr;
        if (m_new_data_arrived)
        {
            save_data();
        }
    }
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
    return chunk_is_voice(t / vad_chunk_length_ms) * vad_chunk_length_ms;
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
    if (ch < 0 || ch >= m_vad_data.size())
        return 0;
    return m_vad_data[ch];
}

int VAD::num_chunks() const
{
    return m_vad_data.size();
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

void VAD::ready_read()
{
    int old_bytes_received = m_bytes_received;
    QByteArray b = m_sock->readAll();
    m_bytes_received += b.size();
    qDebug("bytes received: %d", b.size());

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

    if (m_settings)
        process_data();

    emit progress_updated(m_vad_data.size(), m_total_num_chunks);

    if (is_ready())
    {
        stop();
    }
}

void VAD::load_data()
{
    bool res;
    QFile vad_file(m_vad_file);
    if (!vad_file.open(QIODevice::ReadOnly))
        return;

    QDataStream in(&vad_file);

    int32_t version = 1;
    in >> version;
    if (in.status() != QDataStream::Status::Ok || version != VAD_FILE_VERSION)
        return;

    int32_t chunks;

    in >> chunks;
    in >> m_total_num_chunks;

    if (in.status() != QDataStream::Status::Ok)
        return;

    m_vad_data.clear();
    m_vad_data.reserve(chunks);
    while (!in.atEnd())
    {
        uint8_t p;
        in >> p;
        m_vad_data.push_back(p);
    }
}

void VAD::save_data()
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

void VAD::process_data()
{
    const int min_interval_in_chunks = m_settings->get_min_non_voice_interval() / vad_chunk_length_ms;
    const int margin_after_in_chunks = m_settings->get_margin_after() / vad_chunk_length_ms;
    const int margin_before_in_chunks = m_settings->get_margin_before() / vad_chunk_length_ms;

    int processed_size = m_processed_vad_data.size();
    if (processed_size == m_vad_data.size())
        return;

    m_processed_vad_data.resize(m_vad_data.size(), true);

    if (processed_size > m_vad_data.size())
    {
        return;
    }

    // TODO Magic number
    int start_chunk = std::max(0, processed_size - min_interval_in_chunks);
    int last_voice_chunk = start_chunk - 1;
    for (int chunk = start_chunk; chunk < m_vad_data.size(); chunk++)
    {
        //if (count >= window_size)
        //    sum -= m_max_volume[chunk - window_size];
        //sum += m_max_volume[chunk];
        //int volume = sum / window_size;
        //count++;

        if (m_vad_data[chunk] >= m_settings->get_voice_prob() || chunk == m_vad_data.size() - 1)
        {
            int silent_length_ch = (chunk - last_voice_chunk - 1);
            //int silent_length = (chunk - last_voice_chunk - 1) * chunk_length_ms;
            int total_len_ch = min_interval_in_chunks +
                margin_after_in_chunks +
                margin_before_in_chunks;

            if (silent_length_ch > total_len_ch)
            {
                //int margin_before_ch = m_settings->get_margin_before() / chunk_length_ms;
                //int margin_after_ch = m_settings->get_margin_after() / chunk_length_ms;

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
