#ifndef PLAYER_WINDOW_H
#define PLAYER_WINDOW_H

#include "ui_player_window.h"

constexpr int NUM_SUBS_VIEWS = 2;

class Clip;
class File;
class ClipInfoDialog;
class App;
class Library;
class Session;
class JumpCutter;
class JumpCutterSettings;

namespace qsubs
{
    class ISubtitles;
    class ICue;
}

class PlayerWindow;
class SpinBox;

enum class State {
    VideoNotLoaded, Watching, AddingClip, WatchingClip, RepeatingClip
};

class UIState
{
public:
    UIState(PlayerWindow*);
    virtual void activate();

    virtual void play() {};
    virtual void set_time(int) {}

    virtual void on_close() {}
    virtual void on_save_clip() {}
    virtual void on_cancel_clip() {}
    virtual void on_remove_clip() {}
    virtual void on_time_changed(int) {};
    virtual void on_player_timer_triggered(int) {};
protected:
    PlayerWindow* m_pw;
};

class VideoNotLoadedState : public UIState
{
public:
    VideoNotLoadedState(PlayerWindow*);
};

class WatchingState : public UIState
{
public:
    WatchingState(PlayerWindow*);
    void activate() override;

    void play() override;
    void on_close() override;
    void on_time_changed(int) override;
    void on_player_timer_triggered(int) override;
    void set_time(int) override;
};

class ClipState : public UIState
{
public:
    ClipState(PlayerWindow*);
    void activate() override;
};

class AddingClipState : public ClipState
{
public:
    AddingClipState(PlayerWindow*);
    void activate() override;

    void play() override;
    void on_save_clip() override;
    void on_cancel_clip() override;
    void on_player_timer_triggered(int) override;
};

class WatchingClipState : public ClipState
{
public:
    WatchingClipState(PlayerWindow*);
    void activate() override;

    void play() override;
    void on_save_clip() override;
    void on_cancel_clip() override;
    void on_remove_clip() override;
    void on_player_timer_triggered(int) override;
};

class RepeatingClipState : public ClipState
{
public:
    RepeatingClipState(PlayerWindow*);
    void activate() override;

    void play() override;
    void on_remove_clip() override;
    void on_player_timer_triggered(int) override;
};

enum class Mode { Watching, WatchingClip, Repeating };

class PlayerWindow : public QMainWindow
{
    Q_OBJECT

    friend UIState;
    friend VideoNotLoadedState;
    friend ClipState;
    friend WatchingState;
    friend AddingClipState;
    friend WatchingClipState;
    friend RepeatingClipState;

public:
    PlayerWindow(App* app, QWidget* parent = Q_NULLPTR);
    ~PlayerWindow();

    QMenu* createPopupMenu() override;

    void watch(File* file);
    void watch_clip(Clip* clip);
    void repeat(std::vector<File*>&& files);

    void save_player_time();
protected:
    void showEvent(QShowEvent *event);
    void closeEvent(QCloseEvent *event);
    void timerEvent(QTimerEvent* event) override;
private:

    void setup_actions();
    void setup_player_events();
    void setup_subs_views();
    void setup_slider();
    void setup_shortcuts();
    void setup_playback_rates();
    void restore_state();

    void on_actAddClip_triggered();
    void on_actSaveClip_triggered();
    void on_actCancelClip_triggered();
    void on_actPrevClip_triggered();
    void on_actNextClip_triggered();
    void on_actRemoveClip_triggered();
    void on_actPlayPause_triggered();
    void on_actRepeatClip_triggered();
    void on_actJumpCutterSettings_triggered();
    void on_actShowWaveform_triggered(bool);
    void on_actJumpCutter_triggered(bool);

    void on_btnMinus_clicked(bool);
    void on_btnPlus_clicked(bool);

    void on_player_time_changed(int);
    void on_player_playing();
    void on_player_paused();
    void on_player_timer_triggered(int64_t);

    void on_slider_value_changed(int);

    void on_edt_loop_a_value_changed(int);
    void on_edt_loop_b_value_changed(int);

    //void on_forward_shortcut_activated();
    //void on_backward_shortcut_activated();
    void on_change_loop_shortcut_activated();
    void on_escape_shortcut_activated();
    void on_show_subtitles_shortcut_activated(int index);

    void on_subtitles_show_always(int index, bool);
    void on_subtitles_insert_clicked(int index, int);
    void on_subs_file_changed(int index, int);

    void set_slider_value(int value);

    void set_duration(int);
    void set_time(int);

    void set_state(std::shared_ptr<UIState>);

    void show_video();

    void show_clip();
    void refresh_clip_info();

    bool remove_clip_confirmation();
    void update_clip_interval(Clip*);
    void update_clip_subtitles(Clip*);

    void update_insert_button(int index, int value);

    void set_playback_rate(int, bool play = false);
    void on_set_playback_rate(QAbstractButton*);

    void set_subtitles(int index, const QString& filename);
    void update_cue(int index);

    void save_subs_priority();

    int get_loop_a() const;
    int get_loop_b() const;

    void rewind(int);

    void save_new_clip();
    void save_current_clip();

    bool remove_clip();

    void next_clip();

    void jumpcutter(int);

    Ui::PlayerWindow ui;

    bool m_showed = false;

    App* m_app;
    Library* m_library;
    std::shared_ptr<Session> m_session;
    std::shared_ptr<JumpCutter> m_jc;
    //std::shared_ptr<JumpCutterSettings> m_jc_settings;

    ClipInfoDialog* m_clip_info_dialog;

    Mode m_mode;

    std::shared_ptr<UIState> m_ui_state;

    File* m_file = nullptr;
    Clip* m_clip = nullptr;
    bool m_close_after_stopped = false;

    float m_next_level = 0.0;

    float m_playback_rate = 1.0;

    std::vector<QString> m_subtitle_files;
    SubtitlesView* m_subtitle_views[NUM_SUBS_VIEWS];
    std::shared_ptr<const qsubs::ISubtitles> m_subtitles[NUM_SUBS_VIEWS];
    const qsubs::ICue* m_cues[NUM_SUBS_VIEWS];

    SpinBox* m_edt_loop_a;
    QAction* m_edt_loop_a_action;
    SpinBox* m_edt_loop_b;
    QAction* m_edt_loop_b_action;

    QShortcut* m_forward_shortcut;
    QShortcut* m_backward_shortcut;
    QShortcut* m_escape_shortcut;
    QShortcut* m_show_subtitles0_shortcut;
    QShortcut* m_show_subtitles1_shortcut;

    QColor m_default_bg;

    QButtonGroup* m_rate_btn_group;
};

#endif // !PLAYER_WINDOW_H

