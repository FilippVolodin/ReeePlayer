#include <QtWidgets/QApplication>
#include <QFileSystemWatcher>
#include <QDir>
#include <QProcess>
#include <QTimer>

#include "splash_screen.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QPixmap pixmap(":/splash.png");
    SplashScreen splash(pixmap);
    splash.show();

    QProcess reeeplayer;
    reeeplayer.startDetached("ReeePlayer.exe");

    QString filename = QDir::temp().filePath("ReeePlayer.starting");
    QFile file(filename);
    file.open(QIODevice::WriteOnly);
    file.close();

    QFileSystemWatcher watcher({ filename });

    QTimer timer;
    timer.setSingleShot(true);

    QEventLoop loop;
    QObject::connect(&watcher, &QFileSystemWatcher::fileChanged, &loop, &QEventLoop::quit);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(&splash, &SplashScreen::clicked, &loop, &QEventLoop::quit);
    timer.start(60000);
    loop.exec();

    splash.close();
}
