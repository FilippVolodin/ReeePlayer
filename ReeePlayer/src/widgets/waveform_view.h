#ifndef WAVEFORM_VIEW_H
#define WAVEFORM_VIEW_H

#include <qwidget.h>

class Waveform;
class VAD;

class WaveformView : public QWidget
{
    Q_OBJECT

public:
    WaveformView(QWidget* parent = Q_NULLPTR);
    void set_waveform(const Waveform*);
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
    const Waveform* m_waveform = nullptr;
    const VAD* m_vad = nullptr;
    int m_time = 0;
    int m_clip_a = 0;
    int m_clip_b = 0;
    bool m_clip_mode = false;
    int m_a = 0;
    int m_b = 0;
};

#endif // !GRAPHIC_H
