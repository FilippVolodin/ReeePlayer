#pragma once

#include <QObject>
#include <time_types.h>
#include <playback_mediator.h>

class QSlider;

class Ui_PlayerWindow;

class App;

class PlaybackControlModule : public QObject
{
public:
    PlaybackControlModule(App* app, PlaybackMediator*);
    void setup_player(Ui_PlayerWindow* player_window);
protected:
    void timerEvent(QTimerEvent* event);
private:
    void slider_pressed();
    void slider_moved(int);
    void slider_released();

    void act_play_pause_triggered();

    void set_time(PlaybackTime);
    void set_length(PlaybackTime);
    void set_state(PlayState);

    void update_label(PlaybackTime time);

    App* m_app = nullptr;
    PlaybackMediator* m_playback_mediator = nullptr;

    QSlider* m_slider = nullptr;
    QLabel* m_lbl_time = nullptr;
    QAction* m_act_play_pause = nullptr;

    PlaybackTime m_press_slider_time = 0;
    PlaybackTime m_sent_time = 0;
    bool m_slider_pressed = false;
    int m_timer_id = 0;
};