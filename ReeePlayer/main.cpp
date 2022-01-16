#include "pch.h"
#include "models/app.h"
#include "widgets/mainwindow.h"
#include "models/jumpcutter.h"

int main(int argc, char *argv[])
{
    char ARG_DISABLE_WEB_SECURITY[] = "--disable-web-security";
    char ARG_AUTOPLAY_POLICY[] = "--autoplay-policy=no-user-gesture-required";

    int newArgc = argc + 2;
    char** newArgv = new char* [newArgc];
    for (int i = 0; i < argc; i++) {
        newArgv[i] = argv[i];
    }
    newArgv[argc] = ARG_DISABLE_WEB_SECURITY;
    newArgv[argc + 1] = ARG_AUTOPLAY_POLICY;
    //newArgv[argc + 2] = nullptr;

    QApplication a(newArgc, newArgv);
    App app;
    a.setQuitOnLastWindowClosed(false);
    MainWindow w(&app);

    w.show();
    return a.exec();
}
