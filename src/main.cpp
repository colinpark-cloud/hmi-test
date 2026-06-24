#include "mainwindow.h"
#include "calibrator.h"
#include <QApplication>
#include <QScreen>
#include <QWindow>

static QScreen *targetScreen()
{
    QScreen *fallback = QGuiApplication::primaryScreen();
    for (QScreen *screen : QGuiApplication::screens()) {
        if (!screen) {
            continue;
        }
        if (screen->name().contains("HDMI", Qt::CaseInsensitive)) {
            return screen;
        }
        if (!fallback || screen->geometry().size().width() * screen->geometry().size().height() >
                         fallback->geometry().size().width() * fallback->geometry().size().height()) {
            fallback = screen;
        }
    }
    return fallback;
}

int main(int argc, char** argv){
    QApplication a(argc,argv);
    bool calibMode=false;
    for(int i=1;i<argc;i++) if(QString(argv[i])=="--calibrate") calibMode=true;
    if(calibMode){
        Calibrator c; c.setWindowFlags(c.windowFlags() | Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint);
        c.exec();
        return 0;
    }
    MainWindow w;
    if (QScreen *screen = targetScreen()) {
        w.winId();
        if (w.windowHandle()) {
            w.windowHandle()->setScreen(screen);
        }
        w.setGeometry(screen->geometry());
    }
    w.show();
    return a.exec();
}
