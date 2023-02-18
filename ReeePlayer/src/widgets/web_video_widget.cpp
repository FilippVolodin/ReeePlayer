#include "web_video_widget.h"
#include "emitter.h"

#include <QWebEngineSettings>

WebVideoWidget::WebVideoWidget(QWidget* parent)
    : QWebEngineView(parent)
{
    settings()->setAttribute(QWebEngineSettings::PlaybackRequiresUserGesture, false);

    m_emitter = std::make_unique<Emitter>();

    QFile html_file(":/player.html");
    if (!html_file.open(QIODevice::ReadOnly))
        return;

    QEventLoop loop_load;
    QObject::connect(this, &QWebEngineView::loadFinished, &loop_load, &QEventLoop::quit);
    this->setContent(html_file.readAll(), "text/html;charset=UTF-8", QUrl("file://"));
    loop_load.exec();

    m_destroying = false;
    startTimer(10);
}

WebVideoWidget::~WebVideoWidget()
{
}

void WebVideoWidget::set_file_name(const QString& file_name, bool)
{
    if (m_file_name == file_name)
        return;

    m_file_name = file_name;

    page()->runJavaScript(QString("set_source(\"file:///%1\");").arg(file_name));
}

void WebVideoWidget::play()
{
    page()->runJavaScript("play();");
}

void WebVideoWidget::play(int start_time, int stop_time, int repeats)
{
    page()->runJavaScript(QString("seek_and_play(%1);").arg(start_time * 0.001),
        [this, stop_time](const QVariant&) {set_timer(stop_time); });
}

void WebVideoWidget::set_timer(int stop_time)
{
    //if (m_time < stop_time)
        m_trigger_time = stop_time;
}

void WebVideoWidget::pause()
{
    page()->runJavaScript("pause();");
}

void WebVideoWidget::toggle_pause()
{
}

void WebVideoWidget::stop()
{
    page()->runJavaScript("pause();");
}

bool WebVideoWidget::is_playing() const
{
    return m_playing;
}

bool WebVideoWidget::is_stopped() const
{
    return true;
}

void WebVideoWidget::set_time(int value)
{
    page()->runJavaScript(QString("set_time(%1);").arg(value * 0.001));
}

float WebVideoWidget::get_rate() const
{
    return 1.0;
}

void WebVideoWidget::set_rate(float rate)
{
    if (std::abs(m_rate - rate) > 0.01)
    {
        qDebug("RATE: %f (%d)", rate, m_time);
        m_rate = rate;
        page()->runJavaScript(QString("set_rate(%1);").arg(m_rate));
    }
}

void WebVideoWidget::set_volume(int volume)
{
    if (m_volume != volume)
    {
        m_volume = volume;
        page()->runJavaScript(QString("set_volume(%1);").arg(m_volume * 0.01));
    }
}

int WebVideoWidget::get_time() const
{
    return m_time;
}

int WebVideoWidget::get_accuracy_time() const
{
    return m_time;
}

int WebVideoWidget::get_length() const
{
    return m_duration;
}

bool WebVideoWidget::at_end() const
{
    return false;
}

void WebVideoWidget::set_audio_track(int track_index)
{
    page()->runJavaScript(QString("set_audio_track(%1);").arg(track_index));
}

void WebVideoWidget::prepare_to_destroy()
{
    if (m_counter.count() != 0)
    {
        m_counter.set_destroying(true);
        QEventLoop loop;
        QObject::connect(&m_counter, &RunningScriptsCounter::can_be_destroyed, &loop, &QEventLoop::quit);
        loop.exec();
    }
}

QWidget* WebVideoWidget::get_widget()
{
    return this;
}

Emitter* WebVideoWidget::get_emitter()
{
    return m_emitter.get();
}

void WebVideoWidget::timerEvent(QTimerEvent*)
{
    runJS("playing();", [&](const QVariant& v) { m_playing = v.toBool(); });

    auto h_get_time = [&](const QVariant& v)
    {
        if (m_destroying)
            return;
        int time = v.toFloat() * 1000;
        if (time != m_time)
        {
            m_time = time;

            if (std::abs(m_time - m_last_emitted_time) > 500)
            {
                emit m_emitter->time_changed(m_time);
                m_last_emitted_time = m_time;
            }

            if (m_trigger_time != -1 && m_time > m_trigger_time)
            {
                qDebug("TIMER: %d (%d)", m_trigger_time, m_time);

                emit m_emitter->timer_triggered(m_time);
                m_trigger_time = -1;
            }
        }
    };
    runJS("get_time();", h_get_time);

    if (m_duration == 0)
    {
        auto h_get_duration = [&](const QVariant& v)
        {
            if (m_destroying)
                return;
            m_duration = v.toFloat() * 1000;
            if (m_duration != 0)
                emit m_emitter->length_changed(m_duration);
        };
        runJS("get_duration();", h_get_duration);
    }
}

void WebVideoWidget::runJS(const QString& script, const std::function<void(const QVariant&)>& callback)
{
    if (m_counter.is_destroing())
        return;
    m_counter.inc();
    auto cb = [callback, this](const QVariant& v)
    {
        callback(v);
        this->m_counter.dec();
    };
    page()->runJavaScript(script, cb);
}
