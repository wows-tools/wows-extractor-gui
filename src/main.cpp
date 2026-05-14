#include <QApplication>
#include <QSurfaceFormat>
#include <QSplashScreen>
#include <QPainter>
#include <QFont>
#include "mainwindow.h"

static QPixmap makeSplashPixmap()
{
    QPixmap pm(480, 220);
    pm.fill(QColor(0x1a, 0x2a, 0x3a));
    QPainter p(&pm);

    QFont title;
    title.setPointSize(26);
    title.setBold(true);
    p.setFont(title);
    p.setPen(Qt::white);
    p.drawText(QRect(0, 50, 480, 60), Qt::AlignCenter, "WoWs Extractor");

    QFont sub;
    sub.setPointSize(12);
    p.setFont(sub);
    p.setPen(QColor(0x88, 0xbb, 0xff));
    p.drawText(QRect(0, 130, 480, 40), Qt::AlignCenter, "Loading game data…");

    return pm;
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setOrganizationName("wows-tools");
    app.setApplicationName("wows-extractor-gui");
    app.setApplicationDisplayName(QStringLiteral("WoWs Extractor"));

    /* request OpenGL ES3 for Qt Quick 3D */
    QSurfaceFormat fmt;
    fmt.setDepthBufferSize(24);
    fmt.setVersion(3, 3);
    fmt.setProfile(QSurfaceFormat::CoreProfile);
    QSurfaceFormat::setDefaultFormat(fmt);

    QSplashScreen splash(makeSplashPixmap(), Qt::WindowStaysOnTopHint);
    splash.show();
    app.processEvents();

    MainWindow w;
    QObject::connect(&w, &MainWindow::gameDataLoaded, &splash, &QSplashScreen::close);
    w.show();
    return app.exec();
}
