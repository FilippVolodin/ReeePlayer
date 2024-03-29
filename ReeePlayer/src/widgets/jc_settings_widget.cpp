// #include "pch.h"
#include "jc_settings_widget.h"
#include "models/jumpcutter.h"

static bool is_speed_equal(float speed1, float speed2)
{
    return std::abs(speed1 - speed2) < 0.1;
}

constexpr float SILENCE_SPEEDS[] = { 0.0f, 0.5f, 1.01f, 1.5f, 2.0f, 2.5f, 3.0f, 5.0f };

JCSettingsWidget::JCSettingsWidget(QWidget *parent)
    : QWidget(parent)
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

    connect(ui.chkActivateVAD, &QCheckBox::stateChanged, [&](int) {emit applied(); });
    connect(ui.edtVoiceProbTh, &QSpinBox::valueChanged, [&](int) {emit applied(); });
    connect(ui.nonVoiceVolumeSlider, &QSlider::valueChanged, [&](int) {emit applied(); });
    connect(ui.cmbNonVoiceSpeed, &QComboBox::currentIndexChanged, [&](int) {emit applied(); });
    connect(ui.edtMinSilenceInterval, &QSpinBox::valueChanged, [&](int) {emit applied(); });
    connect(ui.edtMarginBefore, &QSpinBox::valueChanged, [&](int) {emit applied(); });
    connect(ui.edtMarginAfter, &QSpinBox::valueChanged, [&](int) {emit applied(); });
}

JCSettingsWidget::~JCSettingsWidget()
{
}

std::shared_ptr<JumpCutterSettings> JCSettingsWidget::get_settings() const
{
    std::shared_ptr<JumpCutterSettings> settings = std::make_shared<JumpCutterSettings>();
    settings->set_activated(ui.chkActivateVAD->isChecked());
    settings->set_voice_prob_th(ui.edtVoiceProbTh->value() * 0.01);
    settings->set_non_voice_volume(ui.nonVoiceVolumeSlider->value());
    settings->set_min_non_voice_interval(ui.edtMinSilenceInterval->value());
    settings->set_margin_before(ui.edtMarginBefore->value());
    settings->set_margin_after(ui.edtMarginAfter->value());
    settings->set_non_voice_speed(ui.cmbNonVoiceSpeed->currentData().toFloat());
    return settings;
}

void JCSettingsWidget::set_settings(std::shared_ptr<JumpCutterSettings> settings)
{
    if (!settings->is_enabled())
        this->setEnabled(false);

    ui.chkActivateVAD->setChecked(settings->is_activated());
    ui.edtVoiceProbTh->setValue(static_cast<int>(settings->get_voice_prob_th() * 100));
    ui.nonVoiceVolumeSlider->setValue(settings->get_non_voice_volume());
    ui.lblNonVoiceVolume->setText(QString("%1%").arg(settings->get_non_voice_volume(), 3));
    ui.edtMinSilenceInterval->setValue(settings->get_min_non_voice_interval());
    ui.edtMarginBefore->setValue(settings->get_margin_before());
    ui.edtMarginAfter->setValue(settings->get_margin_after());
    float speed = settings->get_non_voice_speed();
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

void JCSettingsWidget::on_btnApply_clicked()
{
    emit applied();
}

void JCSettingsWidget::on_btnClose_clicked()
{
    close();
}

void JCSettingsWidget::on_nonVoiceVolumeSlider_valueChanged(int value)
{
    ui.lblNonVoiceVolume->setText(QString("%1%").arg(value, 3));
}
