#pragma once

#include <QObject>

class QMainWindow;

class App;
class IVideoWidget;
class PlaybackPresenter;

class Ui_PlayerWindow;

enum class PlaybackEventSource : int;

class VideoModule : public QObject
{
public:
    VideoModule(App* app, PlaybackPresenter*);
    ~VideoModule();
    void setup_player(Ui_PlayerWindow*);
private:
    // Signals from presenter
    void play(PlaybackEventSource);
    void set_time(int, PlaybackEventSource);
    void set_file(const QString&);

    // Signals from player
    void time_changed(int time);
    void length_changed(int length);

    App* m_app;
    PlaybackPresenter* m_playback_presenter;
    IVideoWidget* m_video_widget;
};