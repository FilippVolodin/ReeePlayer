#pragma once

#include <QObject>
#include <time_types.h>

enum class PlaybackEventSource : int { User, Player };

class PlaybackPresenter : public QObject
{
    Q_OBJECT

signals:
    void file_changed(const QString&);
    void played(PlaybackEventSource);

    void time_changed(PlaybackTime, PlaybackEventSource);
    void length_changed(PlaybackTime);

public:

    QString get_file() const;
    void set_file(const QString&);

    void play(PlaybackEventSource);

    PlaybackTime get_time() const;
    void set_time(PlaybackTime, PlaybackEventSource);

    PlaybackTime get_length() const;
    void set_length(PlaybackTime);

private:
    QString m_filename;
    PlaybackTime m_time = 0;
    PlaybackTime m_length = 0;

    std::chrono::time_point<std::chrono::system_clock> m_last_set_user_time;
};