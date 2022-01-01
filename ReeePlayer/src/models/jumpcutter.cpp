#include "jumpcutter.h"
#include "wav.h"

std::vector<uint8_t> read_wav()
{
    AudioFile<float> audioFile;
    if (!audioFile.load("e:\\dev\\TestAudio\\alexa.wav"))
        return std::vector<uint8_t>();
    
    int sample_rate = audioFile.getSampleRate();
    int bit_depth = audioFile.getBitDepth();

    int num_samples = audioFile.getNumSamplesPerChannel();
    double length = audioFile.getLengthInSeconds();

    int num_channels = audioFile.getNumChannels();
    bool isMono = audioFile.isMono();
    bool isStereo = audioFile.isStereo();

    std::vector<uint8_t> max_volume;
    constexpr int chunk_length_ms = 10;
    const int num_chunks = static_cast<int>(length / chunk_length_ms * 1000);

    max_volume.reserve(length * 1000 / 10);
    for (int64_t ms = 0; ms < length * 1000 - 10; ms += 10)
    {
        int sample_index_begin = ms * sample_rate / 1000;
        int sample_index_end = (ms + 10) * sample_rate / 1000;
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

    return max_volume;
}
