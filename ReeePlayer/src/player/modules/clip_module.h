#pragma once

#include <playback_mediator.h>
#include <mode_mediator.h>
#include <clip_mediator.h>

class App;
struct PlaybackMediator;

class SubtitlesView;
class Ui_PlayerWindow;

class SpinBox;

namespace qsubs
{
    class ISubtitles;
}

class ClipModule : public QObject, public ClipUnit
{
public:
    ClipModule(App* app, ModeMediator*, PlaybackMediator*);

    void setup_player(Ui_PlayerWindow* player_window);

    void load(const Clip& clip) override;
    void save(ClipUserData& clip) override;
private:
    void set_mode(PlayerWindowMode);

    void on_edt_loop_a_value_changed(int);
    void on_edt_loop_b_value_changed(int);

    App* m_app = nullptr;
    ModeMediator* m_mode_mediator = nullptr;
    PlaybackMediator* m_playback_mediator = nullptr;

    SpinBox* m_edt_loop_a = nullptr;
    SpinBox* m_edt_loop_b = nullptr;
    QAction* m_edt_loop_a_action = nullptr;
    QAction* m_edt_loop_b_action = nullptr;

    QAction* m_act_remove_clip = nullptr;
    QAction* m_act_add_to_favorite = nullptr;
};
