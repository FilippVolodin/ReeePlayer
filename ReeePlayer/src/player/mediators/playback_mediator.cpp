#include <playback_mediator.h>

QString PlaybackMediator::get_file() const
{
    return m_filename;
}

void PlaybackMediator::set_file(const QString& filename, bool auto_play, int start_time)
{
    m_filename = filename;
    emit file_changed(filename, auto_play, start_time);
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
    emit played(a, b);
}

void PlaybackMediator::set_trigger_time(PlaybackTime time)
{
    if (m_trigger_time != time)
        emit trigger_time_changed(time);
}

PlaybackTime PlaybackMediator::get_time() const
{
    return m_time;
}

void PlaybackMediator::set_time(PlaybackTime time)
{
    if (m_time != time)
    {
        qDebug("PlaybackMediator::set_time(%d)", time);
        m_time = time;
        emit time_changed(time);
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
