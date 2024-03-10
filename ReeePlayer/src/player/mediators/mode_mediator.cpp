#include <mode_mediator.h>

ModeMediator::ModeMediator()
{
}

PlayerWindowMode ModeMediator::get_mode() const
{
    return m_mode;
}

bool ModeMediator::is_film_mode() const
{
    return ::is_film_mode(m_mode);
}

bool ModeMediator::is_clip_mode() const
{
    return ::is_clip_mode(m_mode);
}

void ModeMediator::set_mode(PlayerWindowMode mode)
{
    if (mode != m_mode)
    {
        m_mode = mode;
        emit mode_changed(m_mode);
    }
}

bool is_film_mode(PlayerWindowMode mode)
{
    return mode == PlayerWindowMode::Watching;
}

bool is_clip_mode(PlayerWindowMode mode)
{
    return
        mode == PlayerWindowMode::AddingClip ||
        mode == PlayerWindowMode::Repeating ||
        mode == PlayerWindowMode::WatchingClip;
}
