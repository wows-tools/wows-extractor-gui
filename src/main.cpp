#include <QApplication>
#include <QSurfaceFormat>
#include "mainwindow.h"

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

    MainWindow w;
    w.show();
    return app.exec();
}
