#include "pch.h"
#include "models/app.h"
#include "widgets/mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    App app;
    a.setQuitOnLastWindowClosed(false);
    MainWindow w(&app);

    w.show();
    return a.exec();
}
