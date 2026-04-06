#include "mainwindow.h"
#include "calibrator.h"
#include <QApplication>
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
    // avoid requesting fullscreen from compositor; use fixed size matching panel
    w.setFixedSize(1024,600);
    w.show();
    return a.exec();
}
