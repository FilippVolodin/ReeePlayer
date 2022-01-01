#ifndef VIDEO_WIDGET_H
#define VIDEO_WIDGET_H

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
                        m_is_executed.store(false, std::memory_order_release);
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
        m_is_executed.store(false, std::memory_order_release);
    }

    void start()
    {
        m_is_executed.store(true, std::memory_order_release);
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
    }

    void set_rate(float rate)
    {
        m_rate = rate;
    }
private:
    libvlc_time_t get_time() const
    {
        high_resolution_clock::time_point cur_time =
            high_resolution_clock::now();

        int diff = (cur_time - m_last_time).count() / 1000000;
        return m_player_time + diff * m_rate;
    }

    std::thread m_thread;
    std::atomic<bool> m_is_executed;
    std::atomic<bool> m_is_destroyed;
    std::atomic<libvlc_time_t> m_player_time;
    std::atomic<libvlc_time_t> m_trigger_time;
    std::atomic<float> m_rate;

    high_resolution_clock::time_point m_last_time;
};

class VideoWidget : public QWidget
{
    Q_OBJECT

public:
    libvlc_media_player_t* m_vlc_mp = nullptr;

    VideoWidget(QWidget* parent);
    ~VideoWidget();

    void init(libvlc_instance_t*);

    void set_file_name(const QString&, bool auto_play = false);

    void play();
    void play(int from, int to, int repeats);
    void set_timer(int time);
    void pause();
    void toggle_pause();
    void stop();
    bool is_playing() const;
    bool is_stopped() const;
    void set_time(int);
    void set_rate(float);

    int get_time() const;
    int get_length() const;
    bool at_end() const;

signals:
    void opening();
    void length_changed(int);
    void time_changed(int);
    void playing();
    void paused();
    void stopped();
    void end_reached();
    void timer_triggered(int64_t);

protected:
    void mouseReleaseEvent(QMouseEvent *event) override;
private:
    void repeat();
    static void libvlc_mp_callback(const libvlc_event_t* event, void* data);
    void sync_stop();

    QString m_file_name;

    libvlc_instance_t* m_vlc_inst = nullptr;
    libvlc_event_manager_t* m_vlc_mp_events = nullptr;

    int m_start_time = 0;
    int m_stop_time = 0;
    int m_repeats = 0;
    int m_length = 0;

    CallBackTimer* m_timer = nullptr;
};

#endif // !VIDEO_WIDGET_H
