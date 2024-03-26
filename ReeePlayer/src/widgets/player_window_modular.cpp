#include "player_window_modular.h"
#include <app.h>

#include <video_module.h>
#include <playback_control_module.h>
#include <subtitles_module.h>
#include <clip_module.h>
#include <vad_module.h>

#include <playback_mediator.h>
#include <clip_mediator.h>
#include <session.h>
#include <clip_storage.h>
#include <subtitles_list.h>

constexpr const char* PLAYER_WINDOW_GEOMETRY_KEY = "player_window_geometry";
constexpr const char* WINDOW_STATE_KEY = "player_window_state";

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

    m_subtitles_list = std::make_unique<SubtitlesList>(m_app, m_clip_queue->get_file_path());

    m_mode_mediator = std::make_unique<ModeMediator>();
    m_playback_mediator = std::make_unique<PlaybackMediator>();
    m_clip_mediator = std::make_unique<ClipMediator>();
    
    m_video_module = std::make_unique<VideoModule>(m_app, m_playback_mediator.get());
    m_playback_module = std::make_unique<PlaybackControlModule>(m_app, m_clip_queue.get(), m_mode_mediator.get(), m_playback_mediator.get());
    m_subtitles_modules[0] = std::make_unique<SubtitlesModule>(0, m_app, m_subtitles_list.get(), m_mode_mediator.get(), m_playback_mediator.get());
    m_subtitles_modules[1] = std::make_unique<SubtitlesModule>(1, m_app, m_subtitles_list.get(), m_mode_mediator.get(), m_playback_mediator.get());
    m_clip_module = std::make_unique<ClipModule>(m_app, m_subtitles_list.get(), m_clip_queue.get(),
        m_mode_mediator.get(), m_playback_mediator.get(), m_clip_mediator.get());
    m_vad_module = std::make_unique<VADModule>(m_app, m_mode_mediator.get(), m_playback_mediator.get());

    m_clip_mediator->add_unit(m_subtitles_modules[0].get());
    m_clip_mediator->add_unit(m_subtitles_modules[1].get());
    m_clip_mediator->add_unit(m_clip_module.get());

    m_video_module->setup_player(&ui);
    m_playback_module->setup_player(&ui);
    m_subtitles_modules[0]->setup_player(&ui);
    m_subtitles_modules[1]->setup_player(&ui);
    m_clip_module->setup_player(&ui);
    m_vad_module->setup_player(&ui);

    m_playback_mediator->set_rewinder(m_subtitles_list.get());

    if (m_mode == Mode::Watching)
    {
        m_mode_mediator->set_mode(PlayerWindowMode::Watching);
        m_playback_mediator->set_file(m_clip_queue->get_current_file());
        // m_playback_mediator->set_time(0);
        m_playback_mediator->set_state(PlayState::Playing);
    }
    else if (m_mode == Mode::WatchingClip)
    {
        m_mode_mediator->set_mode(PlayerWindowMode::WatchingClip);

        const Clip* clip = m_clip_queue->get_clip();

        // const ClipUserData* data = clip->get_user_data();
        m_playback_mediator->set_file(m_clip_queue->get_current_file());
        m_clip_mediator->load(*clip);
        //if (clip)
        //    m_playback_mediator->play(clip->get_user_data()->begin, clip->get_user_data()->end);
        //m_playback_mediator->set_time(clip->get_user_data()->begin);
        //m_playback_mediator->set_state(PlayState::Playing);

        // m_playback_mediator->set_file(m_clip_queue->get_file_path(), true, clip->get_user_data()->begin);
    }
    else if (m_mode == Mode::Repeating)
    {
        m_mode_mediator->set_mode(PlayerWindowMode::Repeating);
        const Clip* clip = m_clip_queue->get_clip();

        // const ClipUserData* data = clip->get_user_data();
        m_playback_mediator->set_file(m_clip_queue->get_current_file());
        m_clip_mediator->load(*clip);
    }
    show();
}

void PlayerWindowModular::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);

    if (m_showed)
        return;
    m_showed = true;

    QByteArray s = m_app->get_setting("gui", WINDOW_STATE_KEY).toByteArray();
    QByteArray g = m_app->get_setting("gui", PLAYER_WINDOW_GEOMETRY_KEY).toByteArray();
    restoreState(s);
    restoreGeometry(g);
}

void PlayerWindowModular::closeEvent(QCloseEvent* event)
{
    m_mode_mediator->set_mode(PlayerWindowMode::Closing);

    m_app->set_setting("gui", WINDOW_STATE_KEY, saveState());
    m_app->set_setting("gui", PLAYER_WINDOW_GEOMETRY_KEY, saveGeometry());

    QWidget::closeEvent(event);
}