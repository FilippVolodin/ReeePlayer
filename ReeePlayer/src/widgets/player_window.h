#ifndef PLAYER_WINDOW_H
#define PLAYER_WINDOW_H

#include "ui_player_window.h"

constexpr int NUM_SUBS_VIEWS = 2;

class Clip;
class File;
class ClipInfoDialog;
class App;
class Library;
class Waveform;
class JumpCutterSettings;
class IVideoWidget;
class VAD;
class VADSettings;
class JumpCutterSettingsDialog;
class StarWidget;
class IClipQueue;
struct ClipUserData;

namespace qsubs
{
    class ISubtitles;
    class ICue;
}

namespace srs
{
    class IModel;
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
    virtual void on_waveform_mouse_release(int /*time*/, QMouseEvent*) {};
    virtual void on_wheel_event(int /*time*/, QWheelEvent*) {};
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
    void on_waveform_mouse_release(int time, QMouseEvent*) override;
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
    void on_waveform_mouse_release(int time, QMouseEvent*) override;
    void on_wheel_event(int time, QWheelEvent*) override;
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

    void run(Mode, std::shared_ptr<IClipQueue>);

    void set_vad(std::shared_ptr<VAD>);

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

    void on_vad_progress_updated(int, int);

    void set_slider_value(int value);

    void set_duration(int);
    void set_time(int);

    void set_state(std::shared_ptr<UIState>);

    void show_video();

    void show_clip();

    bool remove_clip_confirmation();

    std::unique_ptr<ClipUserData> get_clip_user_data();

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

    void jumpcutter(int);
    void load_jc_settings();
    void save_jc_settings();
    std::shared_ptr<VADSettings> get_vad_settings() const;

    Ui::PlayerWindow ui;

    bool m_showed = false;

    App* m_app;
    std::shared_ptr<IClipQueue> m_clip_queue;
    std::shared_ptr<Waveform> m_waveform;
    std::shared_ptr<VAD> m_vad;
    std::shared_ptr<JumpCutterSettings> m_jc_settings;
    std::unique_ptr<srs::IModel> m_srs_model;

    ClipInfoDialog* m_clip_info_dialog;
    JumpCutterSettingsDialog* m_jc_dialog;

    Mode m_mode;

    std::shared_ptr<UIState> m_ui_state;

    //File* m_file = nullptr;
    //Clip* m_clip = nullptr;
    bool m_close_after_stopped = false;

    float m_playback_rate = 1.0;

    int m_num_repeats = 0;

    std::vector<QString> m_subtitle_files;
    SubtitlesView* m_subtitle_views[NUM_SUBS_VIEWS];
    std::shared_ptr<const qsubs::ISubtitles> m_subtitles[NUM_SUBS_VIEWS];
    const qsubs::ICue* m_cues[NUM_SUBS_VIEWS];

    IVideoWidget* m_video_widget;

    SpinBox* m_edt_loop_a;
    QAction* m_edt_loop_a_action;
    SpinBox* m_edt_loop_b;
    QAction* m_edt_loop_b_action;

    QShortcut* m_forward_shortcut;
    QShortcut* m_backward_shortcut;
    QShortcut* m_escape_shortcut;
    QShortcut* m_show_subtitles0_shortcut;
    QShortcut* m_show_subtitles1_shortcut;

    QLabel* m_lbl_clip_stats;
    QLabel* m_lbl_info;

    QButtonGroup* m_rate_btn_group;

    StarWidget* m_star_widget = nullptr;
};

#endif // !PLAYER_WINDOW_H

