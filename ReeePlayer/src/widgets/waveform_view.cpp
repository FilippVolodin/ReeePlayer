#include "waveform_view.h"
#include "models/waveform.h"
#include "models/vad.h"

constexpr int chunk_length_ms = 10;
constexpr int sampling_rate = 16000;
constexpr int window_size_samples = 1536;
constexpr int vad_chunk_length_ms = window_size_samples * 1000 / sampling_rate;

constexpr QColor WAVEFORM_COLOR(0x08, 0x6F, 0xA1);
constexpr QColor VAD_COLOR(153, 217, 234);
constexpr QColor CLIP_COLOR(255, 201, 14, 128);


WaveformView::WaveformView(QWidget* parent) : QWidget(parent)
{
}

void WaveformView::set_waveform(const Waveform* waveform)
{
    m_waveform = waveform;
}

void WaveformView::set_vad(const VAD* vad)
{
    m_vad = vad;
}

void WaveformView::set_time(int time)
{
    m_time = time;
}

bool WaveformView::is_clip_mode(bool) const
{
    return m_clip_mode;
}

void WaveformView::set_clip_mode(bool clip_mode)
{
    m_clip_mode = clip_mode;
}

int WaveformView::get_clip_a() const
{
    return m_clip_a;
}

void WaveformView::set_clip_a(int a)
{
    m_clip_a = a;
}

int WaveformView::get_clip_b() const
{
    return m_clip_b;
}

void WaveformView::set_clip_b(int b)
{
    m_clip_b = b;
}

void WaveformView::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    if (m_waveform == nullptr)
        return;

    const std::vector<uint8_t>& volumes = m_waveform->get_max_volumes();

    int length = volumes.size() * chunk_length_ms;

    double time_window;

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

    double ch_time_window = time_window / chunk_length_ms;
    double ch_a = m_a / chunk_length_ms;
    double ch_b = m_b / chunk_length_ms;

    double vad_ch_time_window = (double)time_window / vad_chunk_length_ms;
    int vad_ch_a = m_a / vad_chunk_length_ms;
    double vad_ch_a_dbl = (double)m_a / vad_chunk_length_ms;
    int vad_ch_b = m_b / vad_chunk_length_ms;

    QPen pen = painter.pen();

    int global_x = ((double)m_a / time_window) * width();
    global_x -= global_x % 3;

    if (m_vad)
    {
        int ch = vad_ch_a;
        if (ch < 0)
            ch = 0;

        while (ch < vad_ch_b && ch < m_vad->num_chunks())
        {
            bool cur_is_voice = m_vad->chunk_is_voice(ch);
            int ch_next = m_vad->next_interval_in_chunks(ch);
            if (!cur_is_voice)
            {
                int zone_x0 = ((double)ch / vad_ch_time_window) * width() - global_x;
                int zone_x1 = ((double)ch_next / vad_ch_time_window) * width() - global_x;
                painter.fillRect(zone_x0, 0, zone_x1 - zone_x0, height(), Qt::lightGray);
            }
            ch = ch_next;
        }

        pen.setColor(Qt::red);
        painter.setPen(pen);

        QBrush brush = painter.brush();

        int prev_y = height();
        for (int ch = vad_ch_a; ch <= vad_ch_b; ch++)
        {
            int x0 = ((double)ch / vad_ch_time_window) * width() - global_x;
            int x1 = ((double)(ch + 1) / vad_ch_time_window) * width() - global_x;
            int y = height() * (1.0 - (float)m_vad->chunk_prob(ch) / 256);
            painter.fillRect(x0, y, x1 - x0, height(), VAD_COLOR);
            prev_y = y;
        }
    }

    pen.setColor(Qt::blue);
    painter.setPen(pen);

    if (m_clip_mode)
    {
        QBrush brush = painter.brush();
        brush.setStyle(Qt::SolidPattern);
        brush.setColor(CLIP_COLOR);
        painter.setBrush(brush);

        int clip_a_x = ((float)(m_clip_a - m_a) / time_window) * width();
        int clip_b_x = ((float)(m_clip_b - m_a) / time_window) * width();
        painter.fillRect(clip_a_x, 0, clip_b_x - clip_a_x, height(), brush);
    }

    pen.setColor(WAVEFORM_COLOR);
    painter.setPen(pen);

    for (int x0 = 0; x0 != width(); x0++)
    {
        if (x0 % 3 == 0)
            continue;

        int x = global_x + x0;
        int ch = (double)x / width() * ch_time_window;
        if (ch < 0 || ch >= volumes.size())
            continue;

        int y0 = height() * (1.0 - (float)volumes[ch] / 256);
        int y1 = height();
        painter.drawLine(x0, y0, x0, y1);
    }

    pen.setColor(Qt::gray);
    painter.setPen(pen);

    int a_sec = std::min(length / 1000, std::max(0, m_a / 1000));
    int b_sec = std::min(length / 1000, std::max(0, m_b / 1000)) + 1;

    for (int sec = a_sec; sec < b_sec; sec++)
    {
        int time_x = (double)(sec * 1000) / time_window * width();
        time_x -= global_x;
        painter.drawLine(time_x, 0, time_x, height());
    }

    float time_window_pos = (float)(m_time - m_a) / (m_b - m_a);
    int time_x = time_window_pos * width();
    painter.fillRect(time_x - 1, 0, 3, height(), Qt::red);
}

void WaveformView::mouseReleaseEvent(QMouseEvent* event)
{
    int x = event->pos().x();
    int time = (float)x / width() * (m_b - m_a) + m_a;
    emit mouse_release(time, event);
}

void WaveformView::wheelEvent(QWheelEvent* event)
{
    int x = event->position().x();
    int time = (float)x / width() * (m_b - m_a) + m_a;
    emit wheel_event(time, event);
}
