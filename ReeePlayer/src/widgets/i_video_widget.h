#ifndef I_VIDEO_WIDGET_H
#define I_VIDEO_WIDGET_H

class Emitter;

class IVideoWidget
{
public:
    virtual void set_file_name(const QString&, bool auto_play = false) = 0;

    virtual void play() = 0;
    virtual void play(int from, int to, int repeats) = 0;
    virtual void set_timer(int time) = 0;
    virtual void pause() = 0;
    virtual void toggle_pause() = 0;
    virtual void stop() = 0;
    virtual bool is_playing() const = 0;
    virtual bool is_stopped() const = 0;
    virtual void set_time(int) = 0;
    virtual float get_rate() const = 0;
    virtual void set_rate(float) = 0;

    virtual int get_time() const = 0;
    virtual int get_accuracy_time() const = 0;
    virtual int get_length() const = 0;
    virtual bool at_end() const = 0;

    virtual QWidget* get_widget() = 0;
    virtual Emitter* get_emitter() = 0;
};

#endif // !I_VIDEO_WIDGET_H
