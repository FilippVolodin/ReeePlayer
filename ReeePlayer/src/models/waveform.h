#ifndef WAVEFORM_H
#define WAVEFORM_H

class Waveform
{
public:
    Waveform(const QString& filename);

    const std::vector<uint8_t>& get_max_volumes() const;
private:
    std::vector<uint8_t> m_max_volume;
};

#endif