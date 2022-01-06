#include "jumpcutter.h"
#include "wav.h"

constexpr int volume_threshold = 20;
constexpr int chunk_length_ms = 10;
constexpr int min_silent_length = 700;
constexpr int VOL_VERSION = 0;

JumpCutter::JumpCutter(const QString& filename)
{
    m_settings = std::make_shared<JumpCutterSettings>();
    read_volumes(filename);
    fragment();
}

bool JumpCutter::is_enabled() const
{
    return m_enabled;
}

void JumpCutter::set_enabled(bool enabled)
{
    m_enabled = enabled;
}

void JumpCutter::read_volumes(const QString& filename)
{
    QFile vol_file(filename);
    if (!vol_file.open(QIODevice::ReadOnly))
        throw std::exception("Can't load vol-file");

    QDataStream in(&vol_file);
    int version;
    int32_t size;
    in >> version;
    in >> size;
    std::vector<uint8_t> volumes;
    volumes.reserve(size);

    const int window_size = 20;
    int sum = 0;
    int count = 0;

    for (int chunk = 0; chunk < size; chunk++)
    {
        int8_t v;
        in >> v;
        volumes.push_back(v);
    }

    m_max_volume.clear();
    m_max_volume.reserve(size);

    for (int chunk = 0; chunk < size; chunk++)
    {
        if (count >= window_size)
            sum -= volumes[chunk - window_size];
        sum += volumes[chunk];
        int avg = sum / window_size;
        count++;

        m_max_volume.push_back(avg);
    }
}

void JumpCutter::fragment()
{
    m_fragments.resize(m_max_volume.size());
    std::fill(m_fragments.begin(), m_fragments.end(), true);
    int last_loud_chunk = 0;

    for (int chunk = 0; chunk < m_max_volume.size(); chunk++)
    {
        int volume = m_max_volume[chunk];
        if (volume >= m_settings->get_volume_threshold() * 256 || chunk == m_max_volume.size() - 1)
        {
            int silent_length_ch = (chunk - last_loud_chunk - 1);
            int silent_length = (chunk - last_loud_chunk - 1) * chunk_length_ms;
            int total_len = m_settings->get_min_silence_interval() +
                m_settings->get_margin_before() +
                m_settings->get_margin_after();

            if (silent_length > total_len)
            {
                int margin_before_ch = m_settings->get_margin_before() / chunk_length_ms;
                int margin_after_ch = m_settings->get_margin_after() / chunk_length_ms;

                std::fill(m_fragments.begin() + last_loud_chunk + margin_after_ch, m_fragments.begin() + chunk - 1 - margin_before_ch, false);
            }
            last_loud_chunk = chunk;
        }
    }
}

int JumpCutter::next_interval(int t) const
{
    int chunk = t / chunk_length_ms;
    if (chunk >= m_fragments.size())
        return m_fragments.size() * chunk_length_ms;

    auto it = std::find(m_fragments.begin() + chunk, m_fragments.end(), !m_fragments[chunk]);
    int chunks_diff = std::distance(m_fragments.begin(), it);
    return chunks_diff * chunk_length_ms;
}

int JumpCutter::rewind(int t0, int delta) const
{
    int chunk = t0 / chunk_length_ms;
    if (chunk >= m_fragments.size())
        chunk = m_fragments.size() - 1;

    int chunks_delta = std::abs(delta / chunk_length_ms);
    int cd = 0;
    int sign = delta > 0 ? 1 : -1;
    int cur_chunk = chunk;
    for (int c = chunk; c >= 0 && c < m_fragments.size() && cd < chunks_delta; c += sign)
    {
        if (m_fragments[c])
            ++cd;
        cur_chunk = c;
    }
    return cur_chunk * chunk_length_ms;
}

const std::vector<uint8_t>& JumpCutter::get_max_volumes() const
{
    return m_max_volume;
}

const std::vector<bool>& JumpCutter::get_intervals() const
{
    return m_fragments;
}

std::shared_ptr<JumpCutterSettings> JumpCutter::get_settings() const
{
    return m_settings;
}

void JumpCutter::apply_settings(std::shared_ptr<JumpCutterSettings> settings)
{
    m_settings = settings;
    fragment();
}

bool JumpCutter::current_interval_is_loud(int t) const
{
    int chunk = t / chunk_length_ms;
    if (chunk >= m_fragments.size())
        return false;
    return m_fragments[chunk];
}

QString create_wav(const QString& filename, std::function<void(QString)> log)
{
    log("Start wav-file generation");
    QString temp_wav = QDir::temp().absoluteFilePath("audio.wav");
    QStringList args;
    args << "-y" << "-i" << filename << "-ar" << "5512" << "-ac" << "1" << "-vn" << "-hide_banner" << temp_wav;

    QProcess ffmpeg;
    ffmpeg.start("ffmpeg", args);
    if (!ffmpeg.waitForStarted(-1))
        return QString();

    while (!ffmpeg.waitForFinished(500))
    {
        QString info = ffmpeg.readAllStandardError();
        info = info.trimmed();
        if (!info.isEmpty())
            log(info);
    }
    return temp_wav;
}

std::vector<uint8_t> read_wav(const QString& filename, std::function<void(QString)> log)
{
    log("Start reading wav-file");
    AudioFile<float> audioFile;
    if (!audioFile.load(filename.toStdString()))
        return std::vector<uint8_t>();

    log("Start waveform generation");

    int sample_rate = audioFile.getSampleRate();
    int bit_depth = audioFile.getBitDepth();

    int num_samples = audioFile.getNumSamplesPerChannel();
    double length = audioFile.getLengthInSeconds();

    int num_channels = audioFile.getNumChannels();
    bool isMono = audioFile.isMono();
    bool isStereo = audioFile.isStereo();

    const int num_chunks = static_cast<int>(length / chunk_length_ms * 1000);

    std::vector<uint8_t> max_volume;
    max_volume.reserve(num_chunks);
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
        max_volume.push_back(static_cast<uint8_t>(max_chunk_volume * 256));
    }

    log("Finish");

    return max_volume;
}

void save_volumes(const QString& filename, const std::vector<uint8_t>& max_volume)
{
    QFile vol_file(filename);
    if (!vol_file.open(QIODevice::WriteOnly))
        return;

    QDataStream out(&vol_file);
    out << VOL_VERSION;
    out << static_cast<uint32_t>(max_volume.size());
    for (int chunk = 0; chunk < max_volume.size(); chunk++)
    {
        uint8_t volume = max_volume[chunk];
        out << volume;
    }
}

bool is_vol_exist(const QString& filename)
{
    return QFileInfo(get_vol_file(filename)).exists();
}

QString get_vol_file(const QString& filename)
{
    QFileInfo fi(filename);
    return fi.absolutePath() + "/" + fi.completeBaseName() + ".vol";
}

float JumpCutterSettings::get_volume_threshold() const
{
    return m_volume_threshold;
}

void JumpCutterSettings::set_volume_threshold(float volume_threshold)
{
    m_volume_threshold = volume_threshold;
}

float JumpCutterSettings::get_silence_speed() const
{
    return m_silence_speed;
}

void JumpCutterSettings::set_silence_speed(float silence_speed)
{
    m_silence_speed = silence_speed;
}

bool JumpCutterSettings::is_silence_skipping() const
{
    return std::abs(m_silence_speed) < 0.1;
}

int JumpCutterSettings::get_min_silence_interval() const
{
    return m_min_silence_interval;
}

void JumpCutterSettings::set_min_silence_interval(int min_silence_interval)
{
    m_min_silence_interval = min_silence_interval;
}

int JumpCutterSettings::get_margin_before() const
{
    return m_margin_before;
}

void JumpCutterSettings::set_margin_before(int margin_before)
{
    m_margin_before = margin_before;
}

int JumpCutterSettings::get_margin_after() const
{
    return m_margin_after;
}

void JumpCutterSettings::set_margin_after(int margin_after)
{
    m_margin_after = margin_after;
}
