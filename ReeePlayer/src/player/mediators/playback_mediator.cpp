#include "playback_mediator.h"
#include <playback_mediator.h>
#include <rewinder.h>
#include <seeker.h>
#include <clip_storage.h>

void PlaybackMediator::set_precision_time_producer(
    const PrecisionTimeProducer* precision_time_producer)
{
    m_precision_time_producer = precision_time_producer;
}

void PlaybackMediator::set_rewinder(const Rewinder* rewinder)
{
    m_rewinder = rewinder;
}

void PlaybackMediator::set_seeker(const Seeker* seeker)
{
    m_seeker = seeker;
}

const File* PlaybackMediator::get_file() const
{
    return m_file;
}

void PlaybackMediator::set_file(const File* file, bool auto_play, int start_time)
{
    m_file = file;
    emit file_changed(file, auto_play, start_time);
}

PlayState PlaybackMediator::get_state() const
{
    return m_state;
}

void PlaybackMediator::set_state(PlayState ps)
{
    m_state = ps;
    emit state_changed(ps);
}

void PlaybackMediator::play(PlaybackTime a, PlaybackTime b)
{
    //emit trigger_time_changed(b);
    //set_time(a);
    //set_state(PlayState::Playing);

    set_time(a);
    set_trigger_time(b, TimerAction::Pause);
    set_state(PlayState::Playing);

    // emit played(a, b);
}

void PlaybackMediator::set_trigger_time(PlaybackTime time, TimerAction timer_action)
{
    m_timer_action = timer_action;
    if (m_trigger_time != time)
        emit trigger_time_changed(time);
}

void PlaybackMediator::timer_triggered(PlaybackTime)
{
    if (m_timer_action == TimerAction::Pause)
        set_state(PlayState::Paused);
}

void PlaybackMediator::rewind(PlaybackTimeDiff diff)
{
    std::optional<PlaybackTime> new_time;
    if (m_rewinder && (new_time = m_rewinder->rewind(m_time, diff)))
        set_time(new_time.value());
    else
        set_time(m_time + diff);
}

PlaybackRate PlaybackMediator::get_rate() const
{
    return m_overridden_rate ? m_overridden_rate.value() : m_default_rate;
}

//void PlaybackMediator::set_rate(PlaybackRate rate)
//{
//    if (m_rate != rate)
//    {
//        m_rate = rate;
//        emit rate_changed(m_default_rate);
//    }
//}

PlaybackRate PlaybackMediator::get_default_rate() const
{
    return m_default_rate;
}

void PlaybackMediator::set_default_rate(PlaybackRate default_rate)
{
    PlaybackRate old_rate = get_rate();
    if (m_default_rate != default_rate)
    {
        m_default_rate = default_rate;
        emit default_rate_changed(m_default_rate);
    }
    if (old_rate != get_rate())
        emit rate_changed(get_rate());
}

std::optional<PlaybackRate> PlaybackMediator::get_overridden_rate() const
{
    return m_overridden_rate;
}

void PlaybackMediator::set_overridden_rate(std::optional<PlaybackRate> overridden_rate)
{
    PlaybackRate old_rate = get_rate();
    m_overridden_rate = overridden_rate;
    if (old_rate != get_rate())
        emit rate_changed(get_rate());
}

int PlaybackMediator::get_precision_time() const
{
    if (m_precision_time_producer)
        return m_precision_time_producer->get_precision_time();
    else
        return m_time;
}

void PlaybackMediator::update_rate()
{
    PlaybackRate rate = m_default_rate;
    //if (m_rate_overridder)
    //{
    //    overridden_rate = m_rate_overridder->get_rate(m_time);
    //    if (overridden_rate)
    //        rate = overridden_rate->value();
    //}
    // set_rate(rate);
}

PlaybackTime PlaybackMediator::get_time() const
{
    return m_time;
}

void PlaybackMediator::set_time(PlaybackTime time)
{
    if (m_time != time)
    {
        m_time = time;
        emit time_changed(time);

        // update_rate();
    }
}

PlaybackTime PlaybackMediator::get_length() const
{
    return m_length;
}

void PlaybackMediator::set_length(PlaybackTime length)
{
    m_length = length;
    emit length_changed(length);
}
