#ifndef WAITING_DIALOG_H
#define WAITING_DIALOG_H

#include <QWidget>
#include "ui_waiting_dialog.h"

class WaitingDialog : public QDialog
{
    Q_OBJECT

public:
    WaitingDialog(QWidget *parent = Q_NULLPTR);
    ~WaitingDialog();
public slots:
    void append_info(QString);
private:
    Ui::WaitingDialog ui;

    QFuture<QString> m_log;
};

#endif