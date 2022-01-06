#include "waveform.h"
#include "models/jumpcutter.h"

constexpr int chunk_length_ms = 10;

Waveform::Waveform(QWidget* parent) : QWidget(parent)
{
}

void Waveform::set_jumpcutter(const JumpCutter* jc)
{
    m_jc = jc;
}

void Waveform::set_time(int time)
{
    m_time = time;
}

void Waveform::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    painter.fillRect(0, 0, width(), height(), Qt::white);

    if (m_jc == nullptr)
        return;

    const std::vector<uint8_t>& volumes = m_jc->get_max_volumes();
    const std::vector<bool>& intervals = m_jc->get_intervals();
    if (intervals.empty())
        return;

    int length = volumes.size() * chunk_length_ms;
    int time_window = std::min(length, 10000);
    int ch_time_window = time_window / chunk_length_ms;

    int a = m_time - (time_window / 2);
    int b = m_time + (time_window / 2);


    int ch_a = a / chunk_length_ms;
    int ch_b = b / chunk_length_ms;

    QPen pen(Qt::blue);
    painter.setPen(pen);
    //for (int ch = ch_a; ch < ch_b; ch++)
    //{
    //    int x0 = ((float)(ch - ch_a) / ch_time_window) * width();
    //    int y0 = height() * (1.0 - (float)volumes[ch] / 256);
    //    int y1 = height();
    //    painter.drawLine(x0, y0, x0, y1);
    //}
    
    int last_index = static_cast<int>(intervals.size() - 1);
    int interval_begin = std::min(last_index, std::max(ch_a, 0));

    bool cur_interval_value = intervals[interval_begin];

    for (int x0 = 0; x0 < width(); x0++)
    {
        int ch = (float)x0 / width() * ch_time_window + ch_a;
        if (ch < 0 || ch >= volumes.size())
            continue;

        if (intervals[ch] != cur_interval_value || x0 == width() - 1)
        {
            if (!cur_interval_value)
            {
                int zone_x0 = ((float)(interval_begin - ch_a) / ch_time_window) * width();
                int zone_x1 = ((float)(ch - ch_a) / ch_time_window) * width();
                painter.fillRect(zone_x0, 0, zone_x1 - zone_x0, height(), Qt::lightGray);
            }
            cur_interval_value = intervals[ch];
            interval_begin = ch;
        }
    }

    for (int x0 = 0; x0 < width(); x0++)
    {
        int ch = (float)x0 / width() * ch_time_window + ch_a;
        if (ch < 0 || ch >= volumes.size())
            continue;

        int y0 = height() * (1.0 - (float)volumes[ch] / 256);
        int y1 = height();
        painter.drawLine(x0, y0, x0, y1);
    }

    pen.setColor(Qt::red);
    painter.setPen(pen);

    float time_window_pos = (float)(m_time - a) / (b - a);
    int time_x = time_window_pos * width();
    painter.drawLine(time_x, 0, time_x, height());
}
