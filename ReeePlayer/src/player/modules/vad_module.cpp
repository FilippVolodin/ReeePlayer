#include <vad_module.h>
#include <audio_tools.h>
#include <clip_storage.h>
#include <vad.h>
#include <jumpcutter.h>
#include <app.h>

#include <jc_settings_widget.h>

#include <ui_player_window.h>

VADModule::VADModule(App* app, ModeMediator* mode_mediator, PlaybackMediator* playback_mediator)
    : m_app(app), m_mode_mediator(mode_mediator), m_playback_mediator(playback_mediator)
{
    connect(m_mode_mediator, &ModeMediator::mode_changed, this, &VADModule::set_mode);

    connect(m_playback_mediator, &PlaybackMediator::file_changed, this, &VADModule::set_file);
}

void VADModule::setup_player(Ui_PlayerWindow* pw)
{
    m_pw = pw;

    load_jc_settings();
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
        });
    pw->dockJC->setWidget(m_jc_settings_widget);

    bool show_vad_setting = m_app->get_setting("gui", "show_vad_settings", true).toBool();
    pw->dockJC->setVisible(show_vad_setting);
    pw->dockJC->widget()->setEnabled(false);
}

void VADModule::set_mode(PlayerWindowMode mode)
{
    if (mode == PlayerWindowMode::Closing)
    {
        save_jc_settings();
    }
}

void VADModule::set_file(const File* file)
{
    if (m_mode_mediator->is_film_mode())
    {
        m_audio_tools = std::make_unique<AudioTools>(file->get_path());
        connect(m_audio_tools.get(), &AudioTools::waveform_is_ready,
            [this](WaveformPtr waveform)
            {
                m_waveform = waveform;
                m_pw->waveform->set_waveform(m_waveform.get());
                update_waveform_ui();
            });

        connect(m_audio_tools.get(), &AudioTools::vad_is_ready,
            [this](VADPtr vad)
            {
                m_vad = vad;
                m_vad->apply_settings(get_vad_settings());

                m_pw->dockJC->widget()->setEnabled(true);

                m_pw->waveform->set_vad(m_vad.get());
            });
        m_audio_tools->request();
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
