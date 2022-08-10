#include "splash_screen.h"

SplashScreen::SplashScreen(const QPixmap& pixmap, Qt::WindowFlags f)
    : QSplashScreen(pixmap, f)
{
}

void SplashScreen::mousePressEvent(QMouseEvent* event)
{
    QSplashScreen::mousePressEvent(event);
    emit clicked();
}
