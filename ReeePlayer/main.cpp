#include "pch.h"
#include "models/app.h"
#include "widgets/mainwindow.h"
#include "models/jumpcutter.h"

int main(int argc, char *argv[])
{
    //read_wav();
    //return 0;

    QApplication a(argc, argv);
    App app;
    a.setQuitOnLastWindowClosed(false);
    MainWindow w(&app);

    w.show();
    return a.exec();
}
