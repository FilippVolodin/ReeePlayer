#ifndef AUDIO_TOOLS_H
#define AUDIO_TOOLS_H

#include <QObject>
#include <QFutureWatcher>

class QString;

using Waveform = std::vector<uint8_t>;
using WaveformPtr = std::shared_ptr<Waveform>;

class VAD;
class VADData;
using VADPtr = std::shared_ptr<VAD>;

class AudioTools : public QObject
{
    Q_OBJECT
public:
    AudioTools(const QString& media_file);
    ~AudioTools();

    WaveformPtr get_waveform() const;
    VADPtr get_vad() const;

    void request();
signals:
    void waveform_is_ready(WaveformPtr);
    void vad_is_ready(VADPtr);
private:
    void create_wav_finished();

    QString m_media_file;

    bool m_vol_loaded = false;
    bool m_vad_loaded = false;
    QString m_wav_file;
    QString m_vol_file;
    std::unique_ptr<QFutureWatcher<bool>> m_wav_watcher;
    std::unique_ptr<QFutureWatcher<WaveformPtr>> m_waveform_watcher;
    std::unique_ptr<VADData> m_vad_data;
    WaveformPtr m_waveform;
    VADPtr m_vad;
};

#endif