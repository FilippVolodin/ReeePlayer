#include "pch.h"
#include "player_window.h"
#include <qsubtitles.h>
#include "models/session.h"
#include "models/clip_storage.h"
#include "models/library.h"
#include "models/app.h"
#include "spinbox.h"
#include "models/jumpcutter.h"
#include "models/vad.h"
#include "waveform_view.h"
#include "video_widget.h"
#include "web_video_widget.h"
#include "emitter.h"
#include <time.h>
#include <srs.h>
#include <star_widget.h>
#include <jc_settings_widget.h>
#include <audio_tools.h>

struct PlaybackRateItem
{
    float rate;
    const char* text;
    QKeySequence key;
};

PlaybackRateItem PLAYBACK_ITEMS[] =
{
    {0.5f,  ".5", Qt::Key_5},
    {0.6f,  ".6", Qt::Key_6},
    {0.7f,  ".7", Qt::Key_7},
    {0.8f,  ".8", Qt::Key_8},
    {0.9f,  ".9", Qt::Key_9},
    {1.0f,  " 1 ", Qt::Key_0},
    {1.25f, "1.2", Qt::Key_Minus},
    {1.5f,  "1.5", Qt::Key_Equal},
    {2.0f,  "2.0", Qt::Key_BracketRight},
};



//constexpr float PLAYBACK_RATES[] = { 0.5, 0.6, 0.7, 0.8, 0.9, 1.01, 1.25, 1.5, 2.0 };
//static const QKeySequence PLAYBACK_KEYS[] =
//    { Qt::Key_5, Qt::Key_6, Qt::Key_7, Qt::Key_8, Qt::Key_9, Qt::Key_0, Qt::Key_Minus, Qt::Key_Equal, Qt::Key_Backspace };
constexpr int DEFAULT_PLAYBACK_RATE_INDEX = 5;

constexpr int NORMAL_LOOP_STEP = 100;
constexpr int PRECISE_LOOP_STEP = 40;

constexpr const char* PLAYER_WINDOW_GEOMETRY_KEY = "player_window_geometry";
constexpr const char* WINDOW_STATE_KEY = "player_window_state";
constexpr const char* SHOW_SUBTITLES_KEY = "show_subtitles_%1_%2";

struct LC
{
    enum class Type { A, B };
    enum class Direction { Backward, Forward };
    enum class Step { Normal, Precise };
    Type type;
    Direction direction;
    Step step;
};

static const QMap<QKeySequence, LC> LOOP_KEYS =
{
    {Qt::CTRL | Qt::Key_Left,
        {LC::Type::A, LC::Direction::Backward, LC::Step::Normal}},
    {Qt::CTRL | Qt::Key_Right,
        {LC::Type::A, LC::Direction::Forward,  LC::Step::Normal}},
    {Qt::ALT | Qt::Key_Left,
        {LC::Type::B, LC::Direction::Backward, LC::Step::Normal}},
    {Qt::ALT | Qt::Key_Right,
        {LC::Type::B, LC::Direction::Forward,  LC::Step::Normal}},
};

class SliderStyle : public QProxyStyle
{
public:
    using QProxyStyle::QProxyStyle;

    int styleHint(QStyle::StyleHint hint, const QStyleOption* option = 0,
        const QWidget* widget = 0, QStyleHintReturn* returnData = 0) const
    {
        if (hint == QStyle::SH_Slider_AbsoluteSetButtons)
            return (Qt::LeftButton | Qt::MiddleButton | Qt::RightButton);
        return QProxyStyle::styleHint(hint, option, widget, returnData);
    }
};

QString ms_to_time(int ms)
{
    int s = ms / 1000;
    int m = s / 60;
    int h = m / 60;
    if (h == 0)
    {
        return QString("%1:%2").arg(m).arg(s % 60, 2, 10, QChar('0'));
    }
    else
    {
        return QString("%1:%2:%3").arg(h).arg(m % 60, 2, 10, QChar('0')).arg(s % 60, 2, 10, QChar('0'));
    }
}

QString get_show_subtitles_setting_str(Mode mode, int index)
{
    QString mode_str;
    if (mode == Mode::Watching)
        mode_str = "watching";
    else if (mode == Mode::Repeating)
        mode_str = "repeating";
    else if (mode == Mode::WatchingClip)
        mode_str = "watching_clip";

    return QString("show_subtitles_%1_%2").arg(index).arg(mode_str);
}

PlayerWindow::PlayerWindow(App* app, QWidget* parent)
    : QMainWindow(parent), m_app(app)
{
    ui.setupUi(this);

    QByteArray g = m_app->get_setting("gui", PLAYER_WINDOW_GEOMETRY_KEY).toByteArray();
    restoreGeometry(g);

    PLAYER_ENGINE player_engine = (PLAYER_ENGINE) app->get_setting("main", "player", (int)PLAYER_ENGINE::Web).toInt();

    QWidget* w;

    if (player_engine == PLAYER_ENGINE::VLC)
    {
        VideoWidget* video_widget = new VideoWidget(app->get_vlc_instance(), this);
        m_video_widget = video_widget;
        w = video_widget;
    }
    else
    {
        WebVideoWidget* video_widget = new WebVideoWidget(this);
        m_video_widget = video_widget;
        w = video_widget;
    }

    auto sp = w->sizePolicy();
    sp.setVerticalStretch(5);
    w->setSizePolicy(sp);

    QBoxLayout* lo = dynamic_cast<QBoxLayout*>(ui.centralwidget->layout());
    if (lo)
        lo->insertWidget(0, w);
    else
        lo->addWidget(w);

    setup_actions();
    setup_player_events();
    setup_subs_views();
    setup_slider();
    setup_shortcuts();
    setup_playback_rates();
    restore_state();

    ui.dockWidget1->toggleViewAction()->setText("Show/hide subtitles 1");
    ui.dockWidget2->toggleViewAction()->setText("Show/hide subtitles 2");
    ui.dockJC->toggleViewAction()->setText("Show/hide VAD Settings");

    for (const qsubs::ICue*& cue : m_cues)
        cue = nullptr;

    m_lbl_clip_stats = new QLabel(this);
    m_lbl_clip_stats->setMargin(2);
    ui.statusbar->addWidget(m_lbl_clip_stats);

    m_lbl_info = new QLabel(this);
    m_lbl_info->setMargin(2);
    ui.statusbar->addWidget(m_lbl_info);
}

PlayerWindow::~PlayerWindow()
{
    m_video_widget->prepare_to_destroy();
}

QMenu* PlayerWindow::createPopupMenu()
{
    QMenu* menu = new QMenu(this);
    menu->addAction(ui.dockWidget1->toggleViewAction());
    menu->addAction(ui.dockWidget2->toggleViewAction());
    if (m_mode == Mode::Watching)
        menu->addAction(ui.dockJC->toggleViewAction());
    return menu;
}

void PlayerWindow::run(Mode mode, std::shared_ptr<IClipQueue> clip_queue)
{
    m_mode = mode;
    m_clip_queue = clip_queue;
    show();
}

void PlayerWindow::save_player_time()
{
    int time = m_video_widget->get_time();
    std::unique_ptr<FileUserData> file_user_data = std::make_unique<FileUserData>();
    file_user_data->player_time = time;
    file_user_data->length = m_video_widget->get_length();
    m_clip_queue->set_file_user_data(std::move(file_user_data));
    m_clip_queue->save_library();
}

void PlayerWindow::showEvent(QShowEvent * event)
{
    QWidget::showEvent(event);

    if (m_showed)
        return;
    m_showed = true;

    for (int i = 0; i < NUM_SUBS_VIEWS; ++i)
    {
        QString mode_str;
        if (m_mode == Mode::Watching)
            mode_str = "watching";
        else if (m_mode == Mode::Repeating)
            mode_str = "repeating";
        else if (m_mode == Mode::WatchingClip)
            mode_str = "watching_clip";

        QString setting_name =
            QString("show_subtitles_%1_%2").arg(i).arg(mode_str);

        QVariant v = m_app->get_setting("gui", setting_name);
        m_subtitle_views[i]->set_show_always(v.isNull() ? true : v.toBool());
    }

    if (m_mode == Mode::Watching)
    {
        show_video();
        set_state(std::make_shared<WatchingState>(this));
    }
    else if (m_mode == Mode::WatchingClip)
    {
        set_state(std::make_shared<WatchingClipState>(this));
        show_clip();
    }
    else
    {
        set_state(std::make_shared<RepeatingClipState>(this));
        show_clip();
    }
}

void PlayerWindow::closeEvent(QCloseEvent * event)
{
    m_ui_state->on_close();

    m_app->set_setting("gui", WINDOW_STATE_KEY, saveState());
    m_app->set_setting("gui", PLAYER_WINDOW_GEOMETRY_KEY, saveGeometry());

    QWidget::closeEvent(event);
}

void PlayerWindow::timerEvent(QTimerEvent*)
{
    if (m_video_widget->is_playing())
    {
        int time = m_video_widget->get_accuracy_time();
        ui.waveform->set_time(time);
        ui.waveform->repaint();
    }
}

void PlayerWindow::setup_actions()
{
    ui.toolBar->addAction(ui.actAddClip);
    connect(ui.actAddClip, &QAction::triggered,
        this, &PlayerWindow::on_actAddClip_triggered);

    ui.toolBar->addSeparator();

    ui.toolBar->addAction(ui.actShowWaveform);
    connect(ui.actShowWaveform, &QAction::triggered,
        this, &PlayerWindow::on_actShowWaveform_triggered);

    m_edt_loop_a = new SpinBox(this);
    m_edt_loop_a_action = ui.toolBar->addWidget(m_edt_loop_a);
    connect(m_edt_loop_a, &SpinBox::value_changed,
        this, &PlayerWindow::on_edt_loop_a_value_changed);

    m_edt_loop_b = new SpinBox(this);
    m_edt_loop_b_action = ui.toolBar->addWidget(m_edt_loop_b);
    connect(m_edt_loop_b, &SpinBox::value_changed,
        this, &PlayerWindow::on_edt_loop_b_value_changed);

    ui.actSaveClip->setShortcuts({ tr("Return"), tr("Ctrl+Return") });
    ui.toolBar->addAction(ui.actSaveClip);
    connect(ui.actSaveClip, &QAction::triggered,
        this, &PlayerWindow::on_actSaveClip_triggered);

    ui.toolBar->addAction(ui.actCancelClip);
    connect(ui.actCancelClip, &QAction::triggered,
        this, &PlayerWindow::on_actCancelClip_triggered);

    ui.toolBar->addAction(ui.actPrevClip);
    connect(ui.actPrevClip, &QAction::triggered,
        this, &PlayerWindow::on_actPrevClip_triggered);

    ui.actNextClip->setShortcuts({ tr("Return"), tr("Ctrl+Return") });
    ui.toolBar->addAction(ui.actNextClip);
    connect(ui.actNextClip, &QAction::triggered,
        this, &PlayerWindow::on_actNextClip_triggered);

    ui.actRemoveClip->setIconText(QString());
    ui.toolBar->addAction(ui.actRemoveClip);
    connect(ui.actRemoveClip, &QAction::triggered,
        this, &PlayerWindow::on_actRemoveClip_triggered);

    ui.toolBar->addAction(ui.actAddToFavorite);

    connect(ui.actPlayPause, &QAction::triggered,
        this, &PlayerWindow::on_actPlayPause_triggered);

    connect(ui.actRepeatClip, &QAction::triggered,
        this, &PlayerWindow::on_actRepeatClip_triggered);

    connect(ui.waveform, &WaveformView::mouse_release, [this](int time, QMouseEvent* event) {
        m_ui_state->on_waveform_mouse_release(time, event); });

    connect(ui.waveform, &WaveformView::wheel_event, [this](int time, QWheelEvent* event) {
        m_ui_state->on_wheel_event(time, event); });

    QMenu* menu = new QMenu(this);
    menu->addAction(ui.dockWidget1->toggleViewAction());
    menu->addAction(ui.dockWidget2->toggleViewAction());
    menu->addAction(ui.dockJC->toggleViewAction());

    QToolButton* btn_show_panels = new QToolButton(this);
    btn_show_panels->setPopupMode(QToolButton::InstantPopup);
    btn_show_panels->setIcon(QIcon(":/MainWindow/panels"));
    btn_show_panels->setMenu(menu);
    ui.toolBar->addWidget(btn_show_panels);
    
    m_star_widget = new StarWidget(4, this);
    m_star_widget->set_rating_names({ "Again", "Hard", "Medium", "Easy" });
    m_star_widget_action = ui.toolBar->addWidget(m_star_widget);
    connect(m_star_widget, &StarWidget::rating_changed, this, &PlayerWindow::on_rating_changed);
}

void PlayerWindow::setup_player_events()
{
    connect(m_video_widget->get_emitter(), &Emitter::time_changed,
        this, &PlayerWindow::on_player_time_changed, Qt::QueuedConnection);
    connect(m_video_widget->get_emitter(), &Emitter::playing,
        this, &PlayerWindow::on_player_playing);
    connect(m_video_widget->get_emitter(), &Emitter::paused,
        this, &PlayerWindow::on_player_paused);
    connect(m_video_widget->get_emitter(), &Emitter::length_changed,
        this, &PlayerWindow::set_duration);
    connect(m_video_widget->get_emitter(), &Emitter::timer_triggered,
        this, &PlayerWindow::on_player_timer_triggered);
}

void PlayerWindow::setup_subs_views()
{
    m_subtitle_views[0] = ui.edtSubtitles1;
    m_subtitle_views[1] = ui.edtSubtitles2;

    for (int i = 0; i < NUM_SUBS_VIEWS; ++i)
    {
        connect(m_subtitle_views[i], &SubtitlesView::on_show_always, this,
            [this, i](bool show) { on_subtitles_show_always(i, show); });

        connect(m_subtitle_views[i], &SubtitlesView::on_insert_clicked, this,
            [this, i](int show) { on_subtitles_insert_clicked(i, show); });

        connect(m_subtitle_views[i], &SubtitlesView::on_file_changed,
            [this, i](int index) { on_subs_file_changed(i, index); });
    }
}

void PlayerWindow::setup_slider()
{
    ui.slider->setStyle(new SliderStyle(ui.slider->style()));
    connect(ui.slider, &QSlider::valueChanged,
        this, &PlayerWindow::on_slider_value_changed);
}

void PlayerWindow::setup_shortcuts()
{
    m_escape_shortcut = new QShortcut(Qt::Key_Escape, this);
    connect(m_escape_shortcut, &QShortcut::activated,
        this, &PlayerWindow::on_escape_shortcut_activated);

    m_show_subtitles0_shortcut = new QShortcut(Qt::Key_1, this);
    connect(m_show_subtitles0_shortcut, &QShortcut::activated,
        [this]() {on_show_subtitles_shortcut_activated(0); });

    m_show_subtitles1_shortcut = new QShortcut(Qt::Key_2, this);
    connect(m_show_subtitles1_shortcut, &QShortcut::activated,
        [this]() {on_show_subtitles_shortcut_activated(1); });

    m_forward_shortcut = new QShortcut(Qt::Key_Right, this);
    connect(m_forward_shortcut, &QShortcut::activated,
        [this]() { rewind(2000); });

    m_backward_shortcut = new QShortcut(Qt::Key_Left, this);
    connect(m_backward_shortcut, &QShortcut::activated,
        [this]() { rewind(-2000); });

    m_return_shortcut = new QShortcut(this);
    m_return_shortcut->setKeys({ tr("Return"), tr("Ctrl+Return") });
    connect(m_return_shortcut, &QShortcut::activated,
        [this]() { m_ui_state->on_next_clip(); });

    for (auto it = LOOP_KEYS.begin(); it != LOOP_KEYS.end(); ++it)
    {
        QKeySequence key = it.key();
        const LC& lc = it.value();
        SpinBox* spinbox = lc.type == LC::Type::A ? m_edt_loop_a : m_edt_loop_b;
        int step = lc.direction == LC::Direction::Forward ?
            NORMAL_LOOP_STEP : -NORMAL_LOOP_STEP;
        connect(new QShortcut(key, this), &QShortcut::activated,
            [this, spinbox, step]() { spinbox->setValue(spinbox->value() + step, false); });
    }

    int id = 0;
    for (const auto& item : PLAYBACK_ITEMS)
    {
        QShortcut* shortcut = new QShortcut(item.key, this);
        connect(shortcut, &QShortcut::activated,
            [this, id] { set_playback_rate(id, true); });
        ++id;
    }
}

void PlayerWindow::setup_playback_rates()
{
    m_rate_btn_group = new QButtonGroup(this);
    m_rate_btn_group->setExclusive(true);
    int id = 0;
    QFontMetrics fm(font());
    for (const auto& item : PLAYBACK_ITEMS)
    {
        QPushButton* b = new QPushButton(this);
        b->setText(item.text);
        b->setToolTip(QString("Hotkey: %1").arg(item.key.toString()));
        b->setCheckable(true);
        b->setMaximumWidth(fm.horizontalAdvance(item.text) + 15);
        b->setFocusPolicy(Qt::NoFocus);
        m_rate_btn_group->addButton(b);
        m_rate_btn_group->setId(b, id);
        ui.playbackLayout->addWidget(b);
        ++id;
    }
    set_playback_rate(DEFAULT_PLAYBACK_RATE_INDEX);
    connect(m_rate_btn_group, &QButtonGroup::buttonClicked,
        this, &PlayerWindow::on_set_playback_rate);
}

void PlayerWindow::setup_jc_settings()
{
    load_jc_settings();
    //if (m_vad)
    //{
        bool jc_enabled = m_app->get_setting("jumpcutter", "activated", true).toBool();
        m_jc_settings->set_activated(jc_enabled);
    //}
    //else
    //{
    //    m_jc_settings->set_enabled(false);
    //}

    m_jc_settings_widget = new JCSettingsWidget(this);
    m_jc_settings_widget->set_settings(m_jc_settings);
    connect(m_jc_settings_widget, &JCSettingsWidget::applied,
        [this]()
        {
            m_jc_settings = m_jc_settings_widget->get_settings();
            if (m_vad)
                m_vad->apply_settings(get_vad_settings());
        });
    ui.dockJC->setWidget(m_jc_settings_widget);

    bool show_vad_setting = m_app->get_setting("gui", "show_vad_settings", true).toBool();
    ui.dockJC->setVisible(show_vad_setting);
    ui.dockJC->widget()->setEnabled(false);
}

void PlayerWindow::restore_state()
{
    QByteArray ss = m_app->get_setting("gui", WINDOW_STATE_KEY).toByteArray();
    restoreState(ss);
}

void PlayerWindow::on_actAddClip_triggered()
{
    set_state(std::make_shared<AddingClipState>(this));
}

void PlayerWindow::on_actSaveClip_triggered()
{
    m_ui_state->on_save_clip();
}

void PlayerWindow::on_actCancelClip_triggered()
{
    m_ui_state->on_cancel_clip();
}

void PlayerWindow::on_actPrevClip_triggered()
{
    if (m_clip_queue->has_prev())
    {
        m_clip_queue->set_clip_user_data(get_clip_user_data());
        m_clip_queue->save_library();

        m_clip_queue->prev();
        show_clip();
    }
}

void PlayerWindow::on_actNextClip_triggered()
{
    if (m_clip_queue->has_next())
    {
        m_clip_queue->set_clip_user_data(get_clip_user_data());
        m_clip_queue->save_library();

        m_clip_queue->next();
        show_clip();
    }
}

void PlayerWindow::on_actRemoveClip_triggered(bool value)
{
    m_clip_queue->set_removed(value);
    //m_ui_state->on_remove_clip();
}

void PlayerWindow::on_actPlayPause_triggered()
{
    if (m_video_widget->is_playing())
    {
        m_video_widget->pause();
    }
    else
    {
        if (m_video_widget->at_end())
            m_video_widget->set_time(0);
        m_video_widget->play();
    }
}

void PlayerWindow::on_actRepeatClip_triggered()
{
    m_ui_state->play();
    m_video_widget->play(get_loop_a(), get_loop_b(), 1);
}

void PlayerWindow::on_actShowWaveform_triggered(bool value)
{
    ui.waveform->setVisible(value);
    m_app->set_setting("gui", "show_waveform", value);
}

void PlayerWindow::on_player_time_changed(int time)
{
    set_slider_value(time);
    set_time(time);
    m_ui_state->on_time_changed(time);
}

void PlayerWindow::on_player_playing()
{
    ui.actPlayPause->setChecked(true);
}

void PlayerWindow::on_player_paused()
{
    ui.actPlayPause->setChecked(false);
}

void PlayerWindow::on_player_timer_triggered(int64_t t)
{
    qDebug(">>> PlayerWindow::on_player_timer_triggered: %d", t);
    m_ui_state->on_player_timer_triggered(t);
}

void PlayerWindow::on_slider_value_changed(int value)
{
    if (m_video_widget->get_time() != value)
    {
        qDebug(">>> on_slider_value_changed: %d", value);
        m_video_widget->set_time(value);
        set_time(value);
        ui.waveform->set_time(value);
        ui.waveform->repaint();
    }
}

void PlayerWindow::on_edt_loop_a_value_changed(int)
{
    int a = get_loop_a();
    int b = get_loop_b();
    ui.waveform->set_clip_a(a);
    ui.waveform->repaint();
    m_video_widget->play(a, b, 1);
}

void PlayerWindow::on_edt_loop_b_value_changed(int)
{
    int a = get_loop_a();
    int b = get_loop_b();
    ui.waveform->set_clip_b(b);
    ui.waveform->repaint();
    m_video_widget->play(std::max(a, b - 1000), b, 1);
}

void PlayerWindow::on_change_loop_shortcut_activated()
{
    QShortcut* shortcut = static_cast<QShortcut*>(sender());
    QKeySequence key = shortcut->key();
    auto it = LOOP_KEYS.find(key);
    if (it == LOOP_KEYS.end())
        return;

    const LC& lc = it.value();

    SpinBox* spinbox = lc.type == LC::Type::A ? m_edt_loop_a : m_edt_loop_b;
    int step = lc.step == LC::Step::Normal ?
        NORMAL_LOOP_STEP : PRECISE_LOOP_STEP;
    step = lc.direction == LC::Direction::Forward ? step : -step;
    
    int val = spinbox->value();
    spinbox->setValue(val + step, false);
}

void PlayerWindow::on_escape_shortcut_activated()
{
    m_video_widget->get_widget()->setFocus();
}

void PlayerWindow::on_show_subtitles_shortcut_activated(int index)
{
    m_subtitle_views[index]->set_show_once(true);
}

void PlayerWindow::on_subtitles_show_always(int index, bool show)
{
    QString mode_str;
    if (m_mode == Mode::Watching)
        mode_str = "watching";
    else if (m_mode == Mode::Repeating)
        mode_str = "repeating";
    else if (m_mode == Mode::WatchingClip)
        mode_str = "watching_clip";

    QString setting_name =
        QString("show_subtitles_%1_%2").arg(index).arg(mode_str);

    m_app->set_setting("gui", setting_name, show ? "1" : "0");
}

void PlayerWindow::on_subtitles_insert_clicked(int index, int value)
{
    const qsubs::ISubtitles* subs = m_subtitles[index].get();
    const qsubs::ICue* cue = m_cues[index];

    if (subs && cue)
    {
        int idx = cue->get_index() + value;
        const qsubs::ICue* n_cue = subs->get_cue(idx);
        if (n_cue)
        {
            QString text = m_subtitle_views[index]->get_text();
            if (value < 0)
            {
                text = n_cue->get_text() + "\n" + text;
            }
            else
            {
                text += "\n" + n_cue->get_text();
            }
            m_subtitle_views[index]->set_text(text);
            update_insert_button(index, value + (value < 0 ? -1 : 1));
        }
    }
}

void PlayerWindow::on_subs_file_changed(int view_index, int file_index)
{
    QString filename;
    if (file_index < m_subtitle_files.size())
        filename = m_subtitle_files[file_index];

    set_subtitles(view_index, filename);
    save_subs_priority();
}

void PlayerWindow::on_vad_progress_updated(int ready_chunks, int total_chunks)
{
    if (ready_chunks < total_chunks)
    {
        int percent = ((float)ready_chunks / total_chunks) * 100;
        m_lbl_info->setText(QString("VAD: %1%").arg(percent));
    }
    else
    {
        m_lbl_info->clear();
    }
}

void PlayerWindow::on_rating_changed(int)
{
    next_clip();
}

void PlayerWindow::set_slider_value(int value)
{
    ui.slider->blockSignals(true);
    ui.slider->setValue(value);
    ui.slider->blockSignals(false);
}

void PlayerWindow::set_duration(int duration)
{
    ui.slider->blockSignals(true);
    ui.slider->setMaximum(duration);
    ui.slider->blockSignals(false);
}

void PlayerWindow::set_time(int value)
{
    int l = m_video_widget->get_length();
    ui.lblTime->setText(ms_to_time(value) + " / " + ms_to_time(l));
    m_ui_state->set_time(value);
}

void PlayerWindow::set_state(std::shared_ptr<UIState> new_state)
{
    m_ui_state = new_state;
    m_ui_state->activate();
}

void PlayerWindow::show_video()
{
    QString filename = m_clip_queue->get_file_path();
    QFileInfo fileinfo(filename);

    setWindowTitle(filename);
    m_video_widget->set_file_name(filename, true);

    SubsCollection subs = m_app->get_subtitles(filename);

    QString complete_base_file_name = fileinfo.completeBaseName();
    std::vector<QString> short_sub_names;
    for (int si = 0; si < subs.files.size(); ++si)
    {
        const QString& s = subs.files[si];
        QString subs_filename = fileinfo.absolutePath() + "/" + s;
        m_subtitle_files.push_back(subs_filename);

        QFileInfo sub_info(s);
        QString subs_file_name = sub_info.fileName();
        QString short_name;
        if (subs_file_name.startsWith(complete_base_file_name))
        {
            QString suffix = subs_file_name.mid(complete_base_file_name.length());
            short_sub_names.push_back(suffix);
        }
        else
        {
            short_sub_names.push_back(s);
        }
    }

    for (int i = 0; i < NUM_SUBS_VIEWS; ++i)
    {
        QComboBox* cmb = m_subtitle_views[i]->get_combobox();
        cmb->blockSignals(true);
        for (int si = 0; si < subs.files.size(); ++si)
        {
            cmb->addItem(short_sub_names[si]);
        }
        cmb->addItem("<none>");

        int idx = subs.indices[i];
        if (idx == -1)
            idx = cmb->count() - 1;
        else
            set_subtitles(i, m_subtitle_files[idx]);

        cmb->setCurrentIndex(idx);

        cmb->blockSignals(false);
    }
    
    setup_jc_settings();

    m_audio_tools = std::make_unique<AudioTools>(filename);
    connect(m_audio_tools.get(), &AudioTools::waveform_is_ready,
        [this](WaveformPtr waveform)
        {
            m_waveform = waveform;
            ui.waveform->set_waveform(m_waveform.get());
            update_waveform_ui();
        });

    connect(m_audio_tools.get(), &AudioTools::vad_is_ready,
        [this](VADPtr vad)
        {
            m_vad = vad;
            m_vad->apply_settings(get_vad_settings());

            ui.dockJC->widget()->setEnabled(true);

            ui.waveform->set_vad(m_vad.get());
        });
    m_audio_tools->request();
    
    startTimer(25);

    m_video_widget->play();
    m_video_widget->set_rate(1.0);

    const FileUserData* file_user_data = m_clip_queue->get_file_user_data();
    if (file_user_data)
    {
        int time = file_user_data->player_time;
        if (time > 0)
            m_video_widget->set_time(time);
    }
}

void PlayerWindow::show_clip(bool clip_changed)
{
    const ClipUserData* clip_data = m_clip_queue->get_clip_user_data();
    //std::unique_ptr<IFileData> file_data = m_clip_session->get_file_data();
    if (!clip_data)
    {
        m_video_widget->stop();
        set_state(std::make_shared<VideoNotLoadedState>(this));
        QMessageBox::information(this, tr("Information"),
            tr("No clips to repeat"));
        close();
        return;
    }

    if (clip_changed)
    {
        QString filename = m_clip_queue->get_file_path();
        m_video_widget->set_file_name(filename, true);

        set_playback_rate(DEFAULT_PLAYBACK_RATE_INDEX);

        m_video_widget->play(clip_data->begin, clip_data->end, 1);

        m_edt_loop_a->setValue(clip_data->begin);
        m_edt_loop_b->setValue(clip_data->end);

        ui.actAddToFavorite->setChecked(clip_data->is_favorite);
        ui.actRemoveClip->setChecked(m_clip_queue->get_clip()->is_removed());

        for (int i = 0; i < NUM_SUBS_VIEWS; ++i)
        {
            m_subtitle_views[i]->next();
            m_subtitle_views[i]->set_text(clip_data->subtitles[i]);
        }

        ui.actPrevClip->setEnabled(m_clip_queue->has_prev());
        ui.actNextClip->setEnabled(m_clip_queue->has_next());
    }

    // TODO move from here
    if (m_mode == Mode::Repeating)
    {
        QString filename = m_clip_queue->get_file_path();
        int remains = m_clip_queue->overdue_count();
        setWindowTitle(QString("[%1] %2").arg(remains).arg(filename));

        const TodayClipStat* stat = m_clip_queue->get_today_clip_stat();
        m_lbl_clip_stats->setText(QString("Repeated today: %1").arg(stat->get_repeated_count()));

        bool is_reviewing = m_clip_queue->is_reviewing();
        m_return_shortcut->setEnabled(is_reviewing);

        m_num_repeats = 1;

        const Clip* clip = m_clip_queue->get_clip();
        const srs::ICard* card = clip->get_card();
        bool use_rating = false;
        if (card)
        {
            const srs::IModel* model = card->get_model();
            use_rating = model->use_rating();
            int rating = card->get_model()->get_default_rating(m_num_repeats);
            m_star_widget->set_rating(rating + 1);

            m_due_intervals = card->get_due_intervals(now());
            QString s = get_interval_str(m_due_intervals[rating]);
            m_lbl_info->setText("Due: " + s);

            QStringList dues_list;
            std::ranges::transform(m_due_intervals, std::back_inserter(dues_list),
                [](Duration d) {return get_interval_str(d); });
            m_star_widget->set_rating_comments(dues_list);
        }

        m_star_widget_action->setVisible(is_reviewing && use_rating);

        if (remains == 0)
        {
            m_lbl_info->setText("Clips repeated");
            if (!m_done_dialog_showed)
            {
                QMessageBox::information(this, tr("Information"), tr("You have repeated all the clips"));
                m_done_dialog_showed = true;
            }
        }
        //else
        //    m_lbl_info->clear();
    }
    else if (m_mode == Mode::WatchingClip)
    {
        QString filename = m_clip_queue->get_file_path();
        setWindowTitle(filename);
    }
    m_video_widget->get_widget()->setFocus();
}

bool PlayerWindow::remove_clip_confirmation()
{
    //int ret = QMessageBox::warning(this, tr("Confirmation"),
    //    tr("Remove clip?"), QMessageBox::Ok | QMessageBox::Cancel,
    //    QMessageBox::Cancel);

    //return ret == QMessageBox::Ok;
    return true;
}

std::unique_ptr<ClipUserData> PlayerWindow::get_clip_user_data()
{
    std::unique_ptr<ClipUserData> user_data = std::make_unique<ClipUserData>();

    user_data->begin = get_loop_a();
    user_data->end = get_loop_b();

    user_data->subtitles.resize(NUM_SUBS_VIEWS);
    for (int i = 0; i < NUM_SUBS_VIEWS; ++i)
        user_data->subtitles[i] = m_subtitle_views[i]->get_text();

    user_data->is_favorite = ui.actAddToFavorite->isChecked();
    return user_data;
}

void PlayerWindow::update_insert_button(int index, int value)
{
    const qsubs::ISubtitles* subs = m_subtitles[index].get();
    QString text;
    if (subs && m_cues[index])
    {
        int idx = m_cues[index]->get_index() + value;
        const qsubs::ICue* cue = subs->get_cue(idx);
        if (cue)
        {
            text = cue->get_text();
        }
    }
    if (value < 0)
        m_subtitle_views[index]->set_insert_left_button_tip(text);
    else if (value > 0)
        m_subtitle_views[index]->set_insert_right_button_tip(text);
}

void PlayerWindow::set_playback_rate(int index, bool play)
{
    QAbstractButton* button = m_rate_btn_group->button(index);
    button->blockSignals(true);
    button->setChecked(true);
    button->blockSignals(false);

    if (play)
    {
        m_ui_state->play();
    }

    m_playback_rate = PLAYBACK_ITEMS[index].rate;

    if (!m_jc_settings || !m_jc_settings->is_activated())
    {
        m_video_widget->set_rate(m_playback_rate);
    }
}

void PlayerWindow::on_set_playback_rate(QAbstractButton* button)
{
    int index = m_rate_btn_group->id(button);
    set_playback_rate(index, true);
}

void PlayerWindow::set_subtitles(int index, const QString& filename)
{
    if (!filename.isEmpty())
        m_subtitles[index] = qsubs::load(filename);
    else
        m_subtitles[index].reset();

    update_cue(index);
}

void PlayerWindow::update_cue(int index)
{
    int time = m_video_widget->get_time();
    const qsubs::ISubtitles* subs = m_subtitles[index].get();
    const qsubs::ICue* cue = nullptr;
    if (subs)
    {
        int offset = m_subtitle_views[index]->get_offset();
        cue = subs->pick_cue(time + offset, false);
    }

    if (cue)
    {
        if (m_cues[index] != cue)
        {
            m_subtitle_views[index]->set_text(cue->get_text());
            m_subtitle_views[index]->next();

            bool first_non_empty_subs = true;
            for (int i = 0; i < index; ++i)
                if (m_subtitles[i])
                    first_non_empty_subs = false;

            if (first_non_empty_subs)
            {
                int temp = round50(cue->get_start_time());
                qDebug("update_cue: %d", temp);
                m_edt_loop_a->setValue(round50(cue->get_start_time()));
                m_edt_loop_b->setValue(round50(cue->get_end_time()));
            }
        }
    }
    else
    {
        m_subtitle_views[index]->clear();
    }
    m_cues[index] = cue;
}

void PlayerWindow::save_subs_priority()
{
    SubsCollection subs;
    subs.indices.resize(NUM_SUBS_VIEWS);
    subs.files = m_subtitle_files;

    for (int i = 0; i < NUM_SUBS_VIEWS; ++i)
    {
        int idx = m_subtitle_views[i]->get_combobox()->currentIndex();
        if (idx >= subs.files.size())
            idx = -1;
        subs.indices[i] = idx;
    }

    QString filename = m_clip_queue->get_file_path();
    m_app->save_subtitle_priority(filename, subs);
}

int PlayerWindow::get_loop_a() const
{
    return m_edt_loop_a->value();
}

int PlayerWindow::get_loop_b() const
{
    return m_edt_loop_b->value();
}

void PlayerWindow::rewind(int delta_ms)
{
    int new_time;
    if (m_vad && m_jc_settings->is_activated() && m_jc_settings->is_non_voice_skipping())
    {
        new_time = m_vad->rewind(m_video_widget->get_time(), delta_ms);
    }
    else
    {
        new_time = m_video_widget->get_time() + delta_ms;
    }

    m_video_widget->set_time(new_time);

    // Callback on_player_time_changed doesn't work when player is paused
    if (!m_video_widget->is_playing())
    {
        set_time(new_time);
        set_slider_value(new_time);
        ui.waveform->set_time(new_time);
        ui.waveform->repaint();
    }
}

void PlayerWindow::save_clip(bool save_library)
{
    m_clip_queue->set_clip_user_data(get_clip_user_data());
    m_clip_queue->set_removed(ui.actRemoveClip->isChecked());
    if (save_library)
        m_clip_queue->save_library();
}

bool PlayerWindow::remove_clip()
{
    //if (remove_clip_confirmation())
    //{
    //    m_clip_queue->remove(true);
    //    return true;
    //}
    return false;
}

void PlayerWindow::next_clip()
{
    save_clip(false);

    if (m_clip_queue->is_reviewing())
    {
        int rating = m_star_widget->get_rating();
        m_clip_queue->repeat(rating - 1);
    }
    m_clip_queue->save_library();

    bool clip_changed = m_clip_queue->next();
    show_clip(clip_changed);
}

void PlayerWindow::jumpcutter(int t)
{
    if (!m_vad || !m_jc_settings || !m_jc_settings->is_activated())
        return;

    bool current_interval_is_loud = m_vad->is_voice(t);
    int next_interval = m_vad->next_interval(t);

    if (next_interval - t < 300)
        return;

    if (m_jc_settings->is_non_voice_skipping())
    {
        m_video_widget->set_rate(m_playback_rate);
        m_video_widget->set_volume(100);
        if (current_interval_is_loud)
        {
            m_video_widget->set_timer(next_interval);
        }
        else
        {
            m_video_widget->set_time(next_interval);
        }
    }
    else
    {
        m_video_widget->set_timer(next_interval);
        if (current_interval_is_loud)
        {
            m_video_widget->set_rate(m_playback_rate);
            m_video_widget->set_volume(100);
        }
        else
        {
            m_video_widget->set_rate(m_jc_settings->get_non_voice_speed());
            m_video_widget->set_volume(m_jc_settings->get_non_voice_volume());
        }
    }

}

void PlayerWindow::load_jc_settings()
{
    m_jc_settings = std::make_shared<JumpCutterSettings>();
    m_jc_settings->set_activated(m_app->get_setting("jumpcutter", "activated", true).toBool());
    m_jc_settings->set_voice_prob_th(m_app->get_setting("jumpcutter", "voice_prob_th", 0.5).toFloat());
    m_jc_settings->set_non_voice_volume(m_app->get_setting("jumpcutter", "non_voice_volume", 100).toFloat());
    m_jc_settings->set_non_voice_speed(m_app->get_setting("jumpcutter", "non_voice_speed", 2.0).toFloat());
    m_jc_settings->set_min_non_voice_interval(m_app->get_setting("jumpcutter", "min_non_voice_interval", 500).toInt());
    m_jc_settings->set_margin_before(m_app->get_setting("jumpcutter", "margin_before", 100).toFloat());
    m_jc_settings->set_margin_after(m_app->get_setting("jumpcutter", "margin_after", 100).toFloat());
}

void PlayerWindow::save_jc_settings()
{
    m_app->set_setting("jumpcutter", "activated", m_jc_settings->is_activated());
    m_app->set_setting("jumpcutter", "voice_prob_th", m_jc_settings->get_voice_prob_th());
    m_app->set_setting("jumpcutter", "non_voice_volume", m_jc_settings->get_non_voice_volume());
    m_app->set_setting("jumpcutter", "non_voice_speed", m_jc_settings->get_non_voice_speed());
    m_app->set_setting("jumpcutter", "min_non_voice_interval", m_jc_settings->get_min_non_voice_interval());
    m_app->set_setting("jumpcutter", "margin_before", m_jc_settings->get_margin_before());
    m_app->set_setting("jumpcutter", "margin_after", m_jc_settings->get_margin_after());
}

std::shared_ptr<VADSettings> PlayerWindow::get_vad_settings() const
{
    std::shared_ptr<VADSettings> vad_settings = std::make_shared<VADSettings>();
    vad_settings->set_min_non_voice_interval(m_jc_settings->get_min_non_voice_interval());
    vad_settings->set_margin_after(m_jc_settings->get_margin_after());
    vad_settings->set_margin_before(m_jc_settings->get_margin_before());
    vad_settings->set_voice_prob(m_jc_settings->get_voice_prob_th() * 256);
    return vad_settings;
}

void PlayerWindow::update_waveform_ui()
{
    if (m_waveform)
    {
        bool show_waveform = m_app->get_setting("gui", "show_waveform", true).toBool();
        ui.actShowWaveform->setEnabled(true);
        ui.actShowWaveform->setChecked(show_waveform);
        ui.waveform->setVisible(show_waveform);
    }
    else
    {
        ui.actShowWaveform->setEnabled(false);
        ui.actShowWaveform->setChecked(false);
        ui.waveform->setVisible(false);
    }

}

UIState::UIState(PlayerWindow* pw) : m_pw(pw)
{

}

void UIState::activate()
{
    Ui::PlayerWindow& ui = m_pw->ui;

    for (QAction* action : ui.btnPlay->actions())
        ui.btnPlay->removeAction(action);

    ui.actPlayPause->setVisible(false);
    ui.actRepeatClip->setVisible(false);

    m_pw->m_edt_loop_a_action->setVisible(false);
    m_pw->m_edt_loop_b_action->setVisible(false);

    for (int i = 0; i < NUM_SUBS_VIEWS; ++i)
    {
        m_pw->m_subtitle_views[i]->set_show_subs_files(false);
        m_pw->m_subtitle_views[i]->set_editable(false);
        m_pw->m_subtitle_views[i]->set_show_offset_buttons(false);
        m_pw->m_subtitle_views[i]->set_show_insert_buttons(false);
    }

    ui.actAddClip->setVisible(false);
    ui.actCancelClip->setVisible(false);
    ui.actNextClip->setVisible(false);
    ui.actPrevClip->setVisible(false);
    ui.actRemoveClip->setVisible(false);
    ui.actSaveClip->setVisible(false);
    ui.actAddClip->setVisible(false);

    ui.actShowWaveform->setVisible(false);
    ui.actAddToFavorite->setVisible(false);
    ui.actAddToFavorite->setChecked(false);

    ui.waveform->setVisible(false);

    m_pw->m_edt_loop_a_action->setVisible(false);
    m_pw->m_edt_loop_b_action->setVisible(false);

    m_pw->m_star_widget_action->setVisible(false);

    m_pw->m_return_shortcut->setEnabled(false);
    //QPalette p = m_pw->palette();
    //QColor bg = m_pw->m_default_bg;
    //p.setColor(QPalette::Window, bg);
    //m_pw->setPalette(p);
}

VideoNotLoadedState::VideoNotLoadedState(PlayerWindow* pw) : UIState(pw)
{
}

WatchingState::WatchingState(PlayerWindow* pw) : UIState(pw)
{
}

void WatchingState::activate()
{
    UIState::activate();

    Ui::PlayerWindow& ui = m_pw->ui;

    ui.actPlayPause->setVisible(true);
    ui.btnPlay->setDefaultAction(ui.actPlayPause);

    ui.actShowWaveform->setVisible(true);

    for (int i = 0; i < NUM_SUBS_VIEWS; ++i)
    {
        m_pw->m_subtitle_views[i]->set_show_subs_files(true);
        m_pw->m_subtitle_views[i]->set_show_offset_buttons(true);

        QVariant v = m_pw->m_app->get_setting("gui",
            QString(SHOW_SUBTITLES_KEY).arg(i).arg("watching"));
        m_pw->m_subtitle_views[i]->set_show_always(v.isNull() ? true : v.toBool());
    }

    ui.actAddClip->setVisible(true);

    ui.dockJC->widget()->setEnabled(true);

    if (m_pw->m_waveform)
        ui.waveform->set_clip_mode(false);

    m_pw->update_waveform_ui();

    const TodayClipStat* stat = m_pw->m_clip_queue->get_today_clip_stat();
    QString lbl = QString("Added today: %1 (%2)")
        .arg(stat->get_added_count_for_file())
        .arg(stat->get_added_count());
    m_pw->m_lbl_clip_stats->setText(lbl);
}

void WatchingState::on_close()
{
    m_pw->save_player_time();
    m_pw->save_jc_settings();
    m_pw->m_app->set_setting("gui", "show_vad_settings", m_pw->ui.dockJC->isVisible());
}

void WatchingState::on_time_changed(int time)
{
    m_pw->jumpcutter(time);
}

void WatchingState::on_player_timer_triggered(int time)
{
    m_pw->jumpcutter(time);
}

void WatchingState::set_time(int)
{
    m_pw->update_cue(0);
    m_pw->update_cue(1);
}

void WatchingState::on_waveform_mouse_release(int time, QMouseEvent*)
{
    m_pw->m_video_widget->set_time(time);
    if (!m_pw->m_video_widget->is_playing())
    {
        m_pw->set_slider_value(time);
        m_pw->set_time(time);
        m_pw->ui.waveform->set_time(time);
        m_pw->ui.waveform->repaint();
    }
}

void WatchingState::play()
{
    if (m_pw->m_video_widget->at_end())
        m_pw->m_video_widget->set_time(0);
    m_pw->m_video_widget->play();
}


ClipState::ClipState(PlayerWindow* pw) : UIState(pw)
{
}

void ClipState::activate()
{
    UIState::activate();
}

AddingClipState::AddingClipState(PlayerWindow* pw) : ClipState(pw)
{
}

void AddingClipState::activate()
{
    ClipState::activate();

    Ui::PlayerWindow& ui = m_pw->ui;

    m_pw->m_jc_settings->set_activated(false);
    m_pw->set_playback_rate(DEFAULT_PLAYBACK_RATE_INDEX);

    ui.actRepeatClip->setVisible(true);
    ui.btnPlay->setDefaultAction(ui.actRepeatClip);

    ui.actSaveClip->setVisible(true);
    ui.actCancelClip->setVisible(true);

    ui.actAddToFavorite->setVisible(true);

    ui.dockJC->widget()->setEnabled(false);

    m_pw->m_edt_loop_a_action->setVisible(true);
    m_pw->m_edt_loop_b_action->setVisible(true);

    for (int i = 0; i < NUM_SUBS_VIEWS; ++i)
    {
        m_pw->m_subtitle_views[i]->set_editable(true);
        m_pw->m_subtitle_views[i]->set_show_insert_buttons(true);
    }

    //QPalette p = m_pw->palette();
    //QColor bg = 0xFFCDD2;
    //p.setColor(QPalette::Window, bg);
    //m_pw->setPalette(p);

    for (int i = 0; i < NUM_SUBS_VIEWS; ++i)
    {
        m_pw->m_subtitle_views[i]->reset_insert_counters();
        m_pw->update_insert_button(i, -1);
        m_pw->update_insert_button(i, 1);
    }

    int time = m_pw->m_video_widget->get_time();

    int a = std::max(0, time - 1000);
    int b = std::min(m_pw->m_video_widget->get_length(), time + 1000);
    for (const qsubs::ICue* cue : m_pw->m_cues)
    {
        if (cue)
        {
            a = cue->get_start_time();
            b = cue->get_end_time();
            break;
        }
    }

    ui.waveform->set_clip_mode(true);
    ui.waveform->set_clip_a(a);
    ui.waveform->set_clip_b(b);
    ui.waveform->setVisible(true);

    //if (m_pw->m_jc)
    //    m_pw->m_jc->set_enabled(false);

    m_pw->m_edt_loop_a->setValue(a);
    m_pw->m_edt_loop_b->setValue(b);
    m_pw->m_video_widget->play(a, b, 1);
}

void AddingClipState::play()
{
    m_pw->m_video_widget->play(m_pw->get_loop_a(), m_pw->get_loop_b(), 1);
}

void AddingClipState::on_save_clip()
{
    m_pw->save_clip(true);

    m_pw->m_video_widget->set_time(m_pw->get_loop_a());
    m_pw->m_video_widget->play();

    for (int i = 0; i < NUM_SUBS_VIEWS; ++i)
        m_pw->m_subtitle_views[i]->next();

    m_pw->set_state(std::make_shared<WatchingState>(m_pw));
}

void AddingClipState::on_cancel_clip()
{
    m_pw->m_video_widget->play();

    for (int i = 0; i < NUM_SUBS_VIEWS; ++i)
        m_pw->m_subtitle_views[i]->next();

    m_pw->set_state(std::make_shared<WatchingState>(m_pw));
}

void AddingClipState::on_player_timer_triggered(int)
{
    m_pw->m_video_widget->pause();
}

void AddingClipState::on_waveform_mouse_release(int time, QMouseEvent* event)
{
    int a = m_pw->get_loop_a();
    int b = m_pw->get_loop_b();

    if (event->button() == Qt::MiddleButton)
    {
        m_pw->m_video_widget->play(a, b, 1);
        return;
    }

    //SpinBox* sb;
    //if (time < (a + b) / 2)
    //    sb = m_pw->m_edt_loop_a;
    //else
    //    sb = m_pw->m_edt_loop_b;

    //int cur = sb->value();

    //if (event->button() == Qt::LeftButton)
    //{
    //    sb->setValue(time, false);
    //}
    //else if (event->button() == Qt::RightButton)
    //{
    //    int delta = (time < cur) ? -20 : 20;
    //    sb->setValue(cur + delta, false);
    //}

    if (event->button() == Qt::LeftButton)
    {
        if (time < b)
            m_pw->m_edt_loop_a->setValue(time, false);
    }
    else if (event->button() == Qt::RightButton)
    {
        if (time > a)
            m_pw->m_edt_loop_b->setValue(time, false);
    }
}

void AddingClipState::on_wheel_event(int time, QWheelEvent* event)
{
    int a = m_pw->get_loop_a();
    int b = m_pw->get_loop_b();
    int delta = event->angleDelta().y() > 0 ? -20 : 20;
    if (time < (a + b) / 2)
    {
        int new_time = a + delta;
        if (new_time < b)
            m_pw->m_edt_loop_a->setValue(new_time, false);
    }
    else
    {
        int new_time = b + delta;
        if (new_time > a)
            m_pw->m_edt_loop_b->setValue(new_time, false);
    }
}

WatchingClipState::WatchingClipState(PlayerWindow* pw) : ClipState(pw)
{
}

void WatchingClipState::activate()
{
    ClipState::activate();

    Ui::PlayerWindow& ui = m_pw->ui;

    ui.actRepeatClip->setVisible(true);
    ui.btnPlay->setDefaultAction(ui.actRepeatClip);

    //ui.actSaveClip->setVisible(true);

    ui.actAddToFavorite->setVisible(true);
    ui.actRemoveClip->setVisible(true);

    ui.dockJC->setVisible(false);
    ui.dockJC->toggleViewAction()->setEnabled(false);

    m_pw->m_edt_loop_a_action->setVisible(true);
    m_pw->m_edt_loop_b_action->setVisible(true);

    ui.waveform->setVisible(false);

    for (int i = 0; i < NUM_SUBS_VIEWS; ++i)
    {
        m_pw->m_subtitle_views[i]->set_editable(true);
        QVariant v = m_pw->m_app->get_setting("gui",
            QString(SHOW_SUBTITLES_KEY).arg(i).arg("watching_clip"));
        m_pw->m_subtitle_views[i]->set_show_always(v.isNull() ? true : v.toBool());
    }
}

void WatchingClipState::play()
{
    m_pw->m_video_widget->play(m_pw->get_loop_a(), m_pw->get_loop_b(), 1);
}

void WatchingClipState::on_close()
{
    m_pw->save_clip(true);
}

void WatchingClipState::on_save_clip()
{
    m_pw->save_clip(true);
    m_pw->close();
}

void WatchingClipState::on_cancel_clip()
{
    m_pw->close();
}

void WatchingClipState::on_remove_clip()
{
    if (m_pw->remove_clip())
    {
        m_pw->close();
    }
}

void WatchingClipState::on_player_timer_triggered(int)
{
    m_pw->m_video_widget->pause();
}

RepeatingClipState::RepeatingClipState(PlayerWindow* pw) : ClipState(pw)
{
}

void RepeatingClipState::activate()
{
    ClipState::activate();

    Ui::PlayerWindow& ui = m_pw->ui;
    m_pw->set_playback_rate(DEFAULT_PLAYBACK_RATE_INDEX);

    ui.actRepeatClip->setVisible(true);
    ui.btnPlay->setDefaultAction(ui.actRepeatClip);

    ui.actRemoveClip->setVisible(true);
    ui.actPrevClip->setVisible(true);
    ui.actNextClip->setVisible(true);

    ui.actAddToFavorite->setVisible(true);

    ui.dockJC->setVisible(false);
    ui.dockJC->toggleViewAction()->setEnabled(false);

    m_pw->m_edt_loop_a_action->setVisible(true);
    m_pw->m_edt_loop_b_action->setVisible(true);

    m_pw->m_star_widget_action->setVisible(true);

    m_pw->m_video_widget->get_widget()->setFocus();
    
    for (int i = 0; i < NUM_SUBS_VIEWS; ++i)
    {
        m_pw->m_subtitle_views[i]->set_editable(true);

        QVariant v = m_pw->m_app->get_setting("gui",
            QString(SHOW_SUBTITLES_KEY).arg(i).arg("repeating"));
        m_pw->m_subtitle_views[i]->set_show_always(v.isNull() ? true : v.toBool());
    }

    m_pw->m_return_shortcut->setEnabled(true);
}

void RepeatingClipState::play()
{
    const Clip* clip = m_pw->m_clip_queue->get_clip();
    if (!clip)
        return;

    const srs::ICard* card = clip->get_card();
    if (!card)
        return;

    ++m_pw->m_num_repeats;
    int rating = card->get_model()->get_default_rating(m_pw->m_num_repeats);
    m_pw->m_star_widget->set_rating(rating + 1);

    QString s = get_interval_str(m_pw->m_due_intervals[rating]);
    m_pw->m_lbl_info->setText("Due: " + s);

    QStringList dues_list;
    std::ranges::transform(m_pw->m_due_intervals, std::back_inserter(dues_list),
        [](Duration d) {return get_interval_str(d); });
    m_pw->m_star_widget->set_rating_comments(dues_list);
}


void RepeatingClipState::on_close()
{
    m_pw->save_clip(true);
}

void RepeatingClipState::on_next_clip()
{
    m_pw->next_clip();
}

void RepeatingClipState::on_remove_clip()
{
    if (m_pw->remove_clip())
    {
        m_pw->show_clip();
    }
}

void RepeatingClipState::on_player_timer_triggered(int)
{
    m_pw->m_video_widget->pause();
}

