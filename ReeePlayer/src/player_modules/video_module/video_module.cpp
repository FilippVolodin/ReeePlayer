#include <video_module.h>
#include <app.h>
#include <i_video_widget.h>
#include <playback_presenter.h>
#include <ui_player_window.h>
#include <emitter.h>

#include <QMainWindow>
#include <QBoxLayout>

VideoModule::VideoModule(App* app, PlaybackPresenter* playback_presenter)
    : m_app(app), m_playback_presenter(playback_presenter)
{
    PLAYER_ENGINE player_engine = (PLAYER_ENGINE)m_app->get_setting("main", "player", (int)PLAYER_ENGINE::Web).toInt();

    m_video_widget = m_app->get_player_widget(player_engine);

    //QObject::connect(m_playback_presenter, &PlaybackPresenter::play, m_video_widget, &IVideoWidget::play);
    connect(m_playback_presenter, &PlaybackPresenter::played, this, &VideoModule::play);
    connect(m_playback_presenter, &PlaybackPresenter::time_changed, this, &VideoModule::set_time);
    connect(m_playback_presenter, &PlaybackPresenter::file_changed, this, &VideoModule::set_file);

    connect(m_video_widget->get_emitter(), &Emitter::time_changed, this, &VideoModule::time_changed);
    connect(m_video_widget->get_emitter(), &Emitter::length_changed, this, &VideoModule::length_changed);

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

void VideoModule::play(PlaybackEventSource es)
{
    if (es == PlaybackEventSource::User)
        m_video_widget->play();
}

void VideoModule::set_time(int time, PlaybackEventSource es)
{
    if (es == PlaybackEventSource::User)
        m_video_widget->set_time(time);
}

void VideoModule::set_file(const QString& file_path)
{
    m_video_widget->set_file_name(file_path);
}

void VideoModule::time_changed(int time)
{
    m_playback_presenter->set_time(time, PlaybackEventSource::Player);
}

void VideoModule::length_changed(int length)
{
    m_playback_presenter->set_length(length);
}
