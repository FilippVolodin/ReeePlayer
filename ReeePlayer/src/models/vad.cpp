#include "vad.h"

const char* STOP_CMD = "1";
constexpr int32_t VAD_FILE_VERSION = 1;
constexpr int sampling_rate = 16000;
constexpr int window_size_samples = 1536;
constexpr int vad_chunk_length_ms = window_size_samples * 1000 / sampling_rate;

VAD::VAD(const QString& wav_filename, const QString& vad_file)
{
    m_vad_file = vad_file;
    m_server = std::make_unique<QTcpServer>();

    if (!m_server->listen(QHostAddress::LocalHost, 12345))
        return;

    connect(m_server.get(), &QTcpServer::newConnection, this, &VAD::new_conn);

    QStringList args;
    args
        << wav_filename
        << QString::number(m_server->serverPort());

    m_vad_process = std::make_unique<QProcess>();

    QObject::connect(m_vad_process.get(), &QProcess::errorOccurred,
        [this]()
        {
            int t = 0;
        }
    );

    QObject::connect(m_vad_process.get(), &QProcess::readyReadStandardError,
        [this]()
        {
            QString msg = m_vad_process->readAllStandardError();
            int temp = 0;
        }
    );

    m_vad_process->start("E:\\dev\\VAD_ONNX\\dist\\vad\\vad.exe", args);
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
    if (m_sock)
    {
        m_sock->write(STOP_CMD);
    }

    if (m_new_data_arrived)
    {
        //save_data();
    }
}

int VAD::next_interval(int t) const
{
    return next_interval_in_chunks(t / vad_chunk_length_ms) * vad_chunk_length_ms;
}

int VAD::next_interval_in_chunks(int chunk) const
{
    if (chunk >= m_vad_data.size())
        return m_vad_data.size();

    bool cur_is_voice = chunk_is_voice(chunk);
    auto interval_changed = [cur_is_voice](uint8_t p)
    {
        return (p >= 128) != cur_is_voice;
    };

    auto it = std::find_if(m_vad_data.begin() + chunk, m_vad_data.end(), interval_changed);
    int chunks_diff = std::distance(m_vad_data.begin(), it);
    return chunks_diff;
}

bool VAD::is_voice(int t) const
{
    return chunk_is_voice(t / vad_chunk_length_ms) * vad_chunk_length_ms;
}

bool VAD::chunk_is_voice(int chunk) const
{
    // No data? Assume voice
    if (chunk >= m_vad_data.size())
        return true;

    return m_vad_data[chunk] >= 128;
}

int VAD::num_chunks() const
{
    return m_vad_data.size();
}

void VAD::ready_read()
{
    QByteArray b = m_sock->readAll();
    m_bytes_received += b.size();
    qDebug("bytes received: %d", b.size());

    if (m_num_chunks == -1)
    {
        m_buffer.append(b);
        if (m_bytes_received >= 4)
        {
            m_num_chunks = qFromBigEndian<int>(m_buffer.data());
            m_vad_data.reserve(m_num_chunks);
            for (int i = 4; i < m_buffer.size(); i++)
                m_vad_data.push_back(m_buffer[i]);

            if (m_bytes_received >= 5)
                m_new_data_arrived = true;

            m_buffer.clear();
        }
    }
    else
    {
        for (int i = 0; i < b.size(); i++)
            m_vad_data.push_back(b[i]);
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
    out << static_cast<uint32_t>(m_num_chunks);
    for (uint8_t v : m_vad_data)
        out << v;
}

bool is_vad_exist(const QString& filename)
{
    return QFileInfo(get_vad_file(filename)).exists();
}

QString get_vad_file(const QString& filename)
{
    QFileInfo fi(filename);
    return fi.absolutePath() + "/" + fi.completeBaseName() + ".vad";
}