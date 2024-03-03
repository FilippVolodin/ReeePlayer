#include <playback_control_module.h>
#include <app.h>
#include <playback_presenter.h>
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

PlaybackControlModule::PlaybackControlModule(App* app, PlaybackPresenter* playback_presenter)
    : m_app(app), m_playback_presenter(playback_presenter)
{
}

void PlaybackControlModule::setup_player(Ui_PlayerWindow* player_window)
{
    m_lbl_time = player_window->lblTime;
    m_slider = player_window->slider;
    m_slider->setStyle(new SliderStyle(m_slider->style()));

    QObject::connect(m_slider, &QSlider::sliderPressed, this, &PlaybackControlModule::slider_pressed);
    QObject::connect(m_slider, &QSlider::sliderMoved, this, &PlaybackControlModule::slider_moved);
    QObject::connect(m_slider, &QSlider::sliderReleased, this, &PlaybackControlModule::slider_released);

    connect(m_playback_presenter, &PlaybackPresenter::length_changed, this, &PlaybackControlModule::set_length);
    connect(m_playback_presenter, &PlaybackPresenter::time_changed, this, &PlaybackControlModule::set_time);
}

void PlaybackControlModule::timerEvent(QTimerEvent* event)
{
    if (m_sent_time != m_press_slider_time)
    {
        m_sent_time = m_press_slider_time;
        m_playback_presenter->set_time(m_press_slider_time, PlaybackEventSource::User);
        update_label(m_press_slider_time);
    }
}

void PlaybackControlModule::slider_value_changed(int value)
{
}

void PlaybackControlModule::slider_pressed()
{
    m_timer_id = startTimer(200ms);

    m_slider_pressed = true;

    m_press_slider_time = m_slider->value();
    m_sent_time = m_press_slider_time;
    m_playback_presenter->set_time(m_press_slider_time, PlaybackEventSource::User);
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
        m_playback_presenter->set_time(m_press_slider_time, PlaybackEventSource::User);
        update_label(m_press_slider_time);
    }
}

void PlaybackControlModule::set_time(PlaybackTime time, PlaybackEventSource es)
{
    if (es == PlaybackEventSource::Player)
    {
        update_label(time);
        if (!m_slider_pressed)
        {
            m_slider->blockSignals(true);
            m_slider->setValue(time);
            m_slider->blockSignals(false);
        }
    }
    else if (es == PlaybackEventSource::User)
    {
        if (!m_slider_pressed && m_slider->value() != time)
        {
            m_slider->blockSignals(true);
            m_slider->setValue(time);
            m_slider->blockSignals(false);

            update_label(time);
        }
    }
}

void PlaybackControlModule::set_length(PlaybackTime length)
{
    m_slider->blockSignals(true);
    m_slider->setMaximum(length);
    m_slider->blockSignals(false);
}

void PlaybackControlModule::update_label(PlaybackTime time)
{
    m_lbl_time->setText(ms_to_str(time) + " / " + ms_to_str(m_playback_presenter->get_length()));
}
