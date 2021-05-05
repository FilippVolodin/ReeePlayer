#ifndef SPINBOX_H
#define SPINBOX_H

#include <QWidget>
#include "ui_spinbox.h"

class SpinBox : public QWidget
{
    Q_OBJECT

public:
    SpinBox(QWidget *parent = Q_NULLPTR);
    ~SpinBox();

    int value() const;
    void setValue(int, bool block_signals = true);
    void stepBy(int);

    void setMaximum(int);

signals:
    void value_changed(int);

private:

    void on_btn_dec_clicked();
    void on_btn_inc_clicked();

    Ui::SpinBox ui;
};

#endif