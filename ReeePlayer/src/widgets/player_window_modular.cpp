#include "player_window_modular.h"
#include <app.h>

#include <video_module.h>
#include <playback_control_module.h>

#include <playback_presenter.h>
#include <session.h>

PlayerWindowModular::PlayerWindowModular(App* app, QWidget* parent)
    : QMainWindow(parent), m_app(app)
{
    ui.setupUi(this);
    ui.waveform->setVisible(false);
}

void PlayerWindowModular::run(Mode, std::shared_ptr<IClipQueue> clip_queue)
{
    m_clip_queue = clip_queue;
    //QWidget* centralwidget = new QWidget(this);
    //centralwidget->setObjectName("centralwidget");
    //this->setCentralWidget(centralwidget);
    //QVBoxLayout* verticalLayout = new QVBoxLayout(centralwidget);

    m_playback_presenter = std::make_unique<PlaybackPresenter>();
    
    m_video_module = std::make_unique<VideoModule>(m_app, m_playback_presenter.get());
    m_playback_module = std::make_unique<PlaybackControlModule>(m_app, m_playback_presenter.get());
    m_video_module->setup_player(&ui);
    m_playback_module->setup_player(&ui);

    show();
}

void PlayerWindowModular::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);

    if (m_showed)
        return;
    m_showed = true;

    m_playback_presenter->set_file(m_clip_queue->get_file_path());
    m_playback_presenter->play(PlaybackEventSource::User);
}
