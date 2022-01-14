#pragma once
#include <qobject.h>

class Emitter : public QObject
{
    Q_OBJECT
public:
    Emitter();
signals:
    void opening();
    void length_changed(int);
    void time_changed(int);
    void playing();
    void paused();
    void stopped();
    void end_reached();
    void timer_triggered(int64_t);
};

