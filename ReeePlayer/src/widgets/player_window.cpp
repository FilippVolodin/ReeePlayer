#include "pch.h"
#include "player_window.h"
#include <qsubtitles.h>
#include "models/repetition_model.h"
#include "models/session.h"
#include "models/clip_storage.h"
#include "models/library.h"
#include "models/app.h"
#include "spinbox.h"
#include "models/jumpcutter.h"
#include "models/vad.h"
#include "waveform.h"
#include "jumpcutter_settings_dialog.h"

constexpr float PLAYBACK_RATES[] = { 0.5, 0.75, 1.0, 1.25, 1.5, 2.0 };
static const QKeySequence PLAYBACK_KEYS[] =
    { Qt::Key_3, Qt::Key_4, Qt::Key_5, Qt::Key_6, Qt::Key_7, Qt::Key_8 };
constexpr int DEFAULT_PLAYBACK_RATE_INDEX = 2;

constexpr int NORMAL_LOOP_STEP = 100;
constexpr int PRECISE_LOOP_STEP = 40;

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

    ui.videoWidget->init(app->get_vlc_instance());

    setup_actions();
    setup_player_events();
    setup_subs_views();
    setup_slider();
    setup_shortcuts();
    setup_playback_rates();
    restore_state();

    ui.dockWidget1->toggleViewAction()->setText("Show/hide subtitles 1");
    ui.dockWidget2->toggleViewAction()->setText("Show/hide subtitles 2");

    m_default_bg = palette().color(QPalette::Window);
    m_library = m_app->get_library();

    for (const qsubs::ICue*& cue : m_cues)
        cue = nullptr;
}

PlayerWindow::~PlayerWindow()
{
}

QMenu* PlayerWindow::createPopupMenu()
{
    QMenu* menu = new QMenu(this);
    menu->addAction(ui.dockWidget1->toggleViewAction());
    menu->addAction(ui.dockWidget2->toggleViewAction());
    return menu;
}

void PlayerWindow::watch(File* file)
{
    m_mode = Mode::Watching;
    m_file = file;
    show();
}

void PlayerWindow::watch_clip(Clip* clip)
{
    m_mode = Mode::WatchingClip;
    m_clip = clip;
    show();
}

void PlayerWindow::repeat(std::vector<File*>&& files)
{
    m_session = std::make_shared<Session>(m_app->get_library(), files);
    m_mode = Mode::Repeating;
    show();
}

void PlayerWindow::set_vad(std::shared_ptr<VAD> vad)
{
    m_vad = vad;
    connect(m_vad.get(), &VAD::progress_updated, this, &PlayerWindow::on_vad_progress_updated);
}

void PlayerWindow::save_player_time()
{
    int time = ui.videoWidget->get_time();
    m_file->set_player_time(time);
    m_file->set_length(ui.videoWidget->get_length());
    m_library->save();
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
        m_clip = m_session->get_next_clip();
        show_clip();
    }
}

void PlayerWindow::closeEvent(QCloseEvent * event)
{
    m_ui_state->on_close();

    m_app->set_setting("gui", WINDOW_STATE_KEY, saveState());

    QWidget::closeEvent(event);
}

void PlayerWindow::timerEvent(QTimerEvent* event)
{
    if (ui.videoWidget->is_playing())
    {
        int time = ui.videoWidget->get_accuracy_time();
        //jumpcutter(time);
        //qDebug("EEEEE %d", time);
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

    ui.toolBar->addAction(ui.actJumpCutter);
    connect(ui.actJumpCutter, &QAction::triggered,
        this, &PlayerWindow::on_actJumpCutter_triggered);

    ui.toolBar->addAction(ui.actJumpCutterSettings);
    connect(ui.actJumpCutterSettings, &QAction::triggered,
        this, &PlayerWindow::on_actJumpCutterSettings_triggered);

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

    ui.toolBar->addAction(ui.actRemoveClip);
    connect(ui.actRemoveClip, &QAction::triggered,
        this, &PlayerWindow::on_actRemoveClip_triggered);

    connect(ui.actPlayPause, &QAction::triggered,
        this, &PlayerWindow::on_actPlayPause_triggered);

    connect(ui.actRepeatClip, &QAction::triggered,
        this, &PlayerWindow::on_actRepeatClip_triggered);

    connect(ui.waveform, &Waveform::mouse_release, [this](int time, QMouseEvent* event) {
        m_ui_state->on_waveform_mouse_release(time, event); });

    connect(ui.waveform, &Waveform::wheel_event, [this](int time, QWheelEvent* event) {
        m_ui_state->on_wheel_event(time, event); });
};

void PlayerWindow::setup_player_events()
{
    connect(ui.videoWidget, &VideoWidget::time_changed,
        this, &PlayerWindow::on_player_time_changed, Qt::QueuedConnection);
    connect(ui.videoWidget, &VideoWidget::playing,
        this, &PlayerWindow::on_player_playing);
    connect(ui.videoWidget, &VideoWidget::paused,
        this, &PlayerWindow::on_player_paused);
    connect(ui.videoWidget, &VideoWidget::length_changed,
        this, &PlayerWindow::set_duration);
    connect(ui.videoWidget, &VideoWidget::timer_triggered,
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
    for (float r : PLAYBACK_RATES)
    {
        QShortcut* shortcut = new QShortcut(PLAYBACK_KEYS[id], this);
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
    for (float r : PLAYBACK_RATES)
    {
        QPushButton* b = new QPushButton(this);
        b->setText(QString::asprintf("%.2gx", r));
        b->setCheckable(true);
        b->setMaximumWidth(40);
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
    if (m_session->has_prev_clip())
    {
        update_clip_interval(m_clip);
        update_clip_subtitles(m_clip);
        m_library->save();

        ui.videoWidget->setFocus();

        m_clip = m_session->get_prev_clip();
        show_clip();
    }
}

void PlayerWindow::on_actNextClip_triggered()
{
    update_clip_interval(m_clip);
    update_clip_subtitles(m_clip);
    m_library->save();

    m_clip->set_level(m_next_level);
    m_clip->set_rep_time(now());
    m_library->save();

    ui.videoWidget->setFocus();

    m_clip = m_session->get_next_clip();
    show_clip();
}

void PlayerWindow::on_actRemoveClip_triggered()
{
    m_ui_state->on_remove_clip();
}

void PlayerWindow::on_actPlayPause_triggered()
{
    if (ui.videoWidget->is_playing())
    {
        ui.videoWidget->pause();
    }
    else
    {
        if (ui.videoWidget->at_end())
            ui.videoWidget->set_time(0);
        ui.videoWidget->play();
    }
}

void PlayerWindow::on_actRepeatClip_triggered()
{
    ui.videoWidget->play(get_loop_a(), get_loop_b(), 1);
}

void PlayerWindow::on_actJumpCutterSettings_triggered()
{
    JumpCutterSettingsDialog jc_dialog;
    jc_dialog.set_settings(m_jc->get_settings());
    if (jc_dialog.exec() == QDialog::Accepted)
    {
        auto settings = jc_dialog.get_settings();
        m_jc->apply_settings(settings);
    }
}

void PlayerWindow::on_actShowWaveform_triggered(bool value)
{
    ui.waveform->setVisible(value);
    m_app->set_setting("gui", "show_waveform", value);
}

void PlayerWindow::on_actJumpCutter_triggered(bool value)
{
    m_jc->set_enabled(value);
    m_app->set_setting("jumpcutter", "enabled", value);
}

void PlayerWindow::on_btnMinus_clicked(bool)
{
    m_next_level -= 1.0;
    refresh_clip_info();
}

void PlayerWindow::on_btnPlus_clicked(bool)
{
    m_next_level += 1.0;
    refresh_clip_info();
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
    if (ui.videoWidget->get_time() != value)
    {
        qDebug(">>> on_slider_value_changed: %d", value);
        ui.videoWidget->set_time(value);
        set_time(value);
        ui.waveform->set_time(value);
        ui.waveform->repaint();
    }
}

void PlayerWindow::on_edt_loop_a_value_changed(int)
{
    int a = get_loop_a();
    int b = get_loop_b();
    if (m_jc)
        ui.waveform->set_clip_a(a);
    ui.videoWidget->play(a, b, 1);
}

void PlayerWindow::on_edt_loop_b_value_changed(int)
{
    int a = get_loop_a();
    int b = get_loop_b();
    if (m_jc)
        ui.waveform->set_clip_b(b);
    ui.videoWidget->play(std::max(a, b - 1000), b, 1);
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
    ui.videoWidget->setFocus();
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

    if (subs != nullptr && cue != nullptr)
    {
        int idx = cue->get_index() + value;
        const qsubs::ICue* n_cue = subs->get_cue(idx);
        if (n_cue != nullptr)
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
        statusBar()->showMessage(QString("VAD: %1%").arg(percent));
    }
    else
        statusBar()->clearMessage();
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
    int l = ui.videoWidget->get_length();
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
    QString filename = m_file->get_path();
    QFileInfo fileinfo(filename);

    setWindowTitle(filename);
    ui.videoWidget->set_file_name(filename, true);

    SubsCollection subs = m_app->get_subtitles(filename);

    QString complete_base_file_name = fileinfo.completeBaseName();
    std::vector<QString> short_sub_names;
    for (int si = 0; si < subs.files.size(); ++si)
    {
        const QString& s = subs.files[si];
        QString filename = fileinfo.absolutePath() + "/" + s;
        m_subtitle_files.push_back(filename);

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

    try
    {
        m_jc = std::make_shared<JumpCutter>(get_vol_file(filename));
        ui.waveform->set_jumpcutter(m_jc.get());
    }
    catch(std::exception&)
    {
    }
    ui.waveform->set_vad(m_vad.get());

    startTimer(20);

    ui.videoWidget->play();
    ui.videoWidget->set_rate(1.0);

    int time = m_file->get_player_time();
    if (time > 0)
        ui.videoWidget->set_time(time);
}

void PlayerWindow::show_clip()
{

    if (m_clip == nullptr)
    {
        ui.videoWidget->stop();
        set_state(std::make_shared<VideoNotLoadedState>(this));
        QMessageBox::information(this, tr("Information"),
            tr("No clips to repeat"));
        return;
    }

    File* file = m_clip->get_file();

    QString filename = file->get_path();

    if (file != m_file)
    {
        m_file = file;
        ui.videoWidget->set_file_name(filename, true);
    }

    set_playback_rate(DEFAULT_PLAYBACK_RATE_INDEX);

    ui.videoWidget->play(m_clip->get_begin(), m_clip->get_end(), 1);

    m_edt_loop_a->setValue(m_clip->get_begin());
    m_edt_loop_b->setValue(m_clip->get_end());

    for (int i = 0; i < NUM_SUBS_VIEWS; ++i)
    {
        m_subtitle_views[i]->next();
        m_subtitle_views[i]->set_text(m_clip->get_subtitle(i));
    }

    time_t elapsed = now() - m_clip->get_rep_time();
    m_next_level = get_next_level(elapsed, m_clip->get_level());

    refresh_clip_info();

    if (m_session)
    {
        int remains = m_session->remain_clips();
        setWindowTitle(QString("[%1] %2").arg(remains).arg(filename));

        if (remains == 0)
            statusBar()->showMessage("Clips repeated");
        else
            statusBar()->clearMessage();

        ui.actPrevClip->setEnabled(m_session->has_prev_clip());
    }
}

void PlayerWindow::refresh_clip_info()
{
    QString info = QString::asprintf("%f\n%f (%+f)",
        m_clip->get_level(), m_next_level, m_next_level - m_clip->get_level());

    time_t best_rep_interval = get_repetititon_interval(m_clip->get_level()).begin;
    time_t next_rep_interval = get_repetititon_interval(m_next_level).begin;

    time_t elapsed = now() - m_clip->get_rep_time();
    time_t remain = best_rep_interval - elapsed;

    QString level_str = QString::asprintf("%f", m_clip->get_level());
    QString next_level_str = QString::asprintf("%f", m_next_level);
    QString diff_str = QString::asprintf("%+f", m_next_level - m_clip->get_level());
    QString last_rep_str = get_interval_str(-elapsed);
    QString best_rep_str = m_clip->get_level() > 0.001 ? get_interval_str(remain) : "";
    QString next_rep_str = get_interval_str(next_rep_interval);
}

bool PlayerWindow::remove_clip_confirmation()
{
    int ret = QMessageBox::warning(this, tr("Confirmation"),
        tr("Remove clip?"), QMessageBox::Ok | QMessageBox::Cancel,
        QMessageBox::Cancel);

    return ret == QMessageBox::Ok;
}

void PlayerWindow::update_clip_interval(Clip* clip)
{
    clip->set_begin(get_loop_a());
    clip->set_end(get_loop_b());
}

void PlayerWindow::update_clip_subtitles(Clip* clip)
{
    std::vector<QString> subs;
    subs.reserve(NUM_SUBS_VIEWS);
    for (int i = 0; i < NUM_SUBS_VIEWS; ++i)
        subs.push_back(m_subtitle_views[i]->get_text());
    clip->set_subtitles(std::move(subs));
}

void PlayerWindow::update_insert_button(int index, int value)
{
    const qsubs::ISubtitles* subs = m_subtitles[index].get();
    QString text;
    if (subs != nullptr && m_cues[index] != nullptr)
    {
        int idx = m_cues[index]->get_index() + value;
        const qsubs::ICue* cue = subs->get_cue(idx);
        if (cue != nullptr)
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

    m_playback_rate = PLAYBACK_RATES[index];

    if (!m_jc || !m_jc->is_enabled())
    {
        ui.videoWidget->set_rate(m_playback_rate);
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
    int time = ui.videoWidget->get_time();
    const qsubs::ISubtitles* subs = m_subtitles[index].get();
    const qsubs::ICue* cue = nullptr;
    if (subs != nullptr)
    {
        int offset = m_subtitle_views[index]->get_offset();
        cue = subs->pick_cue(time + offset, false);
    }

    if (cue != nullptr)
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
    m_app->save_subtitle_priority(m_file->get_path(), subs);
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
    if (m_vad)
    {
        new_time = m_vad->rewind(ui.videoWidget->get_time(), delta_ms);
    }
    else
    {
        new_time = ui.videoWidget->get_time() + delta_ms;
    }

    ui.videoWidget->set_time(new_time);

    // Callback on_player_time_changed doesn't work when player is paused
    if (!ui.videoWidget->is_playing())
    {
        set_time(new_time);
        set_slider_value(new_time);
        ui.waveform->set_time(new_time);
        ui.waveform->repaint();
    }
}

void PlayerWindow::save_new_clip()
{
    Clip* new_clip = new Clip();
    update_clip_interval(new_clip);
    update_clip_subtitles(new_clip);
    m_file->add_clip(new_clip);
    m_library->save();
}

void PlayerWindow::save_current_clip()
{
    update_clip_interval(m_clip);
    update_clip_subtitles(m_clip);
    m_library->save();
}

bool PlayerWindow::remove_clip()
{
    if (remove_clip_confirmation())
    {
        m_file->remove_clip(m_clip);
        m_library->save();
        return true;
    }

    return false;
}

void PlayerWindow::next_clip()
{
    m_clip = m_session->get_next_clip();
    show_clip();
}

void PlayerWindow::jumpcutter(int t)
{
    if (!m_jc || !m_jc->is_enabled())
        return;

    bool current_interval_is_loud = m_vad->is_voice(t);
    int next_interval = m_vad->next_interval(t);

    if (next_interval - t < 300)
        return;

    qDebug("JC: %d %d %d", t, current_interval_is_loud, next_interval);

    if (m_jc->get_settings()->is_silence_skipping())
    {
        ui.videoWidget->set_rate(m_playback_rate);
        if (current_interval_is_loud)
        {
            ui.videoWidget->set_timer(next_interval);
        }
        else
        {
            ui.videoWidget->set_time(next_interval);
        }
    }
    else
    {
        ui.videoWidget->set_timer(next_interval);
        if (current_interval_is_loud)
        {
            ui.videoWidget->set_rate(m_playback_rate);
        }
        else
        {
            ui.videoWidget->set_rate(m_jc->get_settings()->get_silence_speed());
        }
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
    ui.actJumpCutter->setVisible(false);
    ui.actJumpCutterSettings->setVisible(false);

    ui.waveform->setVisible(false);

    m_pw->m_edt_loop_a_action->setVisible(false);
    m_pw->m_edt_loop_b_action->setVisible(false);

    QPalette p = m_pw->palette();
    QColor bg = m_pw->m_default_bg;
    p.setColor(QPalette::Window, bg);
    m_pw->setPalette(p);
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
    ui.actJumpCutter->setVisible(true);
    ui.actJumpCutterSettings->setVisible(true);

    for (int i = 0; i < NUM_SUBS_VIEWS; ++i)
    {
        m_pw->m_subtitle_views[i]->set_show_subs_files(true);
        m_pw->m_subtitle_views[i]->set_show_offset_buttons(true);

        QVariant v = m_pw->m_app->get_setting("gui",
            QString(SHOW_SUBTITLES_KEY).arg(i).arg("watching"));
        m_pw->m_subtitle_views[i]->set_show_always(v.isNull() ? true : v.toBool());
    }

    ui.actAddClip->setVisible(true);

    if (m_pw->m_jc)
    {
        bool show_waveform = m_pw->m_app->get_setting("gui", "show_waveform", true).toBool();
        ui.actShowWaveform->setChecked(show_waveform);
        ui.waveform->setVisible(show_waveform);
        ui.waveform->set_clip_mode(false);

        bool jc_enabled = m_pw->m_app->get_setting("jumpcutter", "enabled", true).toBool();
        ui.actJumpCutter->setChecked(jc_enabled);
        m_pw->m_jc->set_enabled(jc_enabled);
    }
    else
    {
        ui.actShowWaveform->setEnabled(false);
        ui.actShowWaveform->setChecked(false);

        ui.actJumpCutter->setEnabled(false);
        ui.actJumpCutter->setChecked(false);

        ui.actJumpCutterSettings->setEnabled(false);

        ui.waveform->setVisible(false);
    }

    //QPalette p = m_pw->palette();
    //QColor bg = m_pw->m_default_bg;
    //p.setColor(QPalette::Window, bg);
    //m_pw->setPalette(p);
}

void WatchingState::on_close()
{
    m_pw->save_player_time();
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
    m_pw->ui.videoWidget->set_time(time);
    if (!m_pw->ui.videoWidget->is_playing())
    {
        m_pw->set_slider_value(time);
        m_pw->set_time(time);
        m_pw->ui.waveform->set_time(time);
        m_pw->ui.waveform->repaint();
    }
}

void WatchingState::play()
{
    if (m_pw->ui.videoWidget->at_end())
        m_pw->ui.videoWidget->set_time(0);
    m_pw->ui.videoWidget->play();
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

    m_pw->set_playback_rate(DEFAULT_PLAYBACK_RATE_INDEX);

    ui.actRepeatClip->setVisible(true);
    ui.btnPlay->setDefaultAction(ui.actRepeatClip);

    ui.actSaveClip->setVisible(true);
    ui.actCancelClip->setVisible(true);

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

    int time = ui.videoWidget->get_time();

    int a = std::max(0, time - 1000);
    int b = std::min(ui.videoWidget->get_length(), time + 1000);
    for (const qsubs::ICue* cue : m_pw->m_cues)
    {
        if (cue != nullptr)
        {
            a = cue->get_start_time();
            b = cue->get_end_time();
            break;
        }
    }

    if (m_pw->m_jc)
    {
        ui.waveform->set_clip_mode(true);
        ui.waveform->set_clip_a(a);
        ui.waveform->set_clip_b(b);
        ui.waveform->setVisible(true);
    }

    if (m_pw->m_jc)
    m_pw->m_jc->set_enabled(false);

    m_pw->m_edt_loop_a->setValue(a);
    m_pw->m_edt_loop_b->setValue(b);
    ui.videoWidget->play(a, b, 1);
}

void AddingClipState::play()
{
    m_pw->ui.videoWidget->play(m_pw->get_loop_a(), m_pw->get_loop_b(), 1);
}

void AddingClipState::on_save_clip()
{
    m_pw->save_new_clip();

    Ui::PlayerWindow& ui = m_pw->ui;
    ui.videoWidget->set_time(m_pw->get_loop_a());
    ui.videoWidget->play();

    for (int i = 0; i < NUM_SUBS_VIEWS; ++i)
        m_pw->m_subtitle_views[i]->next();

    m_pw->set_state(std::make_shared<WatchingState>(m_pw));
}

void AddingClipState::on_cancel_clip()
{
    m_pw->ui.videoWidget->play();

    for (int i = 0; i < NUM_SUBS_VIEWS; ++i)
        m_pw->m_subtitle_views[i]->next();

    m_pw->set_state(std::make_shared<WatchingState>(m_pw));
}

void AddingClipState::on_player_timer_triggered(int)
{
    m_pw->ui.videoWidget->pause();
}

void AddingClipState::on_waveform_mouse_release(int time, QMouseEvent* event)
{
    int a = m_pw->get_loop_a();
    int b = m_pw->get_loop_b();

    if (event->button() == Qt::MiddleButton)
    {
        m_pw->ui.videoWidget->play(a, b, 1);
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
    SpinBox* sb;
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

    ui.actSaveClip->setVisible(true);
    ui.actCancelClip->setVisible(true);

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

    QPalette p = m_pw->palette();
    QColor bg = m_pw->m_default_bg;
    p.setColor(QPalette::Window, bg);
    m_pw->setPalette(p);
}

void WatchingClipState::play()
{
    m_pw->ui.videoWidget->play(m_pw->get_loop_a(), m_pw->get_loop_b(), 1);
}

void WatchingClipState::on_save_clip()
{
    m_pw->save_current_clip();
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
    m_pw->ui.videoWidget->pause();
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

    m_pw->m_edt_loop_a_action->setVisible(true);
    m_pw->m_edt_loop_b_action->setVisible(true);

    for (int i = 0; i < NUM_SUBS_VIEWS; ++i)
    {
        m_pw->m_subtitle_views[i]->set_editable(true);

        QVariant v = m_pw->m_app->get_setting("gui",
            QString(SHOW_SUBTITLES_KEY).arg(i).arg("repeating"));
        m_pw->m_subtitle_views[i]->set_show_always(v.isNull() ? true : v.toBool());
    }

    QPalette p = m_pw->palette();
    QColor bg = m_pw->m_default_bg;
    p.setColor(QPalette::Window, bg);
    m_pw->setPalette(p);
}

void RepeatingClipState::play()
{
    m_pw->ui.videoWidget->play(m_pw->get_loop_a(), m_pw->get_loop_b(), 1);
}

void RepeatingClipState::on_remove_clip()
{
    if (m_pw->remove_clip())
    {
        m_pw->next_clip();
    }
}

void RepeatingClipState::on_player_timer_triggered(int)
{
    m_pw->ui.videoWidget->pause();
}

