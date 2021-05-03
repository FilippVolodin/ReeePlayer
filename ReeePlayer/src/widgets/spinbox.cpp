#include "pch.h"
#include "spinbox.h"

SpinBox::SpinBox(QWidget *parent)
    : QWidget(parent)
{
    ui.setupUi(this);

    connect(ui.btnDec, &QAbstractButton::clicked,
        this, &SpinBox::on_btn_dec_clicked);
    connect(ui.btnInc, &QAbstractButton::clicked,
        this, &SpinBox::on_btn_inc_clicked);
    connect(ui.edtSpinBox, qOverload<int>(&QSpinBox::valueChanged),
        this, &SpinBox::value_changed);
}

SpinBox::~SpinBox()
{
}

int SpinBox::value() const
{
    return ui.edtSpinBox->value();
}

void SpinBox::setValue(int value, bool block_signals)
{
    if (block_signals)
        ui.edtSpinBox->blockSignals(true);

    ui.edtSpinBox->setValue(value);

    if (block_signals)
        ui.edtSpinBox->blockSignals(false);
}

void SpinBox::stepBy(int steps)
{
    ui.edtSpinBox->stepBy(steps);
}

void SpinBox::setMaximum(int value)
{
    ui.edtSpinBox->setMaximum(value);
}

void SpinBox::on_btn_dec_clicked()
{
    bool ctrl = qApp->keyboardModifiers() & Qt::ControlModifier;
    stepBy(ctrl ? -1 : -4);
}

void SpinBox::on_btn_inc_clicked()
{
    bool ctrl = qApp->keyboardModifiers() & Qt::ControlModifier;
    stepBy(ctrl ? 1 : 4);
}

//void SpinBox::on_value_changed(int)
//{
//}
