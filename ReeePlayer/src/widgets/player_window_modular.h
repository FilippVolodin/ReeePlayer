#pragma once

// TODO temporary
#include <player_window.h>

#include <ui_player_window.h>

class App;
class VideoModule;
class PlaybackControlModule;
class PlaybackPresenter;
class IClipQueue;

class PlayerWindowModular : public QMainWindow
{
public:
    PlayerWindowModular(App* app, QWidget* parent = Q_NULLPTR);
    void run(Mode, std::shared_ptr<IClipQueue>);

protected:
    void showEvent(QShowEvent* event);

private:
    bool m_showed = false;
    App* m_app;
    std::shared_ptr<IClipQueue> m_clip_queue;

    std::unique_ptr<VideoModule> m_video_module;
    std::unique_ptr<PlaybackControlModule> m_playback_module;

    std::unique_ptr<PlaybackPresenter> m_playback_presenter;

    Ui::PlayerWindow ui;
};