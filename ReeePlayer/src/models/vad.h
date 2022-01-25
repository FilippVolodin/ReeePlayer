#ifndef VAD_H
#define VAD_H

class VAD : public QObject
{
    Q_OBJECT

public:
    VAD(const QString&, const QString&);
    ~VAD();

    int next_interval(int t) const;
    int next_interval_in_chunks(int ch) const;
    bool is_voice(int t) const;
    bool chunk_is_voice(int ch) const;
    int num_chunks() const;

private slots:
    void new_conn();
    void ready_read();
private:
    void save_data();

    QString m_vad_file;
    std::unique_ptr<QTcpServer> m_server;
    QTcpSocket* m_sock = nullptr;
    QByteArray m_buffer;
    std::vector<uint8_t> m_vad_data;
    std::unique_ptr<QProcess> m_vad_process;
    int m_bytes_received = 0;
    int m_num_chunks = -1;
    bool m_new_data_arrived = false;
};

bool is_vad_exist(const QString& filename);
QString get_vad_file(const QString& filename);

#endif