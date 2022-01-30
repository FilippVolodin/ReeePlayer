#ifndef VAD_H
#define VAD_H

class VAD : public QObject
{
    Q_OBJECT

public:
    VAD(const QString& vad_file);
    ~VAD();

    bool is_ready() const;

    void run(const QString& wav_file);
    void stop();

    int next_interval(int t) const;
    int next_interval_in_chunks(int ch) const;
    bool is_voice(int t) const;
    bool chunk_is_voice(int ch) const;
    int num_chunks() const;
    int rewind(int t, int delta) const;

signals:

    void progress_updated(int, int);

private slots:
    void new_conn();
    void ready_read();
private:
    void load_data();
    void save_data();
    void process_data();

    QString m_vad_file;
    std::unique_ptr<QTcpServer> m_server;
    QTcpSocket* m_sock = nullptr;
    QByteArray m_buffer;
    std::vector<uint8_t> m_vad_data;
    std::vector<bool> m_processed_vad_data;
    std::unique_ptr<QProcess> m_vad_process;
    int m_bytes_received = 0;
    int32_t m_total_num_chunks = -1;
    bool m_new_data_arrived = false;
    int m_last_voice_chunk = -1;
};

bool is_vad_ready(const QString& filename);
QString get_vad_file(const QString& filename);

#endif