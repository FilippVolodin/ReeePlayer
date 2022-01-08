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

bool Waveform::is_clip_mode(bool) const
{
    return m_clip_mode;
}

void Waveform::set_clip_mode(bool clip_mode)
{
    m_clip_mode = clip_mode;
}

int Waveform::get_clip_a() const
{
    return m_clip_a;
}

void Waveform::set_clip_a(int a)
{
    m_clip_a = a;
}

int Waveform::get_clip_b() const
{
    return m_clip_b;
}

void Waveform::set_clip_b(int b)
{
    m_clip_b = b;
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

    int time_window;

    if (m_clip_mode)
    {
        int clip_len = m_clip_b - m_clip_a;
        m_a = m_clip_a - clip_len / 4 - 1000;
        m_b = m_clip_b + clip_len / 4 + 1000;
        time_window = m_b - m_a;
    }
    else
    {
        time_window = std::min(length, 10000);
        m_a = m_time - (time_window / 2);
        m_b = m_time + (time_window / 2);
    }

    int ch_time_window = time_window / chunk_length_ms;
    int ch_a = m_a / chunk_length_ms;
    int ch_b = m_b / chunk_length_ms;

    if (m_clip_mode)
    {
        int clip_a_x = ((float)(m_clip_a - m_a) / time_window) * width();
        int clip_b_x = ((float)(m_clip_b - m_a) / time_window) * width();
        painter.fillRect(clip_a_x, 0, clip_b_x - clip_a_x, height(), Qt::green);
    }

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

    if (!m_clip_mode)
    {
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
    }

    pen.setColor(Qt::lightGray);
    painter.setPen(pen);

    int a_sec = std::min(length / 1000, std::max(0, m_a / 1000));
    int b_sec = std::min(length / 1000, std::max(0, m_b / 1000));

    for (int sec = a_sec; sec < b_sec; sec++)
    {
        float time_window_pos = (float)(sec * 1000 - m_a) / (m_b - m_a);
        int time_x = time_window_pos * width();
        painter.drawLine(time_x, 0, time_x, height());
    }

    pen.setColor(Qt::blue);
    painter.setPen(pen);

    for (int x0 = 0; x0 < width(); x0++)
    {
        int ch = (float)x0 / width() * ch_time_window + ch_a;
        if (ch < 0 || ch >= volumes.size())
            continue;

        int y0 = height() * (1.0 - (float)volumes[ch] / 256);
        int y1 = height();
        painter.drawLine(x0, y0, x0, y1);
    }

    //pen.setColor(Qt::red);
    //painter.setPen(pen);

    float time_window_pos = (float)(m_time - m_a) / (m_b - m_a);
    int time_x = time_window_pos * width();
    painter.fillRect(time_x - 1, 0, 3, height(), Qt::red);
    //painter.drawLine(time_x, 0, time_x, height());
}

void Waveform::mouseReleaseEvent(QMouseEvent* event)
{
    int x = event->pos().x();
    int time = (float)x / width() * (m_b - m_a) + m_a;
    emit mouse_release(time, event);
}

void Waveform::wheelEvent(QWheelEvent* event)
{
    int x = event->position().x();
    int time = (float)x / width() * (m_b - m_a) + m_a;
    emit wheel_event(time, event);
}
