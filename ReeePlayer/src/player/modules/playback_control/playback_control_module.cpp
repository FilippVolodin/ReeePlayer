#include <playback_control_module.h>
#include <app.h>
#include <playback_mediator.h>
#include <ui_player_window.h>

#include <QMainWindow>
#include <QHBoxLayout>
#include <QToolButton>

using namespace std::chrono;

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

PlaybackControlModule::PlaybackControlModule(App* app, PlaybackMediator* playback_mediator)
    : m_app(app), m_playback_mediator(playback_mediator)
{
}

void PlaybackControlModule::setup_player(Ui_PlayerWindow* player_window)
{
    m_lbl_time = player_window->lblTime;
    m_slider = player_window->slider;
    m_act_play_pause = player_window->actPlayPause;

    m_slider->setStyle(new SliderStyle(m_slider->style()));

    QObject::connect(m_slider, &QSlider::sliderPressed, this, &PlaybackControlModule::slider_pressed);
    QObject::connect(m_slider, &QSlider::sliderMoved, this, &PlaybackControlModule::slider_moved);
    QObject::connect(m_slider, &QSlider::sliderReleased, this, &PlaybackControlModule::slider_released);

    connect(m_playback_mediator, &PlaybackMediator::length_changed, this, &PlaybackControlModule::set_length);
    connect(m_playback_mediator, &PlaybackMediator::time_changed, this, &PlaybackControlModule::set_time);
    connect(m_playback_mediator, &PlaybackMediator::state_changed, this, &PlaybackControlModule::set_state);

    player_window->actPlayPause->setVisible(true);
    player_window->btnPlay->setDefaultAction(player_window->actPlayPause);

    connect(m_act_play_pause, &QAction::triggered,
        this, &PlaybackControlModule::act_play_pause_triggered);
}

void PlaybackControlModule::timerEvent(QTimerEvent* event)
{
    if (m_sent_time != m_press_slider_time)
    {
        m_sent_time = m_press_slider_time;
        m_playback_mediator->set_time(m_press_slider_time);
        update_label(m_press_slider_time);
    }
}

void PlaybackControlModule::slider_pressed()
{
    m_timer_id = startTimer(200ms);

    m_slider_pressed = true;

    m_press_slider_time = m_slider->value();
    m_sent_time = m_press_slider_time;
    m_playback_mediator->set_time(m_press_slider_time);
    update_label(m_press_slider_time);
}

void PlaybackControlModule::slider_moved(int value)
{
    m_press_slider_time = value;
}

void PlaybackControlModule::slider_released()
{
    killTimer(m_timer_id);

    m_slider_pressed = false;
    if (m_sent_time != m_press_slider_time)
    {
        m_playback_mediator->set_time(m_press_slider_time);
        update_label(m_press_slider_time);
    }
}

void PlaybackControlModule::act_play_pause_triggered()
{
    bool act_played = m_act_play_pause->isChecked();
    m_playback_mediator->set_state(act_played ? PlayState::Playing : PlayState::Paused);

    //if (m_video_widget->is_playing())
    //{
    //    m_video_widget->pause();
    //}
    //else
    //{
    //    if (m_video_widget->at_end())
    //        m_video_widget->set_time(0);
    //    m_video_widget->play();
    //}
}

void PlaybackControlModule::set_time(PlaybackTime time)
{
    if (!m_slider_pressed && m_slider->value() != time)
    {
        m_slider->setValue(time);
        update_label(time);
    }
}

void PlaybackControlModule::set_length(PlaybackTime length)
{
    m_slider->setMaximum(length);
}

void PlaybackControlModule::set_state(PlayState ps)
{
    m_act_play_pause->setChecked(ps == PlayState::Playing);
}

void PlaybackControlModule::update_label(PlaybackTime time)
{
    m_lbl_time->setText(ms_to_str(time) + " / " + ms_to_str(m_playback_mediator->get_length()));
}
