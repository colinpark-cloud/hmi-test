#include "proxtest.h"
#include "cameraview.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QFile>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QProcess>
#include <QStackedWidget>
#include <QTextStream>
#include <QVBoxLayout>
#include <QTimer>
#include <QThread>
#include <QUrl>
#include <QtQuickWidgets/QQuickWidget>

static QString panelStyle() {
    return "QWidget{background:#f7f9fc; border:1px solid #d8e0ea; border-radius:14px;}";
}

ProxTest::ProxTest(QWidget* parent) : QWidget(parent) {
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(12);

    auto *card = new QWidget;
    card->setStyleSheet(panelStyle());
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(18, 18, 18, 18);
    layout->setSpacing(10);

    auto *title = new QLabel("VCNL4200 Proximity");
    title->setStyleSheet("font-size:22px; font-weight:800; color:#17212f;");

    auto *desc = new QLabel("Reads proximity data from VCNL4200 on i2c-6 at 0x51 and swaps demo/camera in-place.");
    desc->setWordWrap(true);
    desc->setStyleSheet("color:#5f6b7a; font-size:14px;");

    auto *contentRow = new QHBoxLayout;
    contentRow->setSpacing(10);

    auto *leftCol = new QVBoxLayout;
    auto *infoColTop = new QVBoxLayout;
    infoColTop->setSpacing(6);
    m_busLabel = new QLabel("Bus: MCP2221 @ /dev/i2c-6");
    m_addrLabel = new QLabel("Addr: 0x51");
    for (auto *l : {m_busLabel, m_addrLabel}) {
        l->setStyleSheet("background:#ffffff; border:1px solid #cdd6e1; border-radius:10px; padding:8px 12px; font-size:14px; font-weight:700;");
        l->setMaximumWidth(220);
        infoColTop->addWidget(l);
    }

    m_status = new QLabel("Sensor Ready");
    m_status->setMaximumWidth(220);
    m_status->setStyleSheet("color:#2563eb; font-weight:700; font-size:14px;");

    auto *valueBox = new QWidget;
    valueBox->setMaximumWidth(220);
    valueBox->setStyleSheet("background:#ffffff; border:1px solid #cdd6e1; border-radius:12px;");
    auto *valueLayout = new QVBoxLayout(valueBox);
    valueLayout->setContentsMargins(12, 10, 12, 10);
    valueLayout->setSpacing(4);
    auto *vTitle = new QLabel("Proximity Data");
    vTitle->setStyleSheet("color:#5f6b7a; font-size:12px; font-weight:700;");
    m_value = new QLabel("-");
    m_value->setStyleSheet("color:#0f1724; font-size:20px; font-weight:800;");
    auto *lightTitle = new QLabel("Ambient Light Data");
    lightTitle->setStyleSheet("color:#5f6b7a; font-size:12px; font-weight:700;");
    m_lightValue = new QLabel("-");
    m_lightValue->setStyleSheet("color:#0f1724; font-size:20px; font-weight:800;");
    auto *brightnessTitle = new QLabel("LCD Brightness");
    brightnessTitle->setStyleSheet("color:#5f6b7a; font-size:12px; font-weight:700;");
    m_brightnessValue = new QLabel("-");
    m_brightnessValue->setStyleSheet("color:#0f1724; font-size:20px; font-weight:800;");
    auto *flagTitle = new QLabel("Interrupt Flags");
    flagTitle->setStyleSheet("color:#5f6b7a; font-size:12px; font-weight:700;");
    m_flagValue = new QLabel("-");
    m_flagValue->setStyleSheet("color:#0f1724; font-size:20px; font-weight:800;");
    valueLayout->addWidget(vTitle);
    valueLayout->addWidget(m_value);
    valueLayout->addWidget(lightTitle);
    valueLayout->addWidget(m_lightValue);
    valueLayout->addWidget(brightnessTitle);
    valueLayout->addWidget(m_brightnessValue);
    valueLayout->addWidget(flagTitle);
    valueLayout->addWidget(m_flagValue);

    m_autoBrightnessBtn = new QPushButton("Auto Brightness ON");
    m_autoBrightnessBtn->setMaximumWidth(220);
    m_autoBrightnessBtn->setMinimumHeight(40);
    m_autoBrightnessBtn->setStyleSheet("font-size:15px; font-weight:700; background:#7a2ea8; color:white; border:1px solid #a15bd1; border-radius:10px; padding:8px 12px;");

    m_reinitBtn = new QPushButton("Re-init Sensor");
    m_reinitBtn->setMaximumWidth(220);
    m_reinitBtn->setMinimumHeight(40);
    m_reinitBtn->setStyleSheet("font-size:15px; font-weight:700; background:#17304c; color:white; border:1px solid #2d5b89; border-radius:10px; padding:8px 12px;");

    m_log = new QPlainTextEdit;
    m_log->setReadOnly(true);
    m_log->hide();
    m_log->setPlaceholderText("Proximity log...");
    m_log->setStyleSheet("background:#ffffff; color:#17212f; font-family:monospace; font-size:11px; border:1px solid #cdd6e1; border-radius:10px;");

    leftCol->addLayout(infoColTop);
    leftCol->addWidget(m_status);
    leftCol->addWidget(valueBox);
    leftCol->addWidget(m_autoBrightnessBtn);
    leftCol->addWidget(m_reinitBtn);
    leftCol->addStretch(1);

    m_previewStack = new QStackedWidget;
    m_previewStack->setStyleSheet("background:#ffffff; border:1px solid #cdd6e1; border-radius:12px;");
    m_previewStack->setMinimumSize(560, 420);

    m_demoView = new QQuickWidget;
    m_demoView->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_demoView->setClearColor(Qt::black);
    m_demoView->setSource(QUrl::fromLocalFile(QCoreApplication::applicationDirPath() + "/qml/FancyDashboard.qml"));

    m_cameraView = new CameraView(this);

    m_previewStack->addWidget(m_demoView);
    m_previewStack->addWidget(m_cameraView);
    m_previewStack->setCurrentWidget(m_demoView);

    contentRow->addLayout(leftCol, 0);
    contentRow->addWidget(m_previewStack, 5);

    layout->addWidget(title);
    layout->addWidget(desc);
    layout->addLayout(contentRow, 1);
    root->addWidget(card, 1);

    connect(m_autoBrightnessBtn, &QPushButton::clicked, this, [this]() {
        m_autoBrightness = !m_autoBrightness;
        if (m_autoBrightnessBtn) {
            m_autoBrightnessBtn->setText(m_autoBrightness ? "Auto Brightness ON" : "Auto Brightness OFF");
        }
        setStatus(m_autoBrightness ? "Auto brightness enabled" : "Auto brightness disabled", false);
    });
    connect(m_reinitBtn, &QPushButton::clicked, this, &ProxTest::reinitializeSensor);

    m_brightnessMax = readBacklightMax();
    m_lastBrightness = m_brightnessMax;
    m_targetBrightness = m_brightnessMax;
    if (m_brightnessValue) m_brightnessValue->setText(QString("%1 / %2").arg(m_lastBrightness).arg(m_brightnessMax));

    auto *brightnessTimer = new QTimer(this);
    brightnessTimer->setInterval(30);
    connect(brightnessTimer, &QTimer::timeout, this, [this]() { tickBrightnessRamp(); });
    brightnessTimer->start();

    startI2CPolling();
    initProxSensor();
    executeI2CCommands();
}

bool ProxTest::runCmd(const QStringList& args, QString* out) {
    QProcess p;
    p.start("/usr/sbin/i2cget", args);
    if (!p.waitForFinished(2000)) return false;
    const QString s = QString::fromLocal8Bit(p.readAllStandardOutput()).trimmed();
    if (out) *out = s;
    return p.exitStatus() == QProcess::NormalExit && p.exitCode() == 0;
}

bool ProxTest::readWord(const QString& bus, const QString& addr, const QString& reg, quint16& value) {
    QString out;
    const QStringList args = {"-y", bus, addr, reg, "w"};
    if (!runCmd(args, &out)) return false;
    bool ok = false;
    value = out.startsWith("0x") ? out.mid(2).toUShort(&ok, 16) : out.toUShort(&ok, 16);
    return ok;
}

bool ProxTest::writeWord(const QString& bus, const QString& addr, const QString& reg, quint16 value) {
    QProcess p;
    const QStringList args = {"-y", bus, addr, reg, QString("0x%1").arg(value, 4, 16, QLatin1Char('0')).toUpper(), "w"};
    p.start("/usr/sbin/i2cset", args);
    if (!p.waitForFinished(2000)) return false;
    return p.exitStatus() == QProcess::NormalExit && p.exitCode() == 0;
}

bool ProxTest::initProxSensor() {
    bool ok = true;

    quint16 dummy = 0;
    readWord(m_bus, m_addr, "0x0e", dummy);
    readWord(m_bus, m_addr, "0x03", dummy);
    readWord(m_bus, m_addr, "0x08", dummy);

    ok &= writeWord(m_bus, m_addr, "0x03", 0x0000);
    readWord(m_bus, m_addr, "0x03", dummy);
    readWord(m_bus, m_addr, "0x08", dummy);

    ok &= writeWord(m_bus, m_addr, "0x03", 0x08ca);
    ok &= writeWord(m_bus, m_addr, "0x04", 0x0760);
    ok &= writeWord(m_bus, m_addr, "0x00", 0x0010);
    ok &= writeWord(m_bus, m_addr, "0x01", 0x0001);

    QThread::msleep(200);

    quint16 reg03 = 0;
    quint16 reg04 = 0;
    readWord(m_bus, m_addr, "0x03", reg03);
    readWord(m_bus, m_addr, "0x04", reg04);

    if (ok) {
        m_initialized = true;
        appendLog(QString("prox init ok reg03=0x%1 reg04=0x%2")
                  .arg(reg03, 4, 16, QLatin1Char('0')).toUpper()
                  .arg(reg04, 4, 16, QLatin1Char('0')).toUpper());
    } else {
        appendLog("prox init failed");
    }
    return ok;
}

int ProxTest::readBacklightMax() const {
    QFile f("/sys/class/backlight/backlight-lvds/max_brightness");
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return 255;
    const int v = QString::fromUtf8(f.readAll()).trimmed().toInt();
    return v > 0 ? v : 255;
}

bool ProxTest::writeBacklight(int value) {
    QFile f("/sys/class/backlight/backlight-lvds/brightness");
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        appendLog("writeBacklight open failed");
        return false;
    }
    QTextStream ts(&f);
    ts << value;
    const bool ok = f.flush();
    if (!ok) appendLog("writeBacklight flush failed");
    return ok;
}

int ProxTest::readBacklightCurrent() const {
    QFile f("/sys/class/backlight/backlight-lvds/brightness");
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return -1;
    return QString::fromUtf8(f.readAll()).trimmed().toInt();
}

int ProxTest::mapAlsToBrightness(quint16 als) const {
    const int minB = 5;
    const int maxB = m_brightnessMax > 0 ? m_brightnessMax : 255;
    const int clampedAls = qBound(0, static_cast<int>(als), 2000);
    return minB + ((maxB - minB) * clampedAls) / 2000;
}

void ProxTest::updateAutoBrightness(quint16 als) {
    if (!m_autoBrightness) return;
    m_smoothedAls = als;
    m_targetBrightness = mapAlsToBrightness(m_smoothedAls);
    m_lastBrightness = m_targetBrightness;
    const bool ok = writeBacklight(m_targetBrightness);
    const int actual = readBacklightCurrent();
    if (m_brightnessValue) {
        m_brightnessValue->setText(QString("%1 / %2").arg(actual >= 0 ? actual : m_targetBrightness).arg(m_brightnessMax));
    }
    appendLog(QString("ALS=0x%1 targetBrightness=%2 actual=%3 write=%4")
              .arg(als, 4, 16, QLatin1Char('0')).toUpper()
              .arg(m_targetBrightness)
              .arg(actual)
              .arg(ok ? "ok" : "fail"));
}

void ProxTest::tickBrightnessRamp() {
    const int actual = readBacklightCurrent();
    if (m_brightnessValue) {
        m_brightnessValue->setText(QString("%1 / %2").arg(actual >= 0 ? actual : m_lastBrightness).arg(m_brightnessMax));
    }
}

void ProxTest::setStatus(const QString& text, bool error) {
    if (m_status) {
        m_status->setText(text);
        m_status->setStyleSheet(error ? "color:#dc2626; font-weight:700; font-size:14px;" : "color:#2563eb; font-weight:700; font-size:14px;");
    }
}

void ProxTest::appendLog(const QString& text) {
    if (!m_log) return;
    m_log->appendPlainText(QDateTime::currentDateTime().toString(Qt::ISODate) + " " + text);
}

void ProxTest::updatePresenceUi(quint16 ps) {
    const bool nextPresent = m_personPresent ? (ps > 80) : (ps >= 120);
    if (nextPresent == m_personPresent) return;
    m_personPresent = nextPresent;
    if (!m_previewStack || !m_cameraView || !m_demoView) return;
    if (m_personPresent) {
        m_previewStack->setCurrentWidget(m_cameraView);
        m_cameraView->startCamera();
        appendLog(QString("Presence detected, switching to camera (PS=0x%1)").arg(ps, 4, 16, QLatin1Char('0')).toUpper());
    } else {
        m_cameraView->stopCamera();
        m_previewStack->setCurrentWidget(m_demoView);
        appendLog(QString("Presence cleared, switching to demo (PS=0x%1)").arg(ps, 4, 16, QLatin1Char('0')).toUpper());
    }
}

void ProxTest::setActive(bool active) {
    if (m_active == active) return;
    m_active = active;
    if (!m_active) {
        if (m_pollTimer) m_pollTimer->stop();
        if (m_cameraView) m_cameraView->stopCamera();
        if (m_previewStack && m_demoView) m_previewStack->setCurrentWidget(m_demoView);
        m_personPresent = false;
        setStatus("Sensor paused", false);
    } else {
        if (m_pollTimer && !m_pollTimer->isActive()) m_pollTimer->start(1000);
        setStatus("Sensor OK", false);
        executeI2CCommands();
    }
}

void ProxTest::reinitializeSensor() {
    m_initialized = false;
    setStatus("Re-initializing sensor...", false);

    bool ok = false;
    quint16 probe = 0;
    for (int attempt = 0; attempt < 5; ++attempt) {
        QThread::msleep(250);
        if (readWord(m_bus, m_addr, "0x0e", probe) || readWord(m_bus, m_addr, "0x08", probe)) {
            ok = initProxSensor();
            if (ok) break;
        }
    }

    if (ok) {
        setStatus("Sensor re-init complete", false);
        executeI2CCommands();
    } else {
        setStatus("Sensor re-init failed", true);
    }
}

void ProxTest::pollProx() {
    executeI2CCommands();
}

void ProxTest::executeI2CCommands() {
    if (!m_active) return;
    if (!m_initialized) {
        initProxSensor();
    }

    quint16 reg03 = 0;
    quint16 reg04 = 0;
    quint16 ps = 0;
    quint16 als = 0;

    const bool okReg03 = readWord(m_bus, m_addr, "0x03", reg03);
    const bool okReg04 = readWord(m_bus, m_addr, "0x04", reg04);
    const bool okPs = readWord(m_bus, m_addr, "0x08", ps);
    const bool okAls = readWord(m_bus, m_addr, "0x09", als);

    if (okReg03) appendLog(QString("i2cget -y 6 0x51 0x03 w => 0x%1").arg(reg03, 4, 16, QLatin1Char('0')).toUpper());
    if (okReg04) appendLog(QString("i2cget -y 6 0x51 0x04 w => 0x%1").arg(reg04, 4, 16, QLatin1Char('0')).toUpper());
    if (okPs) appendLog(QString("i2cget -y 6 0x51 0x08 w => 0x%1").arg(ps, 4, 16, QLatin1Char('0')).toUpper());
    if (okAls) appendLog(QString("i2cget -y 6 0x51 0x09 w => 0x%1").arg(als, 4, 16, QLatin1Char('0')).toUpper());

    if (m_value) m_value->setText(okPs ? QString("0x%1").arg(ps, 4, 16, QLatin1Char('0')).toUpper() : "ERR");
    if (m_lightValue) m_lightValue->setText(okAls ? QString("0x%1").arg(als, 4, 16, QLatin1Char('0')).toUpper() : "ERR");
    if (m_flagValue) m_flagValue->setText(okReg03 ? QString("0x%1").arg(reg03, 4, 16, QLatin1Char('0')).toUpper() : "ERR");
    if (okAls) updateAutoBrightness(als);
    if (okPs) updatePresenceUi(ps);
    setStatus((okPs || okAls) ? "Sensor OK" : "Sensor read failed",
              !(okPs || okAls));
}

void ProxTest::startI2CPolling() {
    m_pollTimer = new QTimer(this);
    connect(m_pollTimer, &QTimer::timeout, this, [this]() {
        quint16 ps = 0;
        quint16 als = 0;
        const bool okPs = readWord(m_bus, m_addr, "0x08", ps);
        const bool okAls = readWord(m_bus, m_addr, "0x09", als) || readWord(m_bus, m_addr, "0x0A", als);
        if (m_value) m_value->setText(okPs ? QString("0x%1").arg(ps, 4, 16, QLatin1Char('0')).toUpper() : "ERR");
        if (m_lightValue) m_lightValue->setText(okAls ? QString("0x%1").arg(als, 4, 16, QLatin1Char('0')).toUpper() : "ERR");
        if (okAls) updateAutoBrightness(als);
        if (okPs) updatePresenceUi(ps);
        if (okPs || okAls) {
            setStatus("Sensor OK", false);
        } else {
            setStatus("Polling failed for I2C", true);
        }
    });
    m_pollTimer->start(1000);
}
