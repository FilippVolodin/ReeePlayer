#ifndef JUMPCUTTER_SETTINGS_DIALOG_H
#define JUMPCUTTER_SETTINGS_DIALOG_H

#include <QWidget>
#include "ui_jumpcutter_settings_dialog.h"

class JumpCutterSettings;

class JumpCutterSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    JumpCutterSettingsDialog(QWidget *parent = Q_NULLPTR);
    ~JumpCutterSettingsDialog();

    std::shared_ptr<JumpCutterSettings> get_settings() const;
    void set_settings(std::shared_ptr<JumpCutterSettings>) const;

signals:
    void applied();

private slots:
    void on_nonVoiceVolumeSlider_valueChanged(int value);
    void on_btnApply_clicked();
    void on_btnClose_clicked();
private:
    Ui::JumpCutterSettingsDialog ui;
};

#endif