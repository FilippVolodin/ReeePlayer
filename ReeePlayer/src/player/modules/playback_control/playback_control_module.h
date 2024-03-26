#pragma once

#include <QObject>
#include <time_types.h>
#include <mode_mediator.h>
#include <playback_mediator.h>

class QSlider;

class Ui_PlayerWindow;

class App;
class File;
class IClipQueue;

class PlaybackControlModule : public QObject
{
public:
    PlaybackControlModule(App* app, IClipQueue* clip_queue, ModeMediator*, PlaybackMediator*);
    void setup_player(Ui_PlayerWindow* player_window);
protected:
    void timerEvent(QTimerEvent* event);
private:
    void setup_playback_rates(Ui_PlayerWindow* pw);

    void set_playback_rate(PlaybackRate, bool play = false);
    void on_set_playback_rate(QAbstractButton* button);

    void slider_pressed();
    void slider_moved(int);
    void slider_released();

    void act_play_pause_triggered();

    void set_mode(PlayerWindowMode);

    void set_file(const File*);
    void set_time(PlaybackTime);
    void set_length(PlaybackTime);
    void set_state(PlayState);
    void set_default_rate(PlaybackRate);

    void update_label(PlaybackTime time);

    void rewind(int);

    App* m_app = nullptr;
    IClipQueue* m_clip_queue = nullptr;
    ModeMediator* m_mode_mediator = nullptr;
    PlaybackMediator* m_playback_mediator = nullptr;

    QSlider* m_slider = nullptr;
    QLabel* m_lbl_time = nullptr;
    QAction* m_act_play_pause = nullptr;
    QToolButton* m_btn_play = nullptr;
    QButtonGroup* m_rate_btn_group = nullptr;

    QShortcut* m_forward_shortcut = nullptr;
    QShortcut* m_backward_shortcut = nullptr;

    PlaybackTime m_press_slider_time = 0;
    PlaybackTime m_sent_time = 0;
    bool m_slider_pressed = false;
    int m_timer_id = 0;
};