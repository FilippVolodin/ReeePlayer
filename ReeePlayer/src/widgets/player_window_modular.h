#pragma once

// TODO temporary
#include <player_window.h>

#include <ui_player_window.h>

#include <mode_mediator.h>

class App;
class VideoModule;
class PlaybackControlModule;
class SubtitlesModule;
class ClipModule;

class PlaybackMediator;
class ModeMediator;
class ClipMediator;

class IClipQueue;

class PlayerWindowModular : public QMainWindow
{
public:
    PlayerWindowModular(App* app, QWidget* parent = Q_NULLPTR);
    void run(Mode, std::shared_ptr<IClipQueue>);

protected:
    void showEvent(QShowEvent* event);

private:

    void set_mode(PlayerWindowMode);

    void add_new_clip_activated();
    void save_new_clip_activated();
    void cancel_new_clip_activated();

    bool m_showed = false;
    App* m_app;
    std::shared_ptr<IClipQueue> m_clip_queue;
    Mode m_mode;

    std::unique_ptr<VideoModule> m_video_module;
    std::unique_ptr<PlaybackControlModule> m_playback_module;
    std::array<std::unique_ptr<SubtitlesModule>, 2> m_subtitles_modules;
    std::unique_ptr<ClipModule> m_clip_module;

    std::unique_ptr<ModeMediator> m_mode_mediator;
    std::unique_ptr<PlaybackMediator> m_playback_mediator;
    std::unique_ptr<ClipMediator> m_clip_mediator;

    Ui::PlayerWindow ui;
};