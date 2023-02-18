#ifndef WEB_VIDEO_WIDGET_H
#define WEB_VIDEO_WIDGET_H

#include "i_video_widget.h"

class Emitter;

class RunningScriptsCounter : public QObject
{
    Q_OBJECT
public:
    void inc() { m_count++; };
    void dec() { m_count--; if (m_count == 0 && m_destroying) emit can_be_destroyed(); };
    int count() { return m_count; }
    bool is_destroing() const { return m_destroying; }
    void set_destroying(bool value) { m_destroying = value; };
signals:
    void can_be_destroyed();
private:
    int m_count = 0;
    bool m_destroying = false;
};

class WebVideoWidget : public QWebEngineView, public IVideoWidget
{
public:
    WebVideoWidget(QWidget* parent);
    ~WebVideoWidget();

    void set_file_name(const QString&, bool auto_play = false) override;

    void play() override;
    void play(int from, int to, int repeats) override;
    void set_timer(int time) override;
    void pause() override;
    void toggle_pause() override;
    void stop() override;
    bool is_playing() const override;
    bool is_stopped() const override;
    void set_time(int) override;
    float get_rate() const override;
    void set_rate(float) override;
    void set_volume(int);

    int get_time() const override;
    int get_accuracy_time() const override;
    int get_length() const override;
    bool at_end() const override;

    void set_audio_track(int) override;

    void prepare_to_destroy() override;

    QWidget* get_widget() override;
    Emitter* get_emitter() override;
protected:
    void timerEvent(QTimerEvent*) override;
private:
    void runJS(const QString& script, const std::function<void(const QVariant&)>& callback);

    QString m_file_name;

    std::atomic<bool> m_destroying;
    std::unique_ptr<Emitter> m_emitter;
    bool m_playing = false;
    int m_time = 0;
    int m_duration = 0;
    int m_trigger_time = -1;
    float m_rate = 1.0;
    int m_volume = 100;
    int m_last_emitted_time = 0.0;
    //float m_cur_rate = 1.0;
    RunningScriptsCounter m_counter;
};

class DoneNotifier : public QObject
{
    Q_OBJECT
public:
    void set_done() { emit done(); };
signals:
    void done();
};



#endif // !WEB_VIDEO_WIDGET_H
