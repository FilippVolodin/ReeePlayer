// #include "pch.h"
#include "waiting_dialog.h"

WaitingDialog::WaitingDialog(QWidget *parent)
    : QDialog(parent)
{
    ui.setupUi(this);
}

WaitingDialog::~WaitingDialog()
{
}

void WaitingDialog::append_info(QString info)
{
    ui.edtInfo->appendPlainText(info);
}
