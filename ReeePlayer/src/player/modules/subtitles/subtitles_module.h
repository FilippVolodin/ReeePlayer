#pragma once

#include <player_module.h>
#include <qsubtitles.h>

#include <playback_mediator.h>
#include <mode_mediator.h>
#include <clip_mediator.h>

#include <vector>

class App;
struct PlaybackMediator;

//namespace pm
//{
//    class SubtitlesView;
//}

class SubtitlesView;
class Ui_PlayerWindow;

namespace qsubs
{
    class ISubtitles;
}

class SubtitlesModule : public QObject, public ClipUnit
{
public:
    SubtitlesModule(int subs_index, App* app, ModeMediator*, PlaybackMediator*);

    void setup_player(Ui_PlayerWindow* player_window);

    void load(const Clip& clip) override;
    void save(ClipUserData& clip) override;
private:
    void set_mode(PlayerWindowMode);

    void set_file(const QString&);
    void set_time(int);

    void set_subtitles(int index, const QString& filename);

    void update_cue(int time);
    void update_insert_button(int value);

    int m_subs_index = -1;
    App* m_app = nullptr;
    ModeMediator* m_mode_mediator = nullptr;
    PlaybackMediator* m_playback_mediator = nullptr;

    SubtitlesView* m_view = nullptr;

    std::vector<QString> m_subtitle_files;
    std::shared_ptr<const qsubs::ISubtitles> m_subtitles;
    const qsubs::ICue* m_current_cue;
};