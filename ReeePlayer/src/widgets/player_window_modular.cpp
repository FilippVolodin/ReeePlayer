#include "player_window_modular.h"
#include <app.h>

#include <video_module.h>
#include <playback_control_module.h>
#include <subtitles_module.h>
#include <clip_module.h>

#include <playback_mediator.h>
#include <clip_mediator.h>
#include <session.h>
#include <clip_storage.h>

PlayerWindowModular::PlayerWindowModular(App* app, QWidget* parent)
    : QMainWindow(parent), m_app(app)
{
    ui.setupUi(this);
    ui.waveform->setVisible(false);
}

void PlayerWindowModular::run(Mode mode, std::shared_ptr<IClipQueue> clip_queue)
{
    m_mode = mode;
    m_clip_queue = clip_queue;
    //QWidget* centralwidget = new QWidget(this);
    //centralwidget->setObjectName("centralwidget");
    //this->setCentralWidget(centralwidget);
    //QVBoxLayout* verticalLayout = new QVBoxLayout(centralwidget);

    m_mode_mediator = std::make_unique<ModeMediator>();
    m_playback_mediator = std::make_unique<PlaybackMediator>();
    m_clip_mediator = std::make_unique<ClipMediator>();
    
    m_video_module = std::make_unique<VideoModule>(m_app, m_playback_mediator.get());
    m_playback_module = std::make_unique<PlaybackControlModule>(m_app, m_playback_mediator.get());
    m_subtitles_modules[0] = std::make_unique<SubtitlesModule>(0, m_app, m_mode_mediator.get(), m_playback_mediator.get());
    m_subtitles_modules[1] = std::make_unique<SubtitlesModule>(1, m_app, m_mode_mediator.get(), m_playback_mediator.get());
    m_clip_module = std::make_unique<ClipModule>(m_app, m_mode_mediator.get(), m_playback_mediator.get());

    m_clip_mediator->add_unit(m_subtitles_modules[0].get());
    m_clip_mediator->add_unit(m_subtitles_modules[1].get());
    m_clip_mediator->add_unit(m_clip_module.get());

    m_video_module->setup_player(&ui);
    m_playback_module->setup_player(&ui);
    m_subtitles_modules[0]->setup_player(&ui);
    m_subtitles_modules[1]->setup_player(&ui);
    m_clip_module->setup_player(&ui);

    connect(m_mode_mediator.get(), &ModeMediator::mode_changed,
        this, &PlayerWindowModular::set_mode);

    ui.toolBar->addAction(ui.actAddClip);
    connect(ui.actAddClip, &QAction::triggered,
        this, &PlayerWindowModular::add_new_clip_activated);

    ui.toolBar->addAction(ui.actSaveClip);
    connect(ui.actSaveClip, &QAction::triggered,
        this, &PlayerWindowModular::save_new_clip_activated);

    ui.toolBar->addAction(ui.actCancelClip);
    connect(ui.actCancelClip, &QAction::triggered,
        this, &PlayerWindowModular::cancel_new_clip_activated);

    if (m_mode == Mode::Watching)
    {
        m_mode_mediator->set_mode(PlayerWindowMode::Watching);
        m_playback_mediator->set_file(m_clip_queue->get_file_path());
        m_playback_mediator->set_state(PlayState::Playing);
    }
    else if (m_mode == Mode::WatchingClip)
    {
        m_mode_mediator->set_mode(PlayerWindowMode::WatchingClip);

        const Clip* clip = m_clip_queue->get_clip();
        const ClipUserData* data = clip->get_user_data();
        //if (clip)
        //    m_playback_mediator->play(clip->get_user_data()->begin, clip->get_user_data()->end);
        m_playback_mediator->set_file(m_clip_queue->get_file_path());
        m_playback_mediator->set_time(clip->get_user_data()->begin);
        m_playback_mediator->set_state(PlayState::Playing);

        // m_playback_mediator->set_file(m_clip_queue->get_file_path(), true, clip->get_user_data()->begin);
    }
    show();
}

void PlayerWindowModular::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);

    if (m_showed)
        return;
    m_showed = true;
}

void PlayerWindowModular::set_mode(PlayerWindowMode mode)
{
    if (mode == PlayerWindowMode::Watching)
    {
        ui.actAddClip->setVisible(true);
        ui.actSaveClip->setVisible(false);
        ui.actCancelClip->setVisible(false);
    }
    else if (mode == PlayerWindowMode::AddingClip)
    {
        ui.actAddClip->setVisible(false);
        ui.actSaveClip->setVisible(true);
        ui.actCancelClip->setVisible(true);
    }
    else
    {
        ui.actAddClip->setVisible(false);
        ui.actSaveClip->setVisible(false);
        ui.actCancelClip->setVisible(false);
    }
}

void PlayerWindowModular::add_new_clip_activated()
{
    m_mode_mediator->set_mode(PlayerWindowMode::AddingClip);
}

void PlayerWindowModular::save_new_clip_activated()
{
    // m_clip_queue->set_clip_user_data(get_clip_user_data());
    std::unique_ptr<ClipUserData> user_data = std::make_unique<ClipUserData>();
    user_data->subtitles.resize(2);
    m_clip_mediator->save(*user_data.get());
    m_clip_queue->set_clip_user_data(std::move(user_data));
    m_clip_queue->set_removed(ui.actRemoveClip->isChecked());
    m_clip_queue->save_library();

    m_mode_mediator->set_mode(PlayerWindowMode::Watching);
}

void PlayerWindowModular::cancel_new_clip_activated()
{
    m_mode_mediator->set_mode(PlayerWindowMode::Watching);
}
