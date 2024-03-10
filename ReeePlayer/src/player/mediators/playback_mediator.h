#pragma once

#include <QObject>
#include <time_types.h>

enum class PlayState { Playing, Paused, Stopped };

class PlaybackMediator : public QObject
{
    Q_OBJECT

signals:
    void file_changed(const QString&, bool auto_play = false, int start_time = 0);
    void time_changed(PlaybackTime);
    void trigger_time_changed(PlaybackTime);
    void length_changed(PlaybackTime);
    void state_changed(PlayState);
    void played(PlaybackTime, PlaybackTime);

public:

    QString get_file() const;
    void set_file(const QString&, bool auto_play = false, int start_time = 0);

    PlaybackTime get_time() const;
    void set_time(PlaybackTime);

    PlaybackTime get_length() const;
    void set_length(PlaybackTime);

    PlayState get_state() const;
    void set_state(PlayState);

    void play(PlaybackTime a, PlaybackTime b);

    void set_trigger_time(PlaybackTime);
private:
    QString m_filename;
    PlaybackTime m_time = 0;
    PlaybackTime m_trigger_time = 0;
    PlaybackTime m_length = 0;
    PlayState m_state = PlayState::Stopped;

    std::chrono::time_point<std::chrono::system_clock> m_last_set_user_time;
};