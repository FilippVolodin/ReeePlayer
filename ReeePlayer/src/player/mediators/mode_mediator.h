#pragma once

enum class PlayerWindowMode { NotLoaded, Watching, AddingClip, WatchingClip, Repeating };

class ModeMediator : public QObject
{
    Q_OBJECT

signals:
    void mode_changed(PlayerWindowMode);

public:
    ModeMediator();

    PlayerWindowMode get_mode() const;
    bool is_film_mode() const;
    bool is_clip_mode() const;
    void set_mode(PlayerWindowMode);

private:
    PlayerWindowMode m_mode = PlayerWindowMode::NotLoaded;
};

bool is_film_mode(PlayerWindowMode);
bool is_clip_mode(PlayerWindowMode);
