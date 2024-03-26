#pragma once

#include <QObject>
#include <time_types.h>

class Rewinder;
class Seeker;
class File;

enum class PlayState { Playing, Paused, Stopped };

using PlaybackRate = float;

class PlaybackMediator : public QObject
{
    Q_OBJECT

signals:
    void file_changed(const File*, bool auto_play = false, int start_time = 0);
    void time_changed(PlaybackTime);
    void trigger_time_changed(PlaybackTime);
    void length_changed(PlaybackTime);
    void state_changed(PlayState);
    void played(PlaybackTime, PlaybackTime);
    void rate_changed(PlaybackRate);
    void default_rate_changed(PlaybackRate);

public:
    void set_rewinder(const Rewinder*);
    void set_seeker(const Seeker*);

    const File* get_file() const;
    void set_file(const File*, bool auto_play = false, int start_time = 0);

    PlaybackTime get_time() const;
    void set_time(PlaybackTime);

    PlaybackTime get_length() const;
    void set_length(PlaybackTime);

    PlayState get_state() const;
    void set_state(PlayState);

    void play(PlaybackTime a, PlaybackTime b);

    void set_trigger_time(PlaybackTime);

    void rewind(PlaybackTimeDiff);

    PlaybackRate get_rate() const;
    PlaybackRate get_default_rate() const;
    void set_default_rate(PlaybackRate);

private:
    void set_rate(PlaybackRate);
    void update_rate();

    const Rewinder* m_rewinder = nullptr;
    const Seeker* m_seeker = nullptr;

    const File* m_file;
    PlaybackTime m_time = -1;
    PlaybackTime m_trigger_time = 0;
    PlaybackTime m_length = 0;
    PlayState m_state = PlayState::Stopped;
    PlaybackRate m_rate = 0.0f;
    PlaybackRate m_default_rate = 0.0f;

    std::chrono::time_point<std::chrono::system_clock> m_last_set_user_time;
};