#include <audio_tools.h>

#include <QString>
#include <QFileInfo>
#include <QDir>
#include <QStandardPaths>
#include <QtConcurrent>
#include <iostream>
#include <wav.h>
#include <vad.h>

constexpr int chunk_length_ms = 10;
constexpr int VOL_VERSION = 0;

static QString fetch_cache_dir()
{
    QStringList locs =
        QStandardPaths::standardLocations(QStandardPaths::AppLocalDataLocation);
    QString appdata = locs[1];
    QDir dir(appdata);
    QString cache_dir = "cache/audio";
    dir.mkpath(cache_dir);
    dir.cd(cache_dir);
    return dir.absolutePath();
}

static QString filepath_hash(const QString& mediafile)
{
    QFileInfo mf_info(mediafile);
    std::filesystem::path fs_path = mf_info.filesystemAbsoluteFilePath();
    size_t file_hash = std::filesystem::hash_value(fs_path);
    return QString("%1").arg(file_hash, 16, 16, QChar('0'));
}

static bool create_wav(const QString& media_file, const QString& wav_file)
{
    QStringList args;
    args << "-y"
        << "-i"
        << media_file
        << "-ar" << "16000"
        << "-ac" << "1"
        << "-vn"
        << "-hide_banner"
        << wav_file;

    QProcess ffmpeg;

    ffmpeg.start("ffmpeg/ffmpeg", args);
    if (!ffmpeg.waitForStarted(-1))
        throw std::exception();

    if (!ffmpeg.waitForFinished(-1))
        throw std::exception();

    if (ffmpeg.exitCode() != 0)
        throw std::exception();

    return true;
}

static WaveformPtr create_waveform(const QString& wavfile)
{
    AudioFile<float> audioFile;
    if (!audioFile.load(wavfile.toLocal8Bit().data()))
        throw std::exception();

    int sample_rate = audioFile.getSampleRate();
    double length = audioFile.getLengthInSeconds();
    int num_channels = audioFile.getNumChannels();

    const int num_chunks = static_cast<int>(length / chunk_length_ms * 1000);

    WaveformPtr waveform = std::make_shared<Waveform>();
    waveform->reserve(num_chunks);
    for (int64_t ms = 0; ms < length * 1000 - chunk_length_ms; ms += chunk_length_ms)
    {
        int sample_index_begin = ms * sample_rate / 1000;
        int sample_index_end = (ms + chunk_length_ms) * sample_rate / 1000;
        double max_chunk_volume = 0;
        for (int si = sample_index_begin; si < sample_index_end; si++)
        {
            for (int ch = 0; ch < num_channels; ch++)
            {
                double sample = audioFile.samples[ch][si];
                if (sample > max_chunk_volume)
                    max_chunk_volume = sample;
            }
        }
        waveform->push_back(static_cast<uint8_t>(max_chunk_volume * 256));
    }
    return waveform;
}

WaveformPtr load_waveform(const QString& filename)
{
    QFile vol_file(filename);
    if (!vol_file.open(QIODevice::ReadOnly))
        return nullptr;

    WaveformPtr waveform = std::make_shared<Waveform>();

    QDataStream in(&vol_file);
    int version;
    int32_t size;
    in >> version;
    in >> size;
    waveform->reserve(size);

    for (int chunk = 0; chunk < size; chunk++)
    {
        int8_t v;
        in >> v;
        waveform->push_back(v);
    }
    return waveform;
}

void save_waveform(const QString& filename, const Waveform& waveform)
{
    QFile vol_file(filename);
    if (!vol_file.open(QIODevice::WriteOnly))
        return;

    QDataStream out(&vol_file);
    out << VOL_VERSION;
    out << static_cast<uint32_t>(waveform.size());
    for (int chunk = 0; chunk < ssize(waveform); ++chunk)
    {
        uint8_t volume = waveform[chunk];
        out << volume;
    }
}

AudioTools::AudioTools(const QString& media_file)
    : m_media_file(media_file)
{
}

AudioTools::~AudioTools()
{
    if (!m_wav_file.isEmpty())
        QFile::remove(m_wav_file);
}

WaveformPtr AudioTools::get_waveform() const
{
    return m_waveform;
}

VADPtr AudioTools::get_vad() const
{
    return m_vad;
}

void AudioTools::request()
{
    QString hash = filepath_hash(m_media_file);

    QString cache_dir = fetch_cache_dir();

    QFileInfo vol_fileinfo(cache_dir, hash + ".vol");
    m_vol_file = vol_fileinfo.absoluteFilePath();
    WaveformPtr waveform = load_waveform(m_vol_file);
    m_vol_loaded = waveform != nullptr;

    QFileInfo vad_fileinfo(cache_dir, hash + ".vad");
    QString vad_file = vad_fileinfo.absoluteFilePath();
    m_vad_data = std::make_unique<VADData>(vad_file);
    m_vad_loaded = m_vad_data->load_vad();

    bool ready = m_vol_loaded && m_vad_loaded;

    if (m_vol_loaded)
        emit waveform_is_ready(waveform);

    if (m_vad_loaded)
    {
        m_vad = std::make_shared<VAD>(std::move(m_vad_data));
        emit vad_is_ready(m_vad);
    }

    if (!m_vol_loaded || !m_vad_loaded)
    {
        QFileInfo wav_fileinfo(QDir::temp(), hash + ".wav");
        m_wav_file = wav_fileinfo.absoluteFilePath();

        m_wav_watcher = std::make_unique<QFutureWatcher<bool>>();
        connect(m_wav_watcher.get(), &QFutureWatcher<bool>::finished,
            this, &AudioTools::create_wav_finished);
        auto wav_future = QtConcurrent::run(create_wav, m_media_file, m_wav_file);
        m_wav_watcher->setFuture(wav_future);
    }
}

void AudioTools::create_wav_finished()
{
    try
    {
        m_wav_watcher->result();
    }
    catch (const std::exception&)
    {
        return;
    }

    if (!m_vol_loaded)
    {
        m_waveform_watcher = std::make_unique<QFutureWatcher<WaveformPtr>>();
        connect(m_waveform_watcher.get(), &QFutureWatcher<WaveformPtr>::finished,
            [&]()
            {
                WaveformPtr waveform = m_waveform_watcher->result();
                save_waveform(m_vol_file, *waveform);
                emit waveform_is_ready(waveform);
            });

        auto waveform_future = QtConcurrent::run(create_waveform, m_wav_file)
            .onFailed([](const std::exception&) {return nullptr; });
        m_waveform_watcher->setFuture(waveform_future);
    }

    if (!m_vad_loaded)
    {
        m_vad_data->extract(m_wav_file);
        m_vad = std::make_shared<VAD>(std::move(m_vad_data));
        emit vad_is_ready(m_vad);
    }
}