#include <vad_module.h>
#include <audio_tools.h>
#include <clip_storage.h>
#include <vad.h>
#include <jumpcutter.h>
#include <app.h>

#include <jc_settings_widget.h>

#include <ui_player_window.h>

VADModule::VADModule(App* app, AudioTools* audio_tools,
    ModeMediator* mode_mediator, PlaybackMediator* playback_mediator)
    : m_app(app), m_audio_tools(audio_tools), m_mode_mediator(mode_mediator), m_playback_mediator(playback_mediator)
{
    if (mode_mediator->get_mode() == PlayerWindowMode::WatchingClip ||
        mode_mediator->get_mode() == PlayerWindowMode::Repeating)
        return;

    load_jc_settings();

    connect(m_mode_mediator, &ModeMediator::mode_changed, this, &VADModule::set_mode);

    connect(m_playback_mediator, &PlaybackMediator::file_changed, this, &VADModule::set_file);
    connect(m_playback_mediator, &PlaybackMediator::time_changed, this, &VADModule::set_time);

    connect(m_audio_tools, &AudioTools::waveform_is_ready,
        [this](WaveformPtr waveform)
        {
            m_waveform = waveform;
        });

    connect(m_audio_tools, &AudioTools::vad_is_ready,
        [this](VADPtr vad)
        {
            m_vad = vad;
            m_vad->apply_settings(get_vad_settings());
        });
}

void VADModule::setup_player(Ui_PlayerWindow* pw)
{
    m_pw = pw;
    //if (m_vad)
    //{
    bool jc_enabled = m_app->get_setting("jumpcutter", "activated", true).toBool();
    m_jc_settings->set_activated(jc_enabled);
    //}
    //else
    //{
    //    m_jc_settings->set_enabled(false);
    //}

    m_jc_settings_widget = new JCSettingsWidget(pw->centralwidget);
    m_jc_settings_widget->set_settings(m_jc_settings);
    connect(m_jc_settings_widget, &JCSettingsWidget::applied,
        [this]()
        {
            m_jc_settings = m_jc_settings_widget->get_settings();
            if (m_vad)
                m_vad->apply_settings(get_vad_settings());

            if (!m_jc_settings->is_activated())
            {
                m_playback_mediator->set_overridden_rate(std::nullopt);
                // TODO volume
            }
        });
    pw->dockJC->setWidget(m_jc_settings_widget);

    bool show_vad_setting = m_app->get_setting("gui", "show_vad_settings", true).toBool();
    pw->dockJC->setVisible(show_vad_setting);
    pw->dockJC->widget()->setEnabled(false);

    connect(m_audio_tools, &AudioTools::waveform_is_ready,
        [this](WaveformPtr waveform)
        {
            m_pw->waveform->set_waveform(m_waveform.get());
            update_waveform_ui();
        });

    connect(m_audio_tools, &AudioTools::vad_is_ready,
        [this](VADPtr vad)
        {
            m_pw->dockJC->widget()->setEnabled(true);
            m_pw->waveform->set_vad(m_vad.get());
        });
    m_audio_tools->request();

    startTimer(10);
}

void VADModule::timerEvent(QTimerEvent* event)
{
    if (m_waveform && m_playback_mediator->get_state() == PlayState::Playing)
    {
        int time = m_playback_mediator->get_precision_time();
        m_pw->waveform->set_time(time);
        m_pw->waveform->repaint();
    }
}

void VADModule::set_mode(PlayerWindowMode mode)
{
    m_pw->waveform->setVisible(mode == PlayerWindowMode::Watching);
    if (mode == PlayerWindowMode::Closing)
    {
        save_jc_settings();
    }
}

void VADModule::set_file(const File* file)
{
    if (m_mode_mediator->is_film_mode())
    {
        // m_audio_tools = std::make_unique<AudioTools>(file->get_path());

    }
}

void VADModule::set_time(PlaybackTime time)
{
    if (m_mode_mediator->get_mode() != PlayerWindowMode::Watching)
        return;

    if (!m_vad || !m_jc_settings || !m_jc_settings->is_activated())
        return;

    bool current_interval_is_loud = m_vad->is_voice(time);
    int next_interval = m_vad->next_interval(time);

    if (next_interval - time < 300)
        return;

    if (m_jc_settings->is_non_voice_skipping())
    {
        m_playback_mediator->set_overridden_rate(std::nullopt);
        // m_video_widget->set_volume(100);
        if (current_interval_is_loud)
        {
            // TODO
            m_playback_mediator->set_trigger_time(next_interval, TimerAction::DoNothing);
        }
        else
        {
            m_playback_mediator->set_time(next_interval);
        }
    }
    else
    {
        // TODO
        m_playback_mediator->set_trigger_time(next_interval, TimerAction::DoNothing);
        if (current_interval_is_loud)
        {
            m_playback_mediator->set_overridden_rate(std::nullopt);
            // m_video_widget->set_volume(100);
        }
        else
        {
            m_playback_mediator->set_overridden_rate(m_jc_settings->get_non_voice_speed());
            // m_video_widget->set_volume(m_jc_settings->get_non_voice_volume());
        }
    }
}

void VADModule::load_jc_settings()
{
    m_jc_settings = std::make_shared<JumpCutterSettings>();
    m_jc_settings->set_activated(m_app->get_setting("jumpcutter", "activated", true).toBool());
    m_jc_settings->set_voice_prob_th(m_app->get_setting("jumpcutter", "voice_prob_th", 0.5).toFloat());
    m_jc_settings->set_non_voice_volume(m_app->get_setting("jumpcutter", "non_voice_volume", 100).toFloat());
    m_jc_settings->set_non_voice_speed(m_app->get_setting("jumpcutter", "non_voice_speed", 2.0).toFloat());
    m_jc_settings->set_min_non_voice_interval(m_app->get_setting("jumpcutter", "min_non_voice_interval", 500).toInt());
    m_jc_settings->set_margin_before(m_app->get_setting("jumpcutter", "margin_before", 100).toFloat());
    m_jc_settings->set_margin_after(m_app->get_setting("jumpcutter", "margin_after", 100).toFloat());
}

void VADModule::save_jc_settings()
{
    m_app->set_setting("jumpcutter", "activated", m_jc_settings->is_activated());
    m_app->set_setting("jumpcutter", "voice_prob_th", m_jc_settings->get_voice_prob_th());
    m_app->set_setting("jumpcutter", "non_voice_volume", m_jc_settings->get_non_voice_volume());
    m_app->set_setting("jumpcutter", "non_voice_speed", m_jc_settings->get_non_voice_speed());
    m_app->set_setting("jumpcutter", "min_non_voice_interval", m_jc_settings->get_min_non_voice_interval());
    m_app->set_setting("jumpcutter", "margin_before", m_jc_settings->get_margin_before());
    m_app->set_setting("jumpcutter", "margin_after", m_jc_settings->get_margin_after());
}

std::shared_ptr<VADSettings> VADModule::get_vad_settings() const
{
    std::shared_ptr<VADSettings> vad_settings = std::make_shared<VADSettings>();
    vad_settings->set_min_non_voice_interval(m_jc_settings->get_min_non_voice_interval());
    vad_settings->set_margin_after(m_jc_settings->get_margin_after());
    vad_settings->set_margin_before(m_jc_settings->get_margin_before());
    vad_settings->set_voice_prob(m_jc_settings->get_voice_prob_th() * 256);
    return vad_settings;
}

void VADModule::update_waveform_ui()
{
    if (m_waveform)
    {
        bool show_waveform = m_app->get_setting("gui", "show_waveform", true).toBool();
        m_pw->actShowWaveform->setEnabled(true);
        m_pw->actShowWaveform->setChecked(show_waveform);
        m_pw->waveform->setVisible(show_waveform);
    }
    else
    {
        m_pw->actShowWaveform->setEnabled(false);
        m_pw->actShowWaveform->setChecked(false);
        m_pw->waveform->setVisible(false);
    }

}
