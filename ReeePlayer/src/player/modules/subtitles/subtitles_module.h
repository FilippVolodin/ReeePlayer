#pragma once

#include <player_module.h>
#include <qsubtitles.h>

#include <playback_mediator.h>
#include <mode_mediator.h>
#include <clip_mediator.h>

#include <vector>

class App;
class SubtitlesList;
struct PlaybackMediator;
struct File;

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
    SubtitlesModule(int subs_id, App* app, SubtitlesList*, ModeMediator*, PlaybackMediator*);

    void setup_player(Ui_PlayerWindow* player_window);

    void load(const Clip& clip) override;
    void save(ClipUserData& clip) override;
private:
    void set_mode(PlayerWindowMode);

    void set_file(const File*);
    void set_time(int);

    void set_subtitles(const QString& filename);

    void update_cue(int time);
    void update_insert_button(int value);

    void on_show_always_changed(bool);
    void on_insert_clicked(int);
    void on_subs_file_changed(int);

    void save_subs_priority();

    int m_subs_id = -1;
    App* m_app = nullptr;
    SubtitlesList* m_subtitles_list;
    ModeMediator* m_mode_mediator = nullptr;
    PlaybackMediator* m_playback_mediator = nullptr;

    SubtitlesView* m_view = nullptr;

    //std::vector<QString> m_subtitle_files;
    //std::shared_ptr<const qsubs::ISubtitles> m_subtitles;
    const qsubs::ISubtitles* m_subtitles = nullptr;
    const qsubs::ICue* m_current_cue;
};