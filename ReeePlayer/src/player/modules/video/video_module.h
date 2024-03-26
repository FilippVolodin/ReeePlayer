#pragma once

#include <QObject>
#include <playback_mediator.h>

class QMainWindow;

class App;
class IVideoWidget;
class PlaybackMediator;
class File;

class Ui_PlayerWindow;

class VideoModule : public QObject
{
public:
    VideoModule(App* app, PlaybackMediator*);
    ~VideoModule();
    void setup_player(Ui_PlayerWindow*);
private:
    // Signals from mediator
    void set_state(PlayState);
    void play(PlaybackTime, PlaybackTime);
    void set_time(PlaybackTime);
    void set_trigger_time(PlaybackTime);
    void set_file(const File*, bool auto_play = false, int start_time = 0);
    void set_rate(PlaybackRate);

    // Signals from player
    void time_changed(int time);
    void length_changed(int length);
    void playing();
    void paused();
    void stopped();
    void timer_triggered();

    App* m_app;
    PlaybackMediator* m_playback_mediator;
    IVideoWidget* m_video_widget;

    PlaybackTime m_video_time = 0;
};