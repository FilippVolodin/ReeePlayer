#ifndef JC_SETTINGS_WIDGET_H
#define JC_SETTINGS_WIDGET_H

#include <QWidget>
#include "ui_jc_settings_widget.h"

class JumpCutterSettings;

class JCSettingsWidget : public QWidget
{
    Q_OBJECT

public:
    JCSettingsWidget(QWidget *parent = Q_NULLPTR);
    ~JCSettingsWidget();

    std::shared_ptr<JumpCutterSettings> get_settings() const;
    void set_settings(std::shared_ptr<JumpCutterSettings>);

signals:
    void applied();

private slots:
    void on_nonVoiceVolumeSlider_valueChanged(int value);
    void on_btnApply_clicked();
    void on_btnClose_clicked();
private:
    Ui::JCSettingsWidget ui;
};

#endif