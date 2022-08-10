#pragma once

#include <QSplashScreen>

class SplashScreen : public QSplashScreen
{
    Q_OBJECT

public:
    SplashScreen(const QPixmap& pixmap = QPixmap(), Qt::WindowFlags f = Qt::WindowFlags());
signals:
    void clicked();
protected:
    virtual void mousePressEvent(QMouseEvent* event) override;
};