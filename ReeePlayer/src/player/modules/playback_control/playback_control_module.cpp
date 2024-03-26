#include <playback_control_module.h>
#include <app.h>
#include <playback_mediator.h>
#include <clip_storage.h>
#include <session.h>

#include <ui_player_window.h>

#include <QMainWindow>
#include <QHBoxLayout>
#include <QToolButton>
#include <QPushButton>
#include "vad_module.h"

using namespace std::chrono;

namespace
{
    struct PlaybackRateItem
    {
        float rate;
        const char* text;
        QKeySequence key;
    };

    const PlaybackRateItem PLAYBACK_ITEMS[] =
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
}

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

PlaybackControlModule::PlaybackControlModule(App* app, IClipQueue* clip_queue,
    ModeMediator* mode_mediator, PlaybackMediator* playback_mediator)
    : m_app(app), m_clip_queue(clip_queue), m_mode_mediator(mode_mediator), m_playback_mediator(playback_mediator)
{
    connect(m_mode_mediator, &ModeMediator::mode_changed, this, &PlaybackControlModule::set_mode);

    connect(m_playback_mediator, &PlaybackMediator::file_changed, this, &PlaybackControlModule::set_file);
    connect(m_playback_mediator, &PlaybackMediator::length_changed, this, &PlaybackControlModule::set_length);
    connect(m_playback_mediator, &PlaybackMediator::time_changed, this, &PlaybackControlModule::set_time);
    connect(m_playback_mediator, &PlaybackMediator::state_changed, this, &PlaybackControlModule::set_state);
    connect(m_playback_mediator, &PlaybackMediator::default_rate_changed, this, &PlaybackControlModule::set_default_rate);
}

void PlaybackControlModule::setup_player(Ui_PlayerWindow* pw)
{
    m_lbl_time = pw->lblTime;
    m_slider = pw->slider;
    m_act_play_pause = pw->actPlayPause;
    m_btn_play = pw->btnPlay;

    m_slider->setStyle(new SliderStyle(m_slider->style()));

    QObject::connect(m_slider, &QSlider::sliderPressed, this, &PlaybackControlModule::slider_pressed);
    QObject::connect(m_slider, &QSlider::sliderMoved, this, &PlaybackControlModule::slider_moved);
    QObject::connect(m_slider, &QSlider::sliderReleased, this, &PlaybackControlModule::slider_released);

    m_btn_play->setDefaultAction(m_act_play_pause);

    connect(m_act_play_pause, &QAction::triggered,
        this, &PlaybackControlModule::act_play_pause_triggered);

    m_forward_shortcut = new QShortcut(Qt::Key_Right, pw->centralwidget);
    connect(m_forward_shortcut, &QShortcut::activated,
        [this]() { rewind(2000); });

    m_backward_shortcut = new QShortcut(Qt::Key_Left, pw->centralwidget);
    connect(m_backward_shortcut, &QShortcut::activated,
        [this]() { rewind(-2000); });

    setup_playback_rates(pw);
}

void PlaybackControlModule::setup_playback_rates(Ui_PlayerWindow* pw)
{
    m_rate_btn_group = new QButtonGroup(this);
    m_rate_btn_group->setExclusive(true);
    int id = 0;
    QFont font;
    QFontMetrics fm(font);
    for (const auto& item : PLAYBACK_ITEMS)
    {
        QPushButton* b = new QPushButton(pw->centralwidget);
        b->setText(item.text);
        b->setToolTip(QString("Hotkey: %1").arg(item.key.toString()));
        b->setCheckable(true);
        // b->setMaximumWidth(fm.horizontalAdvance(item.text) + 10);
        b->setMaximumWidth(30);
        b->setFocusPolicy(Qt::NoFocus);
        b->setStyleSheet("QPushButton:checked {font: bold 14px;}");
        m_rate_btn_group->addButton(b);
        m_rate_btn_group->setId(b, id);
        pw->ratesLayout->addWidget(b);
        ++id;
        connect(b, &QPushButton::toggled,
            [b](bool checked)
            {
                if (checked)
                    b->setStyleSheet("font: bold 14px; background-color: red;");
                else
                    b->setStyleSheet(QString());
            });

        QShortcut* shortcut = new QShortcut(item.key, pw->centralwidget);
        connect(shortcut, &QShortcut::activated,
            [this, &item] {
                m_playback_mediator->set_default_rate(item.rate);
            });
    }
    connect(m_rate_btn_group, &QButtonGroup::buttonClicked,
        this, &PlaybackControlModule::on_set_playback_rate);
}

void PlaybackControlModule::set_playback_rate(PlaybackRate rate, bool play)
{
    //auto it = std::ranges::find_if(PLAYBACK_ITEMS,
    //    [rate](const PlaybackRateItem& item) {return item.rate == rate; });

    //if (it != std::end(PLAYBACK_ITEMS))
    //{
    //    int index = std::distance(std::begin(PLAYBACK_ITEMS), it);
    //    QAbstractButton* button = m_rate_btn_group->button(index);
    //    button->blockSignals(true);
    //    button->setChecked(true);
    //    button->blockSignals(false);
    //}
    //else
    //{
    //    auto button = m_rate_btn_group->checkedButton();
    //    if (button)
    //    {
    //        button->blockSignals(true);
    //        m_rate_btn_group->setExclusive(false);
    //        button->setChecked(false);
    //        m_rate_btn_group->setExclusive(true);
    //        button->blockSignals(false);
    //    }
    //}

    //if (play)
    //{
    //    m_playback_mediator->set_state(PlayState::Playing);
    //}

    //m_playback_rate = PLAYBACK_ITEMS[index].rate;

    ////if (!m_jc_settings || !m_jc_settings->is_activated())
    //{
    //    m_video_widget->set_rate(m_playback_rate);
    //}
}

void PlaybackControlModule::on_set_playback_rate(QAbstractButton* button)
{
    int index = m_rate_btn_group->id(button);
    float rate = PLAYBACK_ITEMS[index].rate;
    m_playback_mediator->set_default_rate(rate);

    // set_playback_rate(index, true);
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

void PlaybackControlModule::set_mode(PlayerWindowMode pw)
{
    bool ena = is_film_mode(pw);
    m_act_play_pause->setVisible(ena);
    m_btn_play->setVisible(ena);
    m_forward_shortcut->setEnabled(ena);
    m_backward_shortcut->setEnabled(ena);

    if (pw == PlayerWindowMode::Watching)
    {
        // TODO prev rate
        m_playback_mediator->set_default_rate(1.0f);
    }
    else if (pw == PlayerWindowMode::Closing)
    {
        std::unique_ptr<FileUserData> file_user_data = std::make_unique<FileUserData>();
        file_user_data->player_time = m_playback_mediator->get_time();
        file_user_data->length = m_playback_mediator->get_length();
        m_clip_queue->set_file_user_data(std::move(file_user_data));
        m_clip_queue->save_library();
    }
}

void PlaybackControlModule::set_file(const File* file)
{
    if (m_mode_mediator->is_film_mode())
    {
        const FileUserData* data = file->get_user_data();
        if (data)
            m_playback_mediator->set_time(data->player_time);
    }
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

void PlaybackControlModule::set_default_rate(PlaybackRate rate)
{
    auto it = std::ranges::find_if(PLAYBACK_ITEMS,
        [rate](const PlaybackRateItem& item) {return item.rate == rate; });

    if (it != std::end(PLAYBACK_ITEMS))
    {
        int index = std::distance(std::begin(PLAYBACK_ITEMS), it);
        QAbstractButton* button = m_rate_btn_group->button(index);
        //button->blockSignals(true);
        button->setChecked(true);
        //button->blockSignals(false);
    }
    else
    {
        auto button = m_rate_btn_group->checkedButton();
        if (button)
        {
            //button->blockSignals(true);
            m_rate_btn_group->setExclusive(false);
            button->setChecked(false);
            m_rate_btn_group->setExclusive(true);
            //button->blockSignals(false);
        }
    }
}

void PlaybackControlModule::update_label(PlaybackTime time)
{
    m_lbl_time->setText(ms_to_str(time) + " / " + ms_to_str(m_playback_mediator->get_length()));
}

void PlaybackControlModule::rewind(int delta_ms)
{
    // TODO VAD
    m_playback_mediator->rewind(delta_ms);

    //int new_time;
    //if (m_vad && m_jc_settings->is_activated() && m_jc_settings->is_non_voice_skipping())
    //{
    //    new_time = m_vad->rewind(m_video_widget->get_time(), delta_ms);
    //}
    //else
    //{
    //    new_time = m_video_widget->get_time() + delta_ms;
    //}
}
