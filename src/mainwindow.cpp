#include "mainwindow.h"
#include <QApplication>
#include <QComboBox>
#include <QDateTime>
#include <QFile>
#include <QFrame>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPainter>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QSlider>
#include <QStandardPaths>
#include <QTextStream>
#include <cmath>
#include <QTimer>
#include <QTouchEvent>
#include <QProcess>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>
#include <QtQml/qqmlengine.h>
#include <QtQuickWidgets/QQuickWidget>
#include <QQuaternion>
#include <QVector3D>
#include <Qt3DCore/QEntity>
#include <Qt3DCore/QTransform>
#include <Qt3DExtras/QCuboidMesh>
#include <Qt3DExtras/QCylinderMesh>
#include <Qt3DExtras/QPhongMaterial>
#include <Qt3DExtras/QSphereMesh>
#include <Qt3DExtras/QTorusMesh>
#include <Qt3DExtras/Qt3DWindow>
#include <Qt3DExtras/QOrbitCameraController>
#include <Qt3DRender/QCamera>
#include <Qt3DRender/QDirectionalLight>
#include <Qt3DRender/QPointLight>
#include <Qt3DExtras/QForwardRenderer>

#include "calibrator.h"
#include "displaytest.h"
#include "gpiotest.h"
#include "perftest.h"
#include "serialtest.h"
#include "smartoven.h"
#include "storagetest.h"
#include "commtest.h"

class GpuDemoWidget : public QWidget {
public:
    explicit GpuDemoWidget(QWidget* parent = nullptr) : QWidget(parent) {
        auto *view = new Qt3DExtras::Qt3DWindow();
        view->defaultFrameGraph()->setClearColor(QColor("#050816"));
        container = QWidget::createWindowContainer(view, this);
        container->setFocusPolicy(Qt::StrongFocus);

        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(container);

        rootEntity = new Qt3DCore::QEntity();
        view->setRootEntity(rootEntity);

        auto *camera = view->camera();
        camera->lens()->setPerspectiveProjection(45.0f, 16.0f / 9.0f, 0.1f, 1000.0f);
        camera->setPosition(QVector3D(0, 0, 28.0f));
        camera->setViewCenter(QVector3D(0, 0, 0));

        auto *lightEntity = new Qt3DCore::QEntity(rootEntity);
        auto *light = new Qt3DRender::QPointLight(lightEntity);
        light->setColor("#ffffff");
        light->setIntensity(1.5f);
        auto *lightTransform = new Qt3DCore::QTransform(lightEntity);
        lightTransform->setTranslation(QVector3D(12, 12, 20));
        lightEntity->addComponent(light);
        lightEntity->addComponent(lightTransform);

        auto *dirEntity = new Qt3DCore::QEntity(rootEntity);
        auto *dir = new Qt3DRender::QDirectionalLight(dirEntity);
        dir->setWorldDirection(QVector3D(-1, -1, -1));
        dir->setColor("#8ddcff");
        dir->setIntensity(1.1f);
        dirEntity->addComponent(dir);

        auto makeMesh = [&](Qt3DRender::QGeometryRenderer *mesh, const QColor &color, const QVector3D &pos, const QVector3D &scale, const QVector3D &axis, float speed, const QVector3D &orbit = QVector3D()) {
            auto *entity = new Qt3DCore::QEntity(rootEntity);
            auto *transform = new Qt3DCore::QTransform(entity);
            transform->setTranslation(pos);
            transform->setScale3D(scale);
            auto *material = new Qt3DExtras::QPhongMaterial(entity);
            material->setDiffuse(color);
            material->setAmbient(color.lighter(150));
            entity->addComponent(mesh);
            entity->addComponent(transform);
            entity->addComponent(material);
            meshes.push_back({transform, axis, speed, orbit, pos});
        };

        auto *torus = new Qt3DExtras::QTorusMesh();
        torus->setRadius(7.0f);
        torus->setMinorRadius(1.0f);
        torus->setRings(72);
        torus->setSlices(28);
        makeMesh(torus, QColor("#00d7ff"), QVector3D(0, 0, 0), QVector3D(1.0f, 1.0f, 1.0f), QVector3D(0, 1, 0), 65.0f);

        auto *sphere = new Qt3DExtras::QSphereMesh();
        sphere->setRadius(2.4f);
        sphere->setRings(24);
        sphere->setSlices(40);
        makeMesh(sphere, QColor("#ff4fd8"), QVector3D(-7.5f, 3.0f, 0), QVector3D(1.0f, 1.0f, 1.0f), QVector3D(1, 1, 0), 95.0f, QVector3D(8.0f, 0.0f, 0.0f));

        auto *cube = new Qt3DExtras::QCuboidMesh();
        makeMesh(cube, QColor("#50ff9a"), QVector3D(7.5f, -2.5f, 0), QVector3D(2.0f, 2.0f, 2.0f), QVector3D(0, 0, 1), 130.0f, QVector3D(-8.0f, 0.0f, 0.0f));

        auto *cylinder = new Qt3DExtras::QCylinderMesh();
        cylinder->setRadius(1.4f);
        cylinder->setLength(5.0f);
        cylinder->setRings(24);
        cylinder->setSlices(28);
        makeMesh(cylinder, QColor("#ffb74d"), QVector3D(0, 0, 8.5f), QVector3D(1.0f, 1.0f, 1.0f), QVector3D(0, 0, 1), 155.0f, QVector3D(0.0f, 0.0f, 0.0f));

        auto *orbitController = new Qt3DExtras::QOrbitCameraController(rootEntity);
        orbitController->setCamera(camera);
        orbitController->setLinearSpeed(25.0f);
        orbitController->setLookSpeed(140.0f);

        startTimer(16);
        demoView = view;
    }
protected:
    void timerEvent(QTimerEvent*) override {
        angle += 1.0f;
        for (auto &m : meshes) {
            const QQuaternion base = QQuaternion::fromAxisAndAngle(m.axis, angle * m.speed * 0.016f);
            const QQuaternion orbit = QQuaternion::fromAxisAndAngle(QVector3D(0,1,0), angle * 0.35f);
            m.transform->setRotation(orbit * base);
            if (m.orbit.lengthSquared() > 0.0f) {
                const float rad = angle * 0.02f;
                const QVector3D offset(std::cos(rad) * m.orbit.x(), std::sin(rad * 1.3f) * 0.8f * (std::abs(m.orbit.y()) + 2.0f), std::sin(rad * 0.9f) * m.orbit.x());
                m.transform->setTranslation(m.basePos + offset);
            }
        }
    }
private:
    struct MeshState {
        Qt3DCore::QTransform *transform;
        QVector3D axis;
        float speed;
        QVector3D orbit;
        QVector3D basePos;
    };
    QVector<MeshState> meshes;
    Qt3DCore::QEntity *rootEntity = nullptr;
    Qt3DExtras::Qt3DWindow *demoView = nullptr;
    QWidget *container = nullptr;
    float angle = 0.0f;
};

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    auto logUi = [](const QString& msg) {
        QFile logf("/tmp/hmi-ui.log");
        if (logf.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            QTextStream ts(&logf);
            ts << QDateTime::currentDateTime().toString(Qt::ISODate) << " " << msg << "\n";
        }
    };

    auto *tabs = new QTabWidget(this);
    tabs->setDocumentMode(true);
    tabs->setUsesScrollButtons(false);
    tabs->tabBar()->setExpanding(false);

    QWidget *displayTab = new QWidget;
    auto *displayLayout = new QVBoxLayout(displayTab);
    displayLayout->setContentsMargins(0, 0, 0, 0);
    auto *displayTest = new DisplayTest;
    displayLayout->addWidget(displayTest);
    connect(displayTest, &DisplayTest::started, this, [tabs, this]() {
        m_displayWasFullScreen = isFullScreen();
        tabs->tabBar()->hide();
        showFullScreen();
    });
    connect(displayTest, &DisplayTest::finished, this, [tabs, this]() {
        tabs->tabBar()->show();
        if (!m_displayWasFullScreen) {
            showNormal();
        }
        tabs->setCurrentIndex(0);
    });
    tabs->addTab(displayTab, "Display");

    QWidget *touchTab = new QWidget;
    auto *touchLayout = new QVBoxLayout(touchTab);
    touchLayout->setContentsMargins(24, 24, 24, 24);
    touchLayout->setSpacing(16);
    auto *touchTitle = new QLabel("Touch Test");
    touchTitle->setAlignment(Qt::AlignCenter);
    touchTitle->setStyleSheet("font-size:22px; font-weight:700; color:#17212f;");
    auto *touchDesc = new QLabel("Calibrate, run single-touch, or check multi-touch response.");
    touchDesc->setAlignment(Qt::AlignCenter);
    touchDesc->setStyleSheet("color:#5f6b7a; font-size:13px;");

    auto *tsCalBtn = new QPushButton("Calibrate");
    auto *tsTestBtn = new QPushButton("Single-touch Test");
    auto *tsTestMtBtn = new QPushButton("Multi-touch Test");
    for (auto *b : {tsCalBtn, tsTestBtn, tsTestMtBtn}) {
        b->setMinimumHeight(48);
        b->setStyleSheet("font-size:16px; font-weight:700; background:#17304c; color:white; border:1px solid #2d5b89; border-radius:12px; padding:10px 14px;");
    }
    tsCalBtn->setStyleSheet("font-size:16px; font-weight:700; background:#d97706; color:white; border:1px solid #f59e0b; border-radius:12px; padding:10px 14px;");
    tsTestMtBtn->setStyleSheet("font-size:16px; font-weight:700; background:#7a2ea8; color:white; border:1px solid #a15bd1; border-radius:12px; padding:10px 14px;");
    auto *touchRow = new QWidget;
    auto *touchRowLayout = new QHBoxLayout(touchRow);
    touchRowLayout->setContentsMargins(0, 0, 0, 0);
    touchRowLayout->setSpacing(12);
    touchRowLayout->addStretch(1);
    touchRowLayout->addWidget(tsCalBtn);
    touchRowLayout->addWidget(tsTestBtn);
    touchRowLayout->addWidget(tsTestMtBtn);
    touchRowLayout->addStretch(1);
    touchLayout->addStretch(1);
    touchLayout->addWidget(touchTitle);
    touchLayout->addWidget(touchDesc);
    touchLayout->addSpacing(10);
    touchLayout->addWidget(touchRow);
    touchLayout->addStretch(2);
    tabs->addTab(touchTab, "Touch");

    tabs->addTab(new GPIOTest, "Buzzer");
    tabs->addTab(new SerialTest, "Serial");
    auto *stressTab = new PerfTest;
    tabs->addTab(stressTab, "Stress");
    tabs->addTab(new CommTest, "Comm");
    tabs->addTab(new StorageTest, "Storage");
    connect(tabs, &QTabWidget::currentChanged, this, [=](int index) {
        logUi(QString("tab_changed:%1:%2").arg(index).arg(tabs->tabText(index)));
        if (tabs->tabText(index) != "Stress" && stressTab) {
            QMetaObject::invokeMethod(stressTab, [stressTab]() { stressTab->stopAllLoads(); }, Qt::QueuedConnection);
        }
    });

    QWidget *quickTab = new QWidget;
    auto *quickLayout = new QVBoxLayout(quickTab);
    quickLayout->setContentsMargins(0, 0, 0, 0);
    auto *quickView = new QQuickWidget;
    quickView->setResizeMode(QQuickWidget::SizeRootObjectToView);
    quickView->setClearColor(Qt::black);
    quickView->setMinimumHeight(360);
    quickView->setSource(QUrl::fromLocalFile(QCoreApplication::applicationDirPath() + "/qml/FancyDashboard.qml"));
    quickLayout->addWidget(quickView);
    tabs->addTab(quickTab, "Qt Demo");

    auto runTool = [=](const QString& tool) {
        QString path = QStandardPaths::findExecutable(tool);
        if (path.isEmpty()) {
            path = QString("/usr/bin/%1").arg(tool);
        }
        if (!QFile::exists(path)) {
            QMessageBox::warning(this, tool, QString("%1 not found on system.").arg(path));
            return;
        }
        QStringList args;
        args << "-s" << "-w" << "--" << "/bin/sh" << "-lc"
             << QString("export TSLIB_TSDEVICE=/dev/input/event1; export TSLIB_FBDEVICE=/dev/fb0; exec %1").arg(path);
        bool ok = QProcess::startDetached("/usr/bin/openvt", args);
        if (!ok) {
            QMessageBox::warning(this, tool, QString("Failed to launch fullscreen VT for %1").arg(path));
        }
    };
    connect(tsCalBtn, &QPushButton::clicked, this, [=]() { runTool("ts_calibrate"); });
    connect(tsTestBtn, &QPushButton::clicked, this, [=]() { runTool("ts_test"); });
    connect(tsTestMtBtn, &QPushButton::clicked, this, [=]() { runTool("ts_test_mt"); });

    QWidget *gpuTab = new QWidget;
    auto *gpuLayout = new QVBoxLayout(gpuTab);
    gpuLayout->setContentsMargins(0, 0, 0, 0);
    gpuLayout->setSpacing(0);
    auto *gpuView = new GpuDemoWidget;
    gpuView->setMinimumHeight(520);
    gpuLayout->addWidget(gpuView, 1);
    const int gpuTabIndex = tabs->addTab(gpuTab, "3D");
    QTimer::singleShot(0, this, [tabs, gpuTabIndex]() {
        const int prev = tabs->currentIndex();
        tabs->setCurrentIndex(gpuTabIndex);
        QTimer::singleShot(120, tabs, [tabs, prev]() { tabs->setCurrentIndex(prev); });
    });

    QWidget *glmarkTab = new QWidget;
    auto *glmarkLayout = new QVBoxLayout(glmarkTab);
    glmarkLayout->setContentsMargins(12, 12, 12, 12);
    glmarkLayout->setSpacing(8);
    auto *glmarkTitle = new QLabel("glmark2");
    glmarkTitle->setStyleSheet("font-size:20px; font-weight:700; color:#17212f;");
    auto *glmarkDesc = new QLabel("Quick / full presets");
    glmarkDesc->setWordWrap(true);
    glmarkDesc->setStyleSheet("color:#5f6b7a; font-size:14px;");
    auto *glmarkQuickBtn = new QPushButton("Quick Run");
    glmarkQuickBtn->setFixedHeight(48);
    glmarkQuickBtn->setStyleSheet("font-size:16px; font-weight:700; background:#17304c; color:white; border:1px solid #2d5b89; border-radius:12px;");
    auto *glmarkFullBtn = new QPushButton("Full Run");
    glmarkFullBtn->setFixedHeight(48);
    glmarkFullBtn->setStyleSheet("font-size:16px; font-weight:700; background:#d97706; color:white; border:1px solid #f59e0b; border-radius:12px;");
    auto *glmarkLog = new QPlainTextEdit;
    glmarkLog->setReadOnly(true);
    glmarkLog->setPlaceholderText("Launch logs will appear here...");
    glmarkLog->setStyleSheet("background:#ffffff; color:#1f2937; font-family:monospace; font-size:12px; border:1px solid #cdd6e1; border-radius:10px;");
    glmarkLayout->addWidget(glmarkTitle);
    glmarkLayout->addWidget(glmarkDesc);
    glmarkLayout->addWidget(glmarkQuickBtn);
    glmarkLayout->addWidget(glmarkFullBtn);
    glmarkLayout->addWidget(glmarkLog, 1);
    tabs->addTab(glmarkTab, "glmark2");

    auto appendGlmarkLog = [glmarkLog](const QString& line) {
        glmarkLog->appendPlainText(QDateTime::currentDateTime().toString(Qt::ISODate) + " " + line);
    };
    auto startGlmark = [=](bool quick) {
        appendGlmarkLog(quick ? "launch requested: quick" : "launch requested: full");
        if (m_demoProc && m_demoProc->state() != QProcess::NotRunning) {
            appendGlmarkLog("already running");
            return;
        }
        QString exe;
        const QStringList candidates = {
            "/root/glmark2",
            "/usr/bin/glmark2-es2-wayland",
            "/usr/bin/glmark2-es2",
            "/usr/bin/glmark2",
            "/usr/local/bin/glmark2",
            "/mnt/data/home/user/glmark2/glmark2"
        };
        for (const auto& c : candidates) {
            if (QFile::exists(c) && (QFile::permissions(c) & QFileDevice::ExeUser)) { exe = c; break; }
        }
        if (exe.isEmpty()) {
            appendGlmarkLog("glmark2 not found");
            QMessageBox::warning(this, "glmark2", "glmark2 not found on system.");
            return;
        }
        if (!m_demoProc) m_demoProc = new QProcess(this);
        m_demoProc->setProcessChannelMode(QProcess::MergedChannels);
        connect(m_demoProc, &QProcess::readyReadStandardOutput, this, [=]() {
            appendGlmarkLog(QString::fromLocal8Bit(m_demoProc->readAllStandardOutput()));
        });
        connect(m_demoProc, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), this, [=](int code, QProcess::ExitStatus st) {
            appendGlmarkLog(QString("glmark2 finished code=%1 status=%2").arg(code).arg(st == QProcess::NormalExit ? "normal" : "crash"));
        });
        QStringList args;
        if (quick) {
            args << "--annotate" << "-b" << "build:duration=1.0" << "-b" << "texture:duration=1.0" << "-b" << "shading:duration=1.0";
            appendGlmarkLog(QString("starting %1 quick bench").arg(exe));
        } else {
            args << "--annotate" << "--size" << "800x600";
            appendGlmarkLog(QString("starting %1 full bench").arg(exe));
        }
        m_demoProc->start(exe, args);
        if (!m_demoProc->waitForStarted(2500)) {
            appendGlmarkLog("failed to start");
            QMessageBox::warning(this, "glmark2", QString("Failed to start %1").arg(exe));
            delete m_demoProc;
            m_demoProc = nullptr;
            return;
        }
        appendGlmarkLog("started");
    };
    connect(glmarkQuickBtn, &QPushButton::clicked, this, [=]() { startGlmark(true); });
    connect(glmarkFullBtn, &QPushButton::clicked, this, [=]() { startGlmark(false); });

    tabs->setStyleSheet("QTabBar::tab{ min-width:68px; min-height:30px; background:#e8edf3; color:#344253; font-size:12px; font-weight:600; padding:4px 8px; border-top-left-radius:0px; border-top-right-radius:0px; margin-right:1px; } QTabBar::tab:selected{ background:#ffffff; color:#0f1724; } QTabWidget::pane{ border: 0; background:#f7f9fc; }");

    auto *central = new QWidget(this);
    auto *centralLayout = new QVBoxLayout(central);
    centralLayout->setContentsMargins(0, 0, 0, 0);
    centralLayout->setSpacing(0);
    centralLayout->addWidget(tabs);
    setCentralWidget(central);

    // touch-friendly window defaults
    qApp->setStyleSheet(R"(
        QWidget {
            background: #f7f9fc;
            color: #17212f;
        }
        QPushButton{
            min-width:72px;
            min-height:34px;
            font-size:14px;
            padding:6px 10px;
            background:#17304c;
            color:white;
            border:1px solid #2d5b89;
            border-radius:10px;
        }
        QPushButton:hover{ background:#1e3d60; }
        QPushButton:pressed{ background:#12263d; }
        QComboBox, QSlider, QProgressBar, QLabel {
            font-size: 14px;
        }
        QPlainTextEdit {
            background:#ffffff;
            color:#17212f;
        }
    )");
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setMinimumSize(1024, 600);
    resize(1024, 600);
    setWindowTitle("HMI Test Tool");

    // load calibration if present
    loadCalibration();

    // log tab info for remote diagnostics
    logUi(QString("tab_count:%1").arg(tabs->count()));
    for (int i = 0; i < tabs->count(); ++i) {
        logUi(QString("tab[%1]=%2").arg(i).arg(tabs->tabText(i)));
    }
}

bool MainWindow::loadCalibration() {
    QString path = QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/.hmi_touch_calib.json";
    QFile f(path);
    if (!f.exists()) return false;
    if (!f.open(QIODevice::ReadOnly)) return false;

    const auto doc = QJsonDocument::fromJson(f.readAll());
    f.close();
    if (!doc.isObject()) return false;

    const auto obj = doc.object();
    if (!obj.contains("transform")) return false;
    const auto arr = obj.value("transform").toArray();
    if (arr.size() != 9) return false;

    QTransform t(arr[0].toDouble(), arr[1].toDouble(), arr[2].toDouble(),
                 arr[3].toDouble(), arr[4].toDouble(), arr[5].toDouble(),
                 arr[6].toDouble(), arr[7].toDouble(), arr[8].toDouble());
    m_calib = t;
    return true;
}

bool MainWindow::eventFilter(QObject* obj, QEvent* ev) {
    Q_UNUSED(obj)
    Q_UNUSED(ev)
    // Touch calibration is intentionally not globally remapped here.
    // The app keeps this hook for future diagnostics, but it no longer
    // injects synthetic events that can re-enter Qt's event pipeline.
    return false;
}
