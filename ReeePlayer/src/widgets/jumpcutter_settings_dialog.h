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
private:
    Ui::JumpCutterSettingsDialog ui;
};

#endif