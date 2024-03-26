#pragma once

#include <playback_mediator.h>
#include <mode_mediator.h>
#include <clip_mediator.h>

class App;
struct PlaybackMediator;
class SubtitlesList;
class IClipQueue;

class SubtitlesView;
class Ui_PlayerWindow;

class SpinBox;
class StarWidget;

namespace qsubs
{
    class ISubtitles;
}

class ClipModule : public QObject, public ClipUnit
{
public:
    ClipModule(App* app, const SubtitlesList* subtitles_list, IClipQueue* clip_queue,
        ModeMediator*, PlaybackMediator*, ClipMediator*);

    void setup_player(Ui_PlayerWindow* player_window);

    void load(const Clip& clip) override;
    void save(ClipUserData& clip) override;

private:
    void set_mode(PlayerWindowMode);

    void on_edt_loop_a_value_changed(int);
    void on_edt_loop_b_value_changed(int);

    void on_add_new_clip();
    void on_save_new_clip();
    void on_cancel_new_clip();
    void on_replay();
    void on_next_clip();
    void on_prev_clip();
    void on_rating_changed(int);

    void clip_reviewed();

    App* m_app = nullptr;
    const SubtitlesList* m_subtitles_list = nullptr;
    IClipQueue* m_clip_queue = nullptr;
    ModeMediator* m_mode_mediator = nullptr;
    PlaybackMediator* m_playback_mediator = nullptr;
    ClipMediator* m_clip_mediator = nullptr;

    Ui_PlayerWindow* m_pw = nullptr;

    SpinBox* m_edt_loop_a = nullptr;
    SpinBox* m_edt_loop_b = nullptr;
    QAction* m_edt_loop_a_action = nullptr;
    QAction* m_edt_loop_b_action = nullptr;
    StarWidget* m_star_widget = nullptr;
    QAction* m_star_widget_action = nullptr;

    QAction* m_act_next_clip = nullptr;
    QAction* m_act_prev_clip = nullptr;
    QAction* m_act_remove_clip = nullptr;
    QAction* m_act_add_to_favorite = nullptr;

    QShortcut* m_clip_reviewed_shortcut = nullptr;

    QToolButton* m_btn_replay = nullptr;
    QAction* m_act_replay = nullptr;

    int m_num_replays = 0;
};
