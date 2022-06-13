#include "export_dialog.h"

ExportDialog::ExportDialog(QWidget *parent) :
    QDialog(parent)
{
    ui.setupUi(this);

    ui.btnExport->setEnabled(false);
    connect(ui.edtDeckName, &QLineEdit::textChanged,
        [this](const QString& text) { ui.btnExport->setEnabled(!text.isEmpty()); });
}

ExportDialog::~ExportDialog()
{
}

void ExportDialog::set_info_text(const QString& text)
{
    ui.lblInfo->setText(text);
}

QString ExportDialog::get_deck_name() const
{
    return ui.edtDeckName->text();
}

bool ExportDialog::is_screenshot_included() const
{
    return ui.chkScreenshot->isChecked();
}

bool ExportDialog::is_audio_included() const
{
    return ui.chkAudio->isChecked();
}

void ExportDialog::on_btnExport_clicked()
{
    accept();
}

void ExportDialog::on_btnCancel_clicked()
{
    reject();
}
