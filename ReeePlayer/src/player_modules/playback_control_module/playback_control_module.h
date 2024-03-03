#pragma once

#include <QObject>
#include <time_types.h>

class QSlider;

class Ui_PlayerWindow;

class App;
class PlaybackPresenter;

enum class PlaybackEventSource : int;

class PlaybackControlModule : public QObject
{
public:
    PlaybackControlModule(App* app, PlaybackPresenter*);
    void setup_player(Ui_PlayerWindow* player_window);
protected:
    void timerEvent(QTimerEvent* event);
private:
    void slider_pressed();
    void slider_moved(int);
    void slider_released();

    void slider_value_changed(int);

    void set_time(PlaybackTime, PlaybackEventSource);
    void set_length(PlaybackTime);

    void update_label(PlaybackTime time);

    App* m_app = nullptr;
    PlaybackPresenter* m_playback_presenter = nullptr;

    QSlider* m_slider = nullptr;
    QLabel* m_lbl_time = nullptr;

    PlaybackTime m_press_slider_time = 0;
    PlaybackTime m_sent_time = 0;
    bool m_slider_pressed = false;
    int m_timer_id = 0;
};