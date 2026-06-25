#include "mainwindow.h"
#include "calibrator.h"
#include <QApplication>
#include <QScreen>
#include <QWindow>

int main(int argc, char** argv){
    QApplication a(argc,argv);

    // Override xdg_toplevel app_id via desktopFileName (takes priority over applicationName)
    QByteArray appId = qgetenv("APP_ID");
    if (!appId.isEmpty()) {
        QGuiApplication::setDesktopFileName(QString::fromLocal8Bit(appId));
        QCoreApplication::setApplicationName(QString::fromLocal8Bit(appId));
    }

    bool calibMode=false;
    for(int i=1;i<argc;i++) if(QString(argv[i])=="--calibrate") calibMode=true;
    if(calibMode){
        Calibrator c; c.setWindowFlags(c.windowFlags() | Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint);
        c.exec();
        return 0;
    }

    MainWindow w;
    w.showFullScreen();
    return a.exec();
}
