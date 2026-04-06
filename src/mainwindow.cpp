#include "mainwindow.h"
#include <QApplication>
#include <QComboBox>
#include <QDateTime>
#include <QFile>
#include <QFrame>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QSlider>
#include <QStandardPaths>
#include <QTextStream>
#include <QTouchEvent>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>
#include <QtQml/qqmlengine.h>
#include <QtQuickWidgets/QQuickWidget>

#include "calibrator.h"
#include "displaytest.h"
#include "gpiotest.h"
#include "perftest.h"
#include "serialtest.h"
#include "smartoven.h"
#include "storagetest.h"

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    auto logUi = [](const QString& msg) {
        QFile logf("/tmp/hmi-ui.log");
        if (logf.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            QTextStream ts(&logf);
            ts << QDateTime::currentDateTime().toString(Qt::ISODate) << " " << msg << "\n";
        }
    };

    auto *tabs = new QTabWidget(this);
    tabs->addTab(new DisplayTest, "Display & Touch");
    tabs->addTab(new GPIOTest, "GPIO / Buzzer");
    tabs->addTab(new SerialTest, "Serial (RS232/485)");
    tabs->addTab(new StorageTest, "Storage");
    tabs->addTab(new PerfTest, "Performance");
    connect(tabs, &QTabWidget::currentChanged, this, [=](int index) {
        logUi(QString("tab_changed:%1:%2").arg(index).arg(tabs->tabText(index)));
    });

    // Qt-provided style sample tab: simple widgets demo instead of the old 3D page.
    auto *sampleTab = new QWidget;
    auto *sampleLayout = new QVBoxLayout(sampleTab);
    sampleLayout->setContentsMargins(16, 16, 16, 16);
    sampleLayout->setSpacing(10);

    auto *title = new QLabel("Qt Widgets Sample");
    title->setStyleSheet("font-size:20px; font-weight:700;");
    auto *desc = new QLabel("A small Qt Widgets demo page with controls, progress, and a live preview panel.");
    desc->setWordWrap(true);
    desc->setStyleSheet("color:#b0b0b0; font-size:16px;");

    auto *preview = new QFrame;
    preview->setMinimumHeight(180);
    preview->setStyleSheet("QFrame { background:#263238; border:2px solid #455a64; border-radius:14px; }");
    auto *previewLayout = new QVBoxLayout(preview);
    previewLayout->setContentsMargins(16, 16, 16, 16);
    previewLayout->setSpacing(8);

    auto *sampleLabel = new QLabel("Hello from Qt");
    sampleLabel->setAlignment(Qt::AlignCenter);
    sampleLabel->setStyleSheet("color:white; font-size:22px; font-weight:700;");
    auto *sampleValue = new QLabel("Value: 35");
    sampleValue->setAlignment(Qt::AlignCenter);
    sampleValue->setStyleSheet("color:#e0e0e0; font-size:13px;");
    auto *meter = new QProgressBar;
    meter->setRange(0, 100);
    meter->setValue(35);
    meter->setTextVisible(true);
    meter->setFormat("Progress: %p%");
    meter->setStyleSheet("QProgressBar { height: 18px; color: white; border: 1px solid #90a4ae; border-radius: 6px; background: rgba(255,255,255,0.08); } QProgressBar::chunk { background: #00acc1; border-radius: 5px; }");
    previewLayout->addStretch();
    previewLayout->addWidget(sampleLabel);
    previewLayout->addWidget(sampleValue);
    previewLayout->addWidget(meter);
    previewLayout->addStretch();

    auto *controls = new QHBoxLayout;
    controls->setSpacing(8);
    auto *valueLabel = new QLabel("35");
    valueLabel->setMinimumWidth(36);
    valueLabel->setAlignment(Qt::AlignCenter);
    valueLabel->setStyleSheet("font-size:14px; font-weight:700;");
    auto *slider = new QSlider(Qt::Horizontal);
    slider->setRange(0, 100);
    slider->setValue(35);
    auto *modeBox = new QComboBox;
    modeBox->addItems({"Blue", "Purple", "Green", "Orange"});
    modeBox->setFixedHeight(28);
    auto *pulseBtn = new QPushButton("Pulse");
    auto *resetBtn = new QPushButton("Reset");
    controls->addWidget(new QLabel("Value"));
    controls->addWidget(valueLabel);
    controls->addWidget(slider, 1);
    controls->addWidget(modeBox);
    controls->addWidget(pulseBtn);
    controls->addWidget(resetBtn);

    auto *sampleList = new QListWidget;
    sampleList->addItems({"Widgets", "Layouts", "Signals", "Styles", "Animations"});
    sampleList->setMinimumHeight(72);

    sampleLayout->addWidget(title);
    sampleLayout->addWidget(desc);
    sampleLayout->addWidget(preview);
    sampleLayout->addLayout(controls);
    sampleLayout->addWidget(sampleList);

    const QVector<QColor> accents = {
        QColor("#1e88e5"),
        QColor("#8e24aa"),
        QColor("#43a047"),
        QColor("#fb8c00")
    };

    auto applyTheme = [=](int value) {
        const QColor accent = accents.value(modeBox->currentIndex(), accents.front());
        const QColor bg = accent.darker(170);
        const QColor border = accent.lighter(150);
        preview->setStyleSheet(QString("QFrame { background:%1; border:2px solid %2; border-radius:18px; }")
                                  .arg(bg.name(), border.name()));
        sampleLabel->setText(QString("Qt Widgets Sample — %1").arg(value));
        sampleValue->setText(QString("Value: %1").arg(value));
        valueLabel->setText(QString::number(value));
        meter->setValue(value);
        meter->setStyleSheet(QString("QProgressBar { height: 26px; color: white; border: 1px solid %1; border-radius: 8px; background: rgba(255,255,255,0.08); } QProgressBar::chunk { background: %2; border-radius: 6px; }")
                                 .arg(border.name(), accent.name()));
    };

    connect(slider, &QSlider::valueChanged, this, [=](int value) {
        logUi(QString("slider_changed:%1").arg(value));
        applyTheme(value);
    });
    connect(modeBox, qOverload<int>(&QComboBox::currentIndexChanged), this, [=](int index) {
        logUi(QString("mode_changed:%1:%2").arg(index).arg(modeBox->currentText()));
        applyTheme(slider->value());
    });
    connect(resetBtn, &QPushButton::clicked, this, [=]() {
        logUi("reset_clicked");
        modeBox->setCurrentIndex(0);
        slider->setValue(35);
    });
    connect(pulseBtn, &QPushButton::clicked, this, [=]() {
        logUi("pulse_clicked");
        slider->setValue((slider->value() + 25) % 101);
    });
    applyTheme(slider->value());

    tabs->addTab(sampleTab, "Qt Sample");

    QWidget *quickTab = new QWidget;
    auto *quickLayout = new QVBoxLayout(quickTab);
    quickLayout->setContentsMargins(0, 0, 0, 0);
    auto *quickView = new QQuickWidget;
    quickView->setResizeMode(QQuickWidget::SizeRootObjectToView);
    quickView->setClearColor(Qt::black);
    quickView->setMinimumHeight(360);
    quickView->setSource(QUrl::fromLocalFile(QCoreApplication::applicationDirPath() + "/qml/FancyDashboard.qml"));
    quickLayout->addWidget(quickView);
    tabs->addTab(quickTab, "Qt Quick Demo");

    tabs->setStyleSheet("QTabBar::tab{ min-width:120px; min-height:32px; background:#444; color:white; font-weight:bold; } QTabWidget::pane{ border: 2px solid #666; }");

    auto *central = new QWidget(this);
    auto *centralLayout = new QVBoxLayout(central);
    centralLayout->addWidget(tabs);
    setCentralWidget(central);

    // touch-friendly window defaults
    qApp->setStyleSheet(R"(
        QPushButton{
            min-width:72px;
            min-height:34px;
            font-size:14px;
            padding:6px 10px;
        }
        QComboBox, QSlider, QProgressBar, QLabel {
            font-size: 14px;
        }
    )");
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setFixedSize(1024, 600);
    setWindowTitle("HMI Test Tool");

    // load calibration if present
    loadCalibration();

    // Put calibration shortcut in the Display tab too.
    auto *calib = new QPushButton("Calibrate Touch");
    calib->setFixedSize(140, 36);
    calib->setStyleSheet("font-size:13px; background:#1565c0; color:white; border-radius:6px;");
    if (auto *wdg = tabs->widget(0)) {
        if (wdg->layout()) {
            wdg->layout()->addWidget(calib);
        }
    }
    connect(calib, &QPushButton::clicked, this, [=]() {
        logUi("calibrate_clicked");
        Calibrator dlg(this);
        if (dlg.exec() == QDialog::Accepted && dlg.isValid()) {
            m_calib = dlg.transform();
            logUi("calibrate_applied");
        } else {
            logUi("calibrate_cancelled_or_invalid");
        }
    });

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
