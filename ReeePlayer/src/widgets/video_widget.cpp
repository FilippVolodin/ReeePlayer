#include "pch.h"
#include "video_widget.h"
#include "emitter.h"

constexpr int MIN_INTERVAL = 100;

using namespace std::chrono;

class CallBackTimer
{

public:
    CallBackTimer(int interval, std::function<void(void)> func)
        :m_is_executed(false), m_is_destroyed(false)
    {
        m_thread = std::thread([this, interval, func]()
            {
                while (!m_is_destroyed.load(std::memory_order_acquire))
                {
                    if (m_is_executed.load(std::memory_order_acquire))
                    {
                        if (m_trigger_time != -1 && get_time() > m_trigger_time)
                        {
                            func();
                            m_trigger_time = -1;
                        }
                    }
                    std::this_thread::sleep_for(
                        std::chrono::milliseconds(interval));
                }
            });

    }

    ~CallBackTimer() {
        m_is_destroyed.store(true, std::memory_order_release);
        if (m_thread.joinable())
            m_thread.join();
    }

    void stop()
    {
        set_player_time(get_time());
        m_is_executed.store(false, std::memory_order_release);
    }

    void start()
    {
        m_is_executed.store(true, std::memory_order_release);
        m_last_time = high_resolution_clock::now();
    }

    bool is_running() const noexcept {
        return (m_is_executed.load(std::memory_order_acquire) &&
            m_thread.joinable());
    }

    libvlc_time_t get_player_time()
    {
        return m_player_time;
    }

    void set_player_time(libvlc_time_t player_time)
    {
        m_player_time.store(player_time, std::memory_order_release);

        m_last_time = high_resolution_clock::now();
    }

    void set_trigger(libvlc_time_t trigger_time)
    {
        m_trigger_time = trigger_time;
        m_is_executed.store(true, std::memory_order_release);
    }

    void set_rate(float rate)
    {
        set_player_time(get_time());
        m_rate = rate;
    }

    libvlc_time_t get_time() const
    {
        if (!m_is_executed)
            return m_player_time;

        high_resolution_clock::time_point cur_time =
            high_resolution_clock::now();

        int diff = (cur_time - m_last_time.load()).count() / 1000000;
        return m_player_time.load() + diff * m_rate.load();
    }
private:

    std::thread m_thread;
    std::atomic<bool> m_is_executed;
    std::atomic<bool> m_is_destroyed;
    std::atomic<libvlc_time_t> m_player_time;
    std::atomic<libvlc_time_t> m_trigger_time;
    std::atomic<float> m_rate = 1.0;

    std::atomic<high_resolution_clock::time_point> m_last_time;
};

VideoWidget::VideoWidget(libvlc_instance_t* vlc_inst, QWidget* parent)
    : QWidget(parent)
{
    m_emitter = std::make_unique<Emitter>();
    init(vlc_inst);
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

    //connect(this, &VideoWidget::end_reached,
    //    this, &VideoWidget::repeat);

    m_timer = new CallBackTimer(10, [this, mp = m_vlc_mp]()
    {
        emit m_emitter->timer_triggered(this->m_timer->get_time());
    });

    connect(get_emitter(), &Emitter::playing, [this]()
    {
        this->m_timer->start();
    });
    connect(get_emitter(), &Emitter::paused, [t = m_timer]() {t->stop();  });
    connect(get_emitter(), &Emitter::stopped, [t = m_timer]() {t->stop();  });
}

void VideoWidget::set_file_name(const QString& file_name, bool)
{
    if (m_file_name == file_name)
        return;

    m_file_name = file_name;

    m_timer->stop();
    m_timer->set_trigger(-1);
    sync_stop();

    QString file = QDir::toNativeSeparators(file_name);
    
    qDebug(">>> file: %s", qPrintable(file));
    libvlc_media_t* vlc_media =
        libvlc_media_new_path(m_vlc_inst, file.toUtf8().data());

    libvlc_event_manager_t* media_events = libvlc_media_event_manager(vlc_media);
    libvlc_event_attach(media_events, libvlc_MediaParsedChanged,
        libvlc_mp_callback, this);
    libvlc_event_attach(media_events, libvlc_MediaMetaChanged,
        libvlc_mp_callback, this);

    libvlc_media_player_set_media(m_vlc_mp, vlc_media);
    libvlc_media_parse(vlc_media);
    libvlc_media_release(vlc_media);
}

void VideoWidget::play()
{
    m_timer->set_trigger(-1);
    m_timer->start();
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

void VideoWidget::set_timer(int stop_time)
{
    m_timer->set_trigger(stop_time);
}

void VideoWidget::pause()
{
    if (is_playing())
    {
        m_timer->stop();
        toggle_pause();
    }
}

void VideoWidget::toggle_pause()
{
    libvlc_media_player_pause(m_vlc_mp);
}

void VideoWidget::stop()
{
    m_timer->stop();
    libvlc_media_player_stop(m_vlc_mp);
}

bool VideoWidget::is_playing() const
{
    return libvlc_media_player_is_playing(m_vlc_mp);
}

bool VideoWidget::is_stopped() const
{
    libvlc_media_t* m = libvlc_media_player_get_media(m_vlc_mp);
    return !m || libvlc_media_get_state(m) == libvlc_Stopped;
}

void VideoWidget::set_time(int value)
{
    qDebug("VideoWidget::set_time %d -> %d", libvlc_media_player_get_time(m_vlc_mp), value);
    if (value != libvlc_media_player_get_time(m_vlc_mp))
    {
        libvlc_media_player_set_time(m_vlc_mp, value);
        if (!is_playing())
            m_timer->set_player_time(value);
    }
}

float VideoWidget::get_rate() const
{
    return libvlc_media_player_get_rate(m_vlc_mp);
}

void VideoWidget::set_rate(float rate)
{
    if (std::abs(get_rate() - rate) > 0.1)
    {
        if (libvlc_media_player_set_rate(m_vlc_mp, rate) != -1)
            m_timer->set_rate(rate);
    }
}

void VideoWidget::set_volume(int volume)
{
    if (volume != m_volume)
    {
        libvlc_audio_set_volume(m_vlc_mp, volume);
        m_volume = volume;
    }
}

int VideoWidget::get_time() const
{
    return libvlc_media_player_get_time(m_vlc_mp);
}

int VideoWidget::get_accuracy_time() const
{
    return m_timer->get_time();
}

int VideoWidget::get_length() const
{
    return m_length;
}

bool VideoWidget::at_end() const
{
    return get_time() + 1000 >= get_length();
}

void VideoWidget::set_audio_track(int track_index)
{
    libvlc_audio_set_track(m_vlc_mp, track_index);
}

void VideoWidget::prepare_to_destroy()
{
}

QWidget* VideoWidget::get_widget()
{
    return this;
}

Emitter* VideoWidget::get_emitter()
{
    return m_emitter.get();
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
    Emitter* m_emitter = videoWidget->get_emitter();
    switch (event->type)
    {
    case libvlc_MediaPlayerOpening:
    {
        emit m_emitter->opening();
        break;
    }
    case libvlc_MediaPlayerTimeChanged:
    {
        libvlc_time_t time = event->u.media_player_time_changed.new_time;
        qDebug(">>> libvlc_MediaPlayerTimeChanged: %d", time);
        videoWidget->m_timer->set_player_time(time);
        emit m_emitter->time_changed(time);
        break;
    }
    case libvlc_MediaPlayerPlaying:
        qDebug(">>> libvlc_MediaPlayerPlaying");
        emit m_emitter->playing();
        break;
    case libvlc_MediaPlayerPaused:
    {
        qDebug(">>> libvlc_MediaPlayerPaused");
        emit m_emitter->paused();
        break;
    }
    case libvlc_MediaPlayerStopped:
        qDebug(">>> libvlc_MediaPlayerStopped");
        emit m_emitter->stopped();
        break;
    case libvlc_MediaPlayerEndReached:
        qDebug(">>> libvlc_MediaPlayerEndReached");
        emit m_emitter->end_reached();
        break;
    case libvlc_MediaPlayerLengthChanged:
    {
        qDebug(">>> libvlc_MediaPlayerLengthChanged");
        int length = event->u.media_player_length_changed.new_length;
        videoWidget->m_length = length;
            
        emit m_emitter->length_changed(length);
        videoWidget->set_audio_track(2);
        qDebug(">>> libvlc_audio_get_track_count %d", libvlc_audio_get_track_count(videoWidget->m_vlc_mp));
        break;
    }
    case libvlc_MediaParsedChanged:
    {
        qDebug(">>> libvlc_MediaParsedChanged");
        break;
    }
    case libvlc_MediaMetaChanged:
    {
        qDebug(">>> libvlc_MediaMetaChanged");
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

