#ifndef WAVEFORM_H
#define WAVEFORM_H

#include <qwidget.h>

class JumpCutter;

class Waveform : public QWidget
{
public:
    Waveform(QWidget* parent = Q_NULLPTR);
    void set_jumpcutter(const JumpCutter*);
    void set_time(int);
protected:
    void paintEvent(QPaintEvent *event) override;
private:
    const JumpCutter* m_jc = nullptr;
    int m_time = 0;
};

#endif // !GRAPHIC_H
