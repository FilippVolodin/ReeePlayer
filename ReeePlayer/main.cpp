// #include "pch.h"
#include <QtWidgets/QApplication>
#include "models/app.h"
#include "widgets/mainwindow.h"

#include "qsubtitles.h"
#include "html.h"
#include "webvtt.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    App app;
    a.setQuitOnLastWindowClosed(false);
    MainWindow w(&app);

    w.show();
    return a.exec();
}
