#ifndef VAD_H
#define VAD_H

#include <QObject>

class QString;
class QTcpServer;
class QTcpSocket;
class QProcess;

class VADSettings
{
public:
    uint8_t get_voice_prob() const;
    void set_voice_prob(uint8_t);

    int get_min_non_voice_interval() const;
    void set_min_non_voice_interval(int);

    int get_margin_before() const;
    void set_margin_before(int);

    int get_margin_after() const;
    void set_margin_after(int);
private:
    uint8_t m_voice_prob = 128;
    int m_min_non_voice_interval = 300;
    int m_margin_before = 100;
    int m_margin_after = 100;
};

class VADData : public QObject
{
    Q_OBJECT

public:

    VADData(const QString& vad_file);
    ~VADData();

    bool load_vad();

    const std::vector<uint8_t>& get_data() const;

    bool is_ready() const;

    bool extract(const QString& wav_file);
    void stop();

signals:

    void progress_updated(int, int);

private slots:
    void new_conn();
    void ready_read();
private:
    void save_data();
    void reset();

    QString m_vad_file;
    std::unique_ptr<QTcpServer> m_server;
    QTcpSocket* m_sock = nullptr;
    QByteArray m_buffer;
    std::unique_ptr<QProcess> m_vad_process;

    std::vector<uint8_t> m_vad_data;
    int m_bytes_received = 0;
    int32_t m_total_num_chunks = -1;
    bool m_new_data_arrived = false;
};

class VAD : public QObject
{
    Q_OBJECT

public:
    VAD(std::unique_ptr<VADData> vad_data);

    int next_interval(int t) const;
    int next_interval_in_chunks(int ch) const;
    bool is_voice(int t) const;
    bool chunk_is_voice(int ch) const;
    uint8_t chunk_prob(int ch) const;
    int num_chunks() const;
    int num_processed_chunks() const;
    int rewind(int t, int delta) const;

    void apply_settings(std::shared_ptr<VADSettings>);
signals:

    void progress_updated(int, int);

private:
    void process_data();
    void reprocess_data();

    std::unique_ptr<VADData> m_vad_data;
    std::shared_ptr<VADSettings> m_settings;
    std::vector<bool> m_processed_vad_data;
};

bool is_vad_ready(const QString& filename);
QString get_vad_file(const QString& filename);

#endif