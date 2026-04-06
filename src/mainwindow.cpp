#include "mainwindow.h"
#include <QTabWidget>
#include <QTabBar>
#include <QVBoxLayout>
#include <QWidget>
#include <QPushButton>
#include <QApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QMouseEvent>
#include <QProcess>
#include <QMessageBox>
#include "gpiotest.h"
#include "serialtest.h"
#include "storagetest.h"
#include "displaytest.h"
#include "perftest.h"
#include "calibrator.h"
#include "gltest.h"
#include "car3d.h"
#include "smartoven.h"

MainWindow::MainWindow(QWidget* parent): QMainWindow(parent){
    auto tabs = new QTabWidget(this);
    tabs->addTab(new DisplayTest, "Display & Touch");
    tabs->addTab(new GPIOTest, "GPIO / Buzzer");
    tabs->addTab(new SerialTest, "Serial (RS232/485)");
    tabs->addTab(new StorageTest, "Storage");
    tabs->addTab(new PerfTest, "Performance");
    // add 3D demo tab (use Qt3D Car widget when available)
    QWidget* carTab = new QWidget;
    auto carLayout = new QVBoxLayout(carTab);
    Car3DWidget* car3d = nullptr;
    bool haveQt3D = true; // assume we built with Qt3D
    try{
        car3d = new Car3DWidget;
    }catch(...){ car3d = nullptr; haveQt3D = false; }
    if(car3d){
        car3d->setMinimumSize(400,300);
        carLayout->addWidget(car3d);
    }else{
        // fallback to RotGLWidget if Qt3D isn't available at runtime
        auto rot = new RotGLWidget;
        rot->setMinimumSize(400,300);
        carLayout->addWidget(rot);
    }
    auto palRow = new QHBoxLayout;
    QString colors[] = {"#c62828","#1565c0","#2e7d32","#f9a825","#ffffff"};
    for(auto &c : colors){
        auto b = new QPushButton;
        b->setStyleSheet(QString("background:%1").arg(c));
        b->setFixedSize(48,48);
        palRow->addWidget(b);
        QColor col(c);
        if(car3d){ connect(b,&QPushButton::clicked,[car3d,col](){ car3d->setColor(col); }); }
        else{ connect(b,&QPushButton::clicked,[=](){ if(auto w = carTab->findChild<RotGLWidget*>() ) { w->setCarColor(col); w->update(); } }); }
    }
    carLayout->addLayout(palRow);
    // Launch external glmark2 (replace earlier cinematic-demo). Provide launch + cancel buttons and allow auto-launch when tab selected.
    auto launchBtn = new QPushButton("Run glmark2");
    launchBtn->setFixedHeight(48);
    carLayout->addWidget(launchBtn);
    m_launchBtn = launchBtn;

    auto cancelBtn = new QPushButton("Cancel 3D");
    cancelBtn->setFixedHeight(48);
    carLayout->addWidget(cancelBtn);

    connect(launchBtn, &QPushButton::clicked, this, [this]() {
        // Debug: log attempts so we can verify from target
        auto logAttempt = [&](const QString &msg){
            QFile lf("/tmp/hmi3d_launcher.log");
            if(lf.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)){
                QTextStream ts(&lf);
                ts << QDateTime::currentDateTime().toString(Qt::ISODate) << " " << msg << "\n";
                lf.close();
            }
        };

        QString exe = "/usr/local/bin/glmark2";
        logAttempt(QString("button-pressed: trying-exe=%1").arg(exe));
        if (QFile::exists(exe) && (QFile::permissions(exe) & QFileDevice::ExeUser)) {
            logAttempt("button-pressed: minimizing main window before launch");
            this->setWindowState(this->windowState() | Qt::WindowMinimized);

            // If we have an existing demo process, try to stop it first
            if (m_demoProc && m_demoProc->state() != QProcess::NotRunning) {
                m_demoProc->terminate();
                m_demoProc->waitForFinished(500);
                delete m_demoProc; m_demoProc = nullptr;
            }

            m_demoProc = new QProcess(this);
            m_demoProc->setProcessChannelMode(QProcess::MergedChannels);
            // Prepare environment for GUI Wayland session
            QProcessEnvironment penv = QProcessEnvironment::systemEnvironment();
            penv.insert("XDG_RUNTIME_DIR", "/run/user/1201");
            penv.insert("WAYLAND_DISPLAY", "wayland-1");
            penv.insert("GLMARK2_DATA_DIR", "/usr/local/share/glmark2");
            m_demoProc->setProcessEnvironment(penv);

            bool started = false;
            // Start in windowed mode only (do not fall back to fullscreen)
            QStringList winArgs;
            winArgs << "--annotate" << "--size=800x600";
            m_demoProc->start(exe, winArgs);
            if (m_demoProc->waitForStarted(2000)) {
                started = true;
                logAttempt("glmark2 started (windowed 800x600)");
            } else {
                logAttempt("windowed start failed — not falling back to fullscreen");
            }

            // When demo finishes, restore HMI window and write a small marker to log
            connect(m_demoProc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [this](int, QProcess::ExitStatus){
                this->setWindowState(this->windowState() & ~Qt::WindowMinimized);
                QFile lf("/mnt/data/home/user/glmark2_run.log");
                if(lf.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)){
                    QTextStream ts(&lf);
                    ts << QDateTime::currentDateTime().toString(Qt::ISODate) << " glmark2 exited\n";
                    lf.close();
                }
                if (m_demoProc) { m_demoProc->deleteLater(); m_demoProc = nullptr; }
            });

            // forward output to persistent log for diagnostics
            connect(m_demoProc, &QProcess::readyReadStandardOutput, this, [this](){
                if (!m_demoProc) return;
                QByteArray out = m_demoProc->readAllStandardOutput();
                QFile lf("/mnt/data/home/user/glmark2_run.log");
                if(lf.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)){
                    lf.write(out);
                    lf.close();
                }
            });

            if (!started) {
                QFile lf("/mnt/data/home/user/glmark2_run.log");
                if(lf.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)){
                    QTextStream ts(&lf);
                    ts << QDateTime::currentDateTime().toString(Qt::ISODate) << " failed to start glmark2\n";
                    lf.close();
                }
                QMessageBox::warning(this, "glmark2", QString("Failed to start %1").arg(exe));
                this->setWindowState(this->windowState() & ~Qt::WindowMinimized);
                delete m_demoProc; m_demoProc = nullptr;
            }
            return;
        }
        // Fallback: try PATH-resolved names via detached start
        bool launched = false;
        for (const QString &name : QStringList{"glmark2-es2-wayland", "glmark2-es2", "glmark2"}) {
            logAttempt(QString("button-pressed: trying-path-name=%1").arg(name));
            if (QProcess::startDetached(name)) { launched = true; logAttempt(QString("startDetached: name=%1 ok=1").arg(name)); break; }
            else logAttempt(QString("startDetached: name=%1 ok=0").arg(name));
        }
        if (!launched) {
            logAttempt("startDetached: no-launch");
            QMessageBox::warning(this, "glmark2", "glmark2 not found on system. Install glmark2 or place the binary under /usr/local/bin or /usr/bin");
        }
    });

    connect(cancelBtn, &QPushButton::clicked, this, [this]() {
        if (m_demoProc && m_demoProc->state() != QProcess::NotRunning) {
            m_demoProc->terminate();
            if (!m_demoProc->waitForFinished(1000)) m_demoProc->kill();
            delete m_demoProc; m_demoProc = nullptr;
            // restore window
            this->setWindowState(this->windowState() & ~Qt::WindowMinimized);
        } else {
            // best-effort: try to kill detached glmark2 processes (not ideal on BusyBox targets)
            QProcess::startDetached("/bin/sh", QStringList() << "-c" << "pkill -f glmark2 || true");
        }
    });

    int carIndex = tabs->addTab(carTab, "3D Demo");
    m_carIndex = carIndex;
    m_tabBar = tabs->tabBar();
    if (m_tabBar) {
        m_tabBar->installEventFilter(this);
        // create an invisible overlay button covering the tab bar to reliably capture touch on header
        QPushButton* headerOverlay = new QPushButton(m_tabBar);
        headerOverlay->setObjectName("tabHeaderOverlay");
        headerOverlay->setFlat(true);
        headerOverlay->setText("");
        headerOverlay->setStyleSheet("background:transparent; border:none;");
        headerOverlay->setGeometry(m_tabBar->rect());
        headerOverlay->show();
        headerOverlay->raise();
        connect(headerOverlay, &QPushButton::clicked, [launchBtn](){ QMetaObject::invokeMethod(launchBtn, "click", Qt::QueuedConnection); });
    }
    // Auto-launch when user selects the Car tab
    connect(tabs, &QTabWidget::currentChanged, this, [this, carIndex, launchBtn](int idx){
        if (idx == carIndex) {
            if (!(m_demoProc && m_demoProc->state() != QProcess::NotRunning)) {
                // trigger launch
                QMetaObject::invokeMethod(launchBtn, "click", Qt::QueuedConnection);
            }
        }
    });

    // Also trigger on tab header clicks (tabBarClicked) so tapping the same tab will launch
    if (auto bar = tabs->tabBar()) {
        connect(bar, &QTabBar::tabBarClicked, this, [this, carIndex, launchBtn](int idx){
            if (idx == carIndex) {
                if (!(m_demoProc && m_demoProc->state() != QProcess::NotRunning)) {
                    QMetaObject::invokeMethod(launchBtn, "click", Qt::QueuedConnection);
                }
            }
        });
    }

    // make tabbar more visible and switch to Car tab so it's easy to find
    tabs->setStyleSheet("QTabBar::tab{ min-width:140px; min-height:40px; background:#444; color:white; font-weight:bold; } QTabWidget::pane{ border: 2px solid #666; }");
    tabs->setCurrentIndex(tabs->count()-1);

    // Install app-wide event filter to catch touch/mouse events that may not reach the tabBar directly
    qApp->installEventFilter(this);
    // keep reference to header overlay for dynamic layout adjustments
    if (m_tabBar) { m_headerOverlay = m_tabBar->findChild<QPushButton*>("tabHeaderOverlay"); if (m_headerOverlay) {
        // ensure overlay follows tabBar geometry on startup
        m_headerOverlay->setGeometry(m_tabBar->rect());
    }
}

    // Add a persistent top-right Launch 3D button so touch can always trigger the demo reliably
    QWidget* central = new QWidget(this);
    auto centralLayout = new QVBoxLayout(central);
    auto topRow = new QHBoxLayout();
    topRow->addStretch();
    auto topLaunchBtn = new QPushButton("Launch 3D");
    topLaunchBtn->setFixedSize(180,64);
    topLaunchBtn->setStyleSheet("font-size:18px; background:#1565c0; color:white; border-radius:8px;");
    topRow->addWidget(topLaunchBtn);

    // Prominent Cancel button next to Launch 3D for quick stop/restore
    auto topCancelBtn = new QPushButton("Cancel 3D");
    topCancelBtn->setFixedSize(180,64);
    topCancelBtn->setStyleSheet("font-size:18px; background:#b71c1c; color:white; border-radius:8px;");
    topRow->addWidget(topCancelBtn);

    centralLayout->addLayout(topRow);
    centralLayout->addWidget(tabs);
    // clicking the top Launch 3D triggers the existing launch logic
    connect(topLaunchBtn, &QPushButton::clicked, [this]() { if (m_launchBtn) QMetaObject::invokeMethod(m_launchBtn, "click", Qt::QueuedConnection); });
    // clicking the top Cancel triggers the same cancel path as the in-tab Cancel 3D button
    connect(topCancelBtn, &QPushButton::clicked, [this]() {
        if (m_demoProc && m_demoProc->state() != QProcess::NotRunning) {
            m_demoProc->terminate();
            if (!m_demoProc->waitForFinished(1000)) m_demoProc->kill();
            delete m_demoProc; m_demoProc = nullptr;
            this->setWindowState(this->windowState() & ~Qt::WindowMinimized);
        } else {
            QProcess::startDetached("/bin/sh", QStringList() << "-c" << "pkill -f glmark2 || true");
        }
    });

    setCentralWidget(central);
    setWindowTitle("HMI Test Tool");

    // make touch-friendly: larger buttons globally
    qApp->setStyleSheet("QPushButton{ min-width:120px; min-height:80px; font-size:24px; padding:16px; }");

    // force fixed window size and remove decorations to avoid compositor fullscreen requests
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setFixedSize(1024,600);

    // load calibration if present
    loadCalibration();
    // NOTE: global event filter removed to avoid re-entrancy issues on Wayland. DisplayTest will receive the transform.

    // add calibration action in Display tab via dialog
    auto calib = new QPushButton("Calibrate Touch");
    if(auto wdg = tabs->widget(0)){
        if(wdg->layout()) wdg->layout()->addWidget(calib);
    }
    connect(calib,&QPushButton::clicked,[this](){
        Calibrator dlg(this);
        if(dlg.exec()==QDialog::Accepted && dlg.isValid()){
            m_calib = dlg.transform();
        }
    });

    // log tab info for remote diagnostics
    QFile logf("/tmp/hmi-ui.log");
    if(logf.open(QIODevice::WriteOnly | QIODevice::Append)){
        QTextStream ts(&logf);
        ts << "Tab count:" << tabs->count() << "\n";
        for(int i=0;i<tabs->count();++i) ts << "Tab["<<i<<"]="<<tabs->tabText(i)<<"\n";
        logf.close();
    }
}


bool MainWindow::loadCalibration(){
    QString path = QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/.hmi_touch_calib.json";
    QFile f(path);
    if(!f.exists()) return false;
    if(!f.open(QIODevice::ReadOnly)) return false;
    auto doc = QJsonDocument::fromJson(f.readAll()); f.close();
    if(!doc.isObject()) return false;
    auto obj = doc.object();
    if(!obj.contains("transform")) return false;
    auto arr = obj["transform"].toArray();
    if(arr.size()!=9) return false;
    QTransform t(arr[0].toDouble(), arr[1].toDouble(), arr[2].toDouble(), arr[3].toDouble(), arr[4].toDouble(), arr[5].toDouble(), arr[6].toDouble(), arr[7].toDouble(), arr[8].toDouble());
    m_calib = t; return true;
}

#include <QTouchEvent>

bool MainWindow::eventFilter(QObject* obj, QEvent* ev){
    // Event logging helper (write to a user-writable path)
    auto logEvent = [&](const QString &line){
        QFile lf("/mnt/data/home/user/tab_event_log.txt");
        if(lf.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)){
            QTextStream ts(&lf);
            ts << QDateTime::currentDateTime().toString(Qt::ISODate) << " " << line << "\n";
            lf.close();
        }
    };

    // Handle both mouse and touch on the tab bar so tapping the same tab reliably launches the demo
    // Global check: accept events from any object by mapping global positions to the tab bar.
    if (m_tabBar) {
        // Mouse release
        if (ev->type() == QEvent::MouseButtonRelease) {
            if (auto mev = dynamic_cast<QMouseEvent*>(ev)) {
                QPoint global = mev->globalPos();
                QPoint local = m_tabBar->mapFromGlobal(global);
                int idx = m_tabBar->tabAt(local);
                logEvent(QString("MouseRelease global=(%1,%2) tab-local=(%3,%4) idx=%5").arg(global.x()).arg(global.y()).arg(local.x()).arg(local.y()).arg(idx));
                if (idx == m_carIndex) {
                    logEvent("MouseRelease matched carIndex");
                    if (!(m_demoProc && m_demoProc->state() != QProcess::NotRunning) && m_launchBtn) {
                        logEvent("launch triggered by MouseRelease");
                        QMetaObject::invokeMethod(m_launchBtn, "click", Qt::QueuedConnection);
                    }
                    return true;
                }
            }
        }
        // Touch end
        if (ev->type() == QEvent::TouchEnd) {
            if (auto tev = dynamic_cast<QTouchEvent*>(ev)) {
                auto tps = tev->touchPoints();
                if (!tps.isEmpty()) {
                    QPointF screen = tps.first().screenPos();
                    QPoint local = m_tabBar->mapFromGlobal(screen.toPoint());
                    int idx = m_tabBar->tabAt(local);
                    logEvent(QString("TouchEnd screen=(%1,%2) tab-local=(%3,%4) idx=%5").arg(screen.x()).arg(screen.y()).arg(local.x()).arg(local.y()).arg(idx));
                    if (idx == m_carIndex) {
                        logEvent("TouchEnd matched carIndex");
                        if (!(m_demoProc && m_demoProc->state() != QProcess::NotRunning) && m_launchBtn) {
                            logEvent("launch triggered by TouchEnd");
                            QMetaObject::invokeMethod(m_launchBtn, "click", Qt::QueuedConnection);
                        }
                        return true;
                    }
                }
            }
        }
    }

    // Then, translate touch/mouse events using calibration as before
    if(m_handlingEvent) return QMainWindow::eventFilter(obj, ev);
    if(!m_calib.isIdentity() && (ev->type()==QEvent::MouseButtonPress || ev->type()==QEvent::MouseButtonRelease || ev->type()==QEvent::MouseMove || ev->type()==QEvent::TouchUpdate)){
        // try mouse event first
        if(auto mev = dynamic_cast<QMouseEvent*>(ev)){
            m_handlingEvent = true;
            QPointF p = mev->localPos();
            QPointF tp = m_calib.map(p);
            logEvent(QString("MouseEvent type=%1 local=(%2,%3) mapped=(%4,%5)").arg(ev->type()).arg(p.x()).arg(p.y()).arg(tp.x()).arg(tp.y()));
            QMouseEvent newEv(mev->type(), tp.toPoint(), mev->windowPos(), mev->screenPos(), mev->button(), mev->buttons(), mev->modifiers());
            QApplication::sendEvent(obj, &newEv);
            m_handlingEvent = false;
            return true; // consume original
        }
        // fallback: touch update -> synthesize mouse event at touch point
        if (ev->type() == QEvent::TouchUpdate || ev->type() == QEvent::TouchEnd) {
            if (auto tev = dynamic_cast<QTouchEvent*>(ev)) {
                auto tps = tev->touchPoints();
                if (!tps.isEmpty()) {
                    QPointF localp = tps.first().pos();
                    QPointF tp = m_calib.map(localp);
                    logEvent(QString("TouchEvent type=%1 pos=(%2,%3) mapped=(%4,%5)").arg(ev->type()).arg(localp.x()).arg(localp.y()).arg(tp.x()).arg(tp.y()));
                    QMouseEvent newEv(QEvent::MouseMove, tp.toPoint(), QPointF(), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
                    QApplication::sendEvent(obj, &newEv);
                    return true;
                }
            }
        }
    }
    return QMainWindow::eventFilter(obj, ev);
}
