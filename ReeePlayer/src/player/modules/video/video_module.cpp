#include <video_module.h>
#include <app.h>
#include <i_video_widget.h>
#include <playback_mediator.h>
#include <ui_player_window.h>
#include <emitter.h>

#include <QMainWindow>
#include <QBoxLayout>

VideoModule::VideoModule(App* app, PlaybackMediator* playback_mediator)
    : m_app(app), m_playback_mediator(playback_mediator)
{
    PLAYER_ENGINE player_engine = (PLAYER_ENGINE)m_app->get_setting("main", "player", (int)PLAYER_ENGINE::Web).toInt();

    m_video_widget = m_app->get_player_widget(player_engine);

    //QObject::connect(m_playback_mediator, &PlaybackMediator::play, m_video_widget, &IVideoWidget::play);
    connect(m_playback_mediator, &PlaybackMediator::state_changed, this, &VideoModule::set_state);
    connect(m_playback_mediator, &PlaybackMediator::played, this, &VideoModule::play);
    connect(m_playback_mediator, &PlaybackMediator::time_changed, this, &VideoModule::set_time);
    connect(m_playback_mediator, &PlaybackMediator::trigger_time_changed, this, &VideoModule::set_trigger_time);
    connect(m_playback_mediator, &PlaybackMediator::file_changed, this, &VideoModule::set_file);

    connect(m_video_widget->get_emitter(), &Emitter::time_changed, this, &VideoModule::time_changed);
    connect(m_video_widget->get_emitter(), &Emitter::length_changed, this, &VideoModule::length_changed);
    connect(m_video_widget->get_emitter(), &Emitter::playing, this, &VideoModule::playing);
    connect(m_video_widget->get_emitter(), &Emitter::paused, this, &VideoModule::paused);
    connect(m_video_widget->get_emitter(), &Emitter::stopped, this, &VideoModule::stopped);
    connect(m_video_widget->get_emitter(), &Emitter::timer_triggered, this, &VideoModule::timer_triggered);

    //connect(m_video_widget->get_emitter(), &Emitter::playing,
    //    this, &PlayerWindow::on_player_playing);
    //connect(m_video_widget->get_emitter(), &Emitter::paused,
    //    this, &PlayerWindow::on_player_paused);
    //connect(m_video_widget->get_emitter(), &Emitter::timer_triggered,
    //    this, &PlayerWindow::on_player_timer_triggered);
}

VideoModule::~VideoModule()
{
    m_video_widget->get_emitter()->disconnect(this);
    m_video_widget->unload();
    dynamic_cast<QWidget*>(m_video_widget)->setParent(nullptr);
}

void VideoModule::setup_player(Ui_PlayerWindow* player_window)
{
    QWidget* w = dynamic_cast<QWidget*>(m_video_widget);

    auto sp = w->sizePolicy();
    sp.setVerticalStretch(5);
    w->setSizePolicy(sp);

    // QWidget* centralwidget = player_window->findChild<QWidget*>("centralwidget");
    if (player_window->centralwidget)
    {
        QBoxLayout* lo = dynamic_cast<QBoxLayout*>(player_window->centralwidget->layout());
        if (lo)
            lo->insertWidget(0, w);
        else
            lo->addWidget(w);
    }
}

void VideoModule::set_state(PlayState ps)
{
    if (ps == PlayState::Playing)
        m_video_widget->play();
    else if (ps == PlayState::Paused)
        m_video_widget->pause();
    else if (ps == PlayState::Stopped)
        m_video_widget->stop();
}

void VideoModule::play(PlaybackTime a, PlaybackTime b)
{
    m_video_widget->play(a, b, 1);
}

void VideoModule::set_time(PlaybackTime time)
{
    if (time != m_video_time)
        m_video_widget->set_time(time);
}

void VideoModule::set_trigger_time(PlaybackTime time)
{
    m_video_widget->set_timer(time);
}

void VideoModule::set_file(const QString& file_path, bool auto_play, int start_time)
{
    m_video_widget->set_file_name(file_path, auto_play, start_time);
}

void VideoModule::time_changed(int time)
{
    m_video_time = time;
    m_playback_mediator->set_time(time);
}

void VideoModule::length_changed(int length)
{
    m_playback_mediator->set_length(length);
}

void VideoModule::playing()
{
    m_playback_mediator->set_state(PlayState::Playing);
}

void VideoModule::paused()
{
    m_playback_mediator->set_state(PlayState::Paused);
}

void VideoModule::stopped()
{
    m_playback_mediator->set_state(PlayState::Stopped);
}

void VideoModule::timer_triggered()
{
    m_playback_mediator->set_state(PlayState::Paused);
}
