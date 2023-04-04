#ifndef VIDEO_WIDGET_H
#define VIDEO_WIDGET_H

#include "i_video_widget.h"

class CallBackTimer;

class VideoWidget : public QWidget, public IVideoWidget
{
public:
    VideoWidget(libvlc_instance_t*, QWidget* parent = nullptr);
    ~VideoWidget();

    void set_file_name(const QString&, bool auto_play = false) override;
    void unload() override;

    void play() override;
    void play(int from, int to, int repeats) override;
    void set_timer(int time) override;
    void pause() override;
    void toggle_pause() override;
    void stop() override;
    bool is_playing() const override;
    bool is_stopped() const override;
    void set_time(int) override;
    float get_rate() const override;
    void set_rate(float) override;
    void set_volume(int) override;

    int get_time() const override;
    int get_accuracy_time() const override;
    int get_length() const override;
    bool at_end() const override;

    void set_audio_track(int) override;

    void prepare_to_destroy() override;

    QWidget* get_widget() override;
    Emitter* get_emitter() override;

protected:
    void mouseReleaseEvent(QMouseEvent *event) override;
private:
    void init(libvlc_instance_t*);
    void repeat();
    static void libvlc_mp_callback(const libvlc_event_t* event, void* data);
    void sync_stop();

    std::unique_ptr<Emitter> m_emitter;

    libvlc_media_player_t* m_vlc_mp = nullptr;
    libvlc_instance_t* m_vlc_inst = nullptr;
    libvlc_event_manager_t* m_vlc_mp_events = nullptr;

    QString m_file_name;

    int m_volume = 100;

    int m_start_time = 0;
    int m_stop_time = 0;
    int m_repeats = 0;
    int m_length = 0;

    CallBackTimer* m_timer = nullptr;
};

#endif // !VIDEO_WIDGET_H
