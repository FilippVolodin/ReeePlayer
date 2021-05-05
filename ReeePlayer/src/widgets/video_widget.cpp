#include "pch.h"
#include "video_widget.h"

constexpr int MIN_INTERVAL = 100;

VideoWidget::VideoWidget(QWidget* parent)
    : QWidget(parent)
{
}

VideoWidget::~VideoWidget()
{
    delete m_timer;
    libvlc_media_player_release(m_vlc_mp);
}

void VideoWidget::init(libvlc_instance_t* vlc_inst)
{
    m_vlc_inst = vlc_inst;
    m_vlc_mp = libvlc_media_player_new(m_vlc_inst);

    void* drawable = reinterpret_cast<unsigned __int64*>(this->winId());
    libvlc_media_player_set_hwnd(m_vlc_mp, drawable);

    m_vlc_mp_events = libvlc_media_player_event_manager(m_vlc_mp);
    libvlc_event_attach(m_vlc_mp_events, libvlc_MediaPlayerOpening,
        libvlc_mp_callback, this);
    libvlc_event_attach(m_vlc_mp_events, libvlc_MediaPlayerTimeChanged,
        libvlc_mp_callback, this);
    libvlc_event_attach(m_vlc_mp_events, libvlc_MediaPlayerPlaying,
        libvlc_mp_callback, this);
    libvlc_event_attach(m_vlc_mp_events, libvlc_MediaPlayerPaused,
        libvlc_mp_callback, this);
    libvlc_event_attach(m_vlc_mp_events, libvlc_MediaPlayerStopped,
        libvlc_mp_callback, this);
    libvlc_event_attach(m_vlc_mp_events, libvlc_MediaPlayerEndReached,
        libvlc_mp_callback, this);
    libvlc_event_attach(m_vlc_mp_events, libvlc_MediaPlayerLengthChanged,
        libvlc_mp_callback, this);

    libvlc_video_set_mouse_input(m_vlc_mp, 0);
    libvlc_video_set_key_input(m_vlc_mp, 0);

    libvlc_audio_set_volume(m_vlc_mp, 100);

    connect(this, &VideoWidget::end_reached,
        this, &VideoWidget::repeat);

    m_timer = new CallBackTimer(10, [mp = m_vlc_mp]()
    {
        libvlc_media_player_pause(mp);
    });

    connect(this, &VideoWidget::playing, [this]()
    {
        this->m_timer->start();
    });
    connect(this, &VideoWidget::paused, [t = m_timer]() {t->stop();  });
    connect(this, &VideoWidget::stopped, [t = m_timer]() {t->stop();  });
}

void VideoWidget::set_file_name(const QString& file_name, bool)
{
    m_timer->stop();
    m_timer->set_trigger(-1);
    sync_stop();

    QString file = QDir::toNativeSeparators(file_name);
    
    qDebug(">>> file: %s", qPrintable(file));
    libvlc_media_t* vlc_media =
        libvlc_media_new_path(m_vlc_inst, file.toUtf8().data());
    libvlc_media_player_set_media(m_vlc_mp, vlc_media);
    libvlc_media_parse(vlc_media);
    libvlc_media_release(vlc_media);
}

void VideoWidget::play()
{
    m_timer->set_trigger(-1);
    libvlc_media_player_play(m_vlc_mp);
}

void VideoWidget::play(int start_time, int stop_time, int repeats)
{
    qDebug("AB: %d %d", start_time, stop_time);
    if (stop_time < start_time + MIN_INTERVAL)
        return;

    m_start_time = start_time;
    m_stop_time = stop_time;
    m_repeats = repeats;

    m_timer->set_trigger(-1);

    libvlc_media_player_play(m_vlc_mp);
    libvlc_media_player_set_time(m_vlc_mp, start_time);

    m_timer->set_player_time(start_time);
    m_timer->set_trigger(stop_time);
}

void VideoWidget::pause()
{
    if(is_playing())
        toggle_pause();
}

void VideoWidget::toggle_pause()
{
    libvlc_media_player_pause(m_vlc_mp);
}

void VideoWidget::stop()
{
    libvlc_media_player_stop(m_vlc_mp);
}

bool VideoWidget::is_playing() const
{
    return libvlc_media_player_is_playing(m_vlc_mp);
}

bool VideoWidget::is_stopped() const
{
    libvlc_media_t* m = libvlc_media_player_get_media(m_vlc_mp);
    return m == nullptr || libvlc_media_get_state(m) == libvlc_Stopped;
}

void VideoWidget::set_time(int value)
{
    if(value != libvlc_media_player_get_time(m_vlc_mp))
        libvlc_media_player_set_time(m_vlc_mp, value);
}

void VideoWidget::set_rate(float rate)
{
    if (libvlc_media_player_set_rate(m_vlc_mp, rate) != -1)
    {
        m_timer->set_rate(rate);
    }
}

int VideoWidget::get_time() const
{
    return libvlc_media_player_get_time(m_vlc_mp);
}

int VideoWidget::get_length() const
{
    return m_length;
}

bool VideoWidget::at_end() const
{
    return get_time() + 1000 >= get_length();
}

void VideoWidget::mouseReleaseEvent(QMouseEvent *)
{
    this->setFocus();
}

void VideoWidget::repeat()
{
    --m_repeats;
    if (m_repeats > 0)
    {
        libvlc_media_player_set_time(m_vlc_mp, m_start_time);
    }
    else
    {
        pause();
    }
}

void VideoWidget::libvlc_mp_callback(const libvlc_event_t* event, void* data)
{
    VideoWidget* videoWidget = static_cast<VideoWidget *>(data);
    switch (event->type)
    {
    case libvlc_MediaPlayerOpening:
    {
        emit videoWidget->opening();
        break;
    }
    case libvlc_MediaPlayerTimeChanged:
    {
        libvlc_time_t time = event->u.media_player_time_changed.new_time;
        qDebug(">>> libvlc_MediaPlayerTimeChanged: %d", time);
        videoWidget->m_timer->set_player_time(time);
        emit videoWidget->time_changed(time);
        break;
    }
    case libvlc_MediaPlayerPlaying:
        qDebug(">>> libvlc_MediaPlayerPlaying");
        emit videoWidget->playing();
        break;
    case libvlc_MediaPlayerPaused:
    {
        qDebug(">>> libvlc_MediaPlayerPaused");
        emit videoWidget->paused();
        break;
    }
    case libvlc_MediaPlayerStopped:
        qDebug(">>> libvlc_MediaPlayerStopped");
        emit videoWidget->stopped();
        break;
    case libvlc_MediaPlayerEndReached:
        qDebug(">>> libvlc_MediaPlayerEndReached");
        emit videoWidget->end_reached();
        break;
    case libvlc_MediaPlayerLengthChanged:
    {
        qDebug(">>> libvlc_MediaPlayerLengthChanged");
        int length = event->u.media_player_length_changed.new_length;
        videoWidget->m_length = length;
            
        emit videoWidget->length_changed(length);
        break;
    }
    }
}

void VideoWidget::sync_stop()
{
    stop();
    while (!is_stopped())
        Sleep(15);
}

