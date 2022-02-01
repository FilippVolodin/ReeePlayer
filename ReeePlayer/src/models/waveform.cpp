#include "waveform.h"

Waveform::Waveform(const QString& filename)
{
    QFile vol_file(filename);
    if (!vol_file.open(QIODevice::ReadOnly))
        throw std::exception("Can't load vol-file");

    QDataStream in(&vol_file);
    int version;
    int32_t size;
    in >> version;
    in >> size;
    //std::vector<uint8_t> volumes;
    m_max_volume.clear();
    m_max_volume.reserve(size);

    const int window_size = 20;
    int sum = 0;
    int count = 0;

    for (int chunk = 0; chunk < size; chunk++)
    {
        int8_t v;
        in >> v;
        m_max_volume.push_back(v);
    }
}

const std::vector<uint8_t>& Waveform::get_max_volumes() const
{
    return m_max_volume;
}
