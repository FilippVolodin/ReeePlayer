#pragma once

class SubsComboBox : public QComboBox
{
    Q_OBJECT

public:
    SubsComboBox(QWidget* parent = Q_NULLPTR);
protected:
    void paintEvent(QPaintEvent*);
};

