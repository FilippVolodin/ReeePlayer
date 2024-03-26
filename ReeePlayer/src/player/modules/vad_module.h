#pragma once

#include <QObject>
#include <time_types.h>
#include <mode_mediator.h>
#include <playback_mediator.h>

class App;
class AudioTools;
class VAD;
using Waveform = std::vector<uint8_t>;
class JumpCutterSettings;
class VADSettings;

class JCSettingsWidget;

class Ui_PlayerWindow;

class VADModule : public QObject
{
public:
    VADModule(App* app, ModeMediator*, PlaybackMediator*);
    void setup_player(Ui_PlayerWindow* player_window);

private:
    void set_mode(PlayerWindowMode);

    void set_file(const File*);

    void load_jc_settings();
    void save_jc_settings();
    std::shared_ptr<VADSettings> get_vad_settings() const;
    void update_waveform_ui();

    App* m_app = nullptr;
    ModeMediator* m_mode_mediator = nullptr;
    PlaybackMediator* m_playback_mediator = nullptr;

    Ui_PlayerWindow* m_pw = nullptr;

    std::unique_ptr<AudioTools> m_audio_tools;
    std::shared_ptr<Waveform> m_waveform;
    std::shared_ptr<VAD> m_vad;
    std::shared_ptr<JumpCutterSettings> m_jc_settings;

    JCSettingsWidget* m_jc_settings_widget = nullptr;
};
