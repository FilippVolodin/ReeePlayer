// #include "pch.h"
#include "jumpcutter_settings_dialog.h"
#include "models/jumpcutter.h"

bool is_speed_equal(float speed1, float speed2)
{
    return std::abs(speed1 - speed2) < 0.1;
}

constexpr float SILENCE_SPEEDS[] = { 0.0, 0.5, 1.01, 1.5, 2.0, 3.0, 5.0 };

JumpCutterSettingsDialog::JumpCutterSettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    ui.setupUi(this);

    ui.cmbNonVoiceSpeed->clear();
    std::for_each(std::begin(SILENCE_SPEEDS), std::end(SILENCE_SPEEDS),
        [this](float speed)
        {
            if (is_speed_equal(speed, 0.0))
                ui.cmbNonVoiceSpeed->addItem("Skip non-voice", 0.0);
            else
                ui.cmbNonVoiceSpeed->addItem(QString::number(speed, 'f', 1), speed);
        }
    );
}

JumpCutterSettingsDialog::~JumpCutterSettingsDialog()
{
}

std::shared_ptr<JumpCutterSettings> JumpCutterSettingsDialog::get_settings() const
{
    std::shared_ptr<JumpCutterSettings> settings = std::make_shared<JumpCutterSettings>();
    settings->set_volume_threshold(ui.edtVoiceProbTh->value() * 0.01);
    settings->set_min_silence_interval(ui.edtMinSilenceInterval->value());
    settings->set_margin_before(ui.edtMarginBefore->value());
    settings->set_margin_after(ui.edtMarginAfter->value());
    settings->set_silence_speed(ui.cmbNonVoiceSpeed->currentData().toFloat());
    return settings;
}

void JumpCutterSettingsDialog::set_settings(std::shared_ptr<JumpCutterSettings> settings) const
{
    ui.edtVoiceProbTh->setValue(static_cast<int>(settings->get_volume_threshold() * 100));
    ui.edtMinSilenceInterval->setValue(settings->get_min_silence_interval());
    ui.edtMarginBefore->setValue(settings->get_margin_before());
    ui.edtMarginAfter->setValue(settings->get_margin_after());
    float speed = settings->get_silence_speed();
    auto it = std::find_if(std::begin(SILENCE_SPEEDS), std::end(SILENCE_SPEEDS),
        [speed](float speed_item) -> bool
        {
            return is_speed_equal(speed, speed_item);
        }
    );

    if (it != std::end(SILENCE_SPEEDS))
        ui.cmbNonVoiceSpeed->setCurrentIndex(std::distance(std::begin(SILENCE_SPEEDS), it));
    else
        ui.cmbNonVoiceSpeed->setCurrentIndex(0);
}
