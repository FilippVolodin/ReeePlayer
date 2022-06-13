#ifndef EXPORT_DIALOG_H
#define EXPORT_DIALOG_H

#include "ui_export_dialog.h"

class ExportDialog : public QDialog
{
    Q_OBJECT

public:
    ExportDialog(QWidget *parent = Q_NULLPTR);
    ~ExportDialog();

    void set_info_text(const QString&);

    QString get_deck_name() const;
    bool is_screenshot_included() const;
    bool is_audio_included() const;

private slots:
    void on_btnExport_clicked();
    void on_btnCancel_clicked();

private:
    Ui::ExportDialog ui;
};

#endif