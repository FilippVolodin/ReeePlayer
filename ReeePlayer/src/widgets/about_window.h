#ifndef ABOUT_WINDOW_H
#define ABOUT_WINDOW_H

#include "ui_about_window.h"

class AboutWindow : public QDialog
{
    Q_OBJECT

public:
    AboutWindow(QWidget *parent = Q_NULLPTR);
    ~AboutWindow();
private:
    Ui::AboutWindow ui;
};

#endif