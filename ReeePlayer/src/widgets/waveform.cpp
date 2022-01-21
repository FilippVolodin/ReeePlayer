#include "waveform.h"
#include "models/jumpcutter.h"

constexpr int chunk_length_ms = 10;
constexpr int sampling_rate = 16000;
constexpr int window_size_samples = 1536;
constexpr int vad_chunk_length_ms = window_size_samples * 1000 / sampling_rate;

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

   // painter.fillRect(0, 0, width(), height(), /*QColor(0xD6, 0xD6, 0xD6)*/ palette().color(QWidget::backgroundRole()));

    if (m_jc == nullptr)
        return;

    const std::vector<uint8_t>& volumes = m_jc->get_max_volumes();
    //const std::vector<bool>& intervals = m_jc->get_intervals();
    const std::vector<uint8_t>& voice_probs = m_jc->get_voice_probs();
    if (voice_probs.empty())
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

    int vad_ch_time_window = time_window / vad_chunk_length_ms;
    int vad_ch_a = m_a / vad_chunk_length_ms;
    double vad_ch_a_dbl = (double)m_a / vad_chunk_length_ms;
    int vad_ch_b = m_b / vad_chunk_length_ms;

    if (m_clip_mode)
    {
        int clip_a_x = ((float)(m_clip_a - m_a) / time_window) * width();
        int clip_b_x = ((float)(m_clip_b - m_a) / time_window) * width();
        painter.fillRect(clip_a_x, 0, clip_b_x - clip_a_x, height(), QColor(255, 201, 14));
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
    
    int vad_last_index = static_cast<int>(voice_probs.size() - 1);
    //int vad_interval_begin = std::min(vad_last_index, std::max(vad_ch_a, 0.f));
    //bool is_voice = voice_probs[vad_interval_begin] >= 128;

    if (!m_clip_mode)
    {
        int ch = vad_ch_a;
        if (ch < 0)
            ch = 0;

        while (ch < vad_ch_b && ch < voice_probs.size())
        {
            bool cur_is_voice = voice_probs[ch] >= 128;
            int ch_next = m_jc->next_interval_in_chunks(ch);
            if (!cur_is_voice)
            {
                int zone_x0 = ((ch - vad_ch_a_dbl) / vad_ch_time_window) * width();
                int zone_x1 = ((ch_next - vad_ch_a_dbl) / vad_ch_time_window) * width();
                painter.fillRect(zone_x0, 0, zone_x1 - zone_x0, height(), Qt::lightGray);
            }
            ch = ch_next;
        }

        //for (int x0 = 0; x0 < width(); x0++)
        //{
        //    int ch = (float)x0 / width() * vad_ch_time_window + vad_ch_a;
        //    if (ch < 0 || ch >= voice_probs.size())
        //        continue;

        //    bool cur_is_voice = voice_probs[ch] >= 128;
        //    if (cur_is_voice != is_voice || x0 == width() - 1)
        //    {
        //        if (!is_voice)
        //        {
        //            int zone_x0 = ((float)(vad_interval_begin - vad_ch_a) / vad_ch_time_window) * width();
        //            int zone_x1 = ((float)(ch - vad_ch_a) / vad_ch_time_window) * width();
        //            painter.fillRect(zone_x0, 0, zone_x1 - zone_x0, height(), Qt::lightGray);
        //        }
        //        is_voice = cur_is_voice;
        //        vad_interval_begin = ch;
        //    }
        //}
    }

    pen.setColor(Qt::gray);
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
