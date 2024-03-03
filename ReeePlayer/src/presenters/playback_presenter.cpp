#include <playback_presenter.h>

QString PlaybackPresenter::get_file() const
{
    return m_filename;
}

void PlaybackPresenter::set_file(const QString& filename)
{
    m_filename = filename;
    emit file_changed(filename);
}

void PlaybackPresenter::play(PlaybackEventSource es)
{
    emit played(es);
}

PlaybackTime PlaybackPresenter::get_time() const
{
    return m_time;
}

void PlaybackPresenter::set_time(PlaybackTime time, PlaybackEventSource es)
{
    if (es == PlaybackEventSource::Player)
        m_time = time;
    emit time_changed(time, es);
}

PlaybackTime PlaybackPresenter::get_length() const
{
    return m_length;
}

void PlaybackPresenter::set_length(PlaybackTime length)
{
    m_length = length;
    emit length_changed(length);
}
