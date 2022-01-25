#ifndef WAVEFORM_H
#define WAVEFORM_H

#include <qwidget.h>

class JumpCutter;
class VAD;

class Waveform : public QWidget
{
    Q_OBJECT

public:
    Waveform(QWidget* parent = Q_NULLPTR);
    void set_jumpcutter(const JumpCutter*);
    void set_vad(const VAD*);
    void set_time(int);
    
    bool is_clip_mode(bool) const;
    void set_clip_mode(bool);
    
    int get_clip_a() const;
    void set_clip_a(int);

    int get_clip_b() const;
    void set_clip_b(int);
signals:
    void mouse_release(int time, QMouseEvent*);
    void wheel_event(int time, QWheelEvent* event);
protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    const JumpCutter* m_jc = nullptr;
    const VAD* m_vad = nullptr;
    int m_time = 0;
    int m_clip_a = 0;
    int m_clip_b = 0;
    bool m_clip_mode = false;
    int m_a = 0;
    int m_b = 0;
};

#endif // !GRAPHIC_H
