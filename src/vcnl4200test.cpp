#include "vcnl4200test.h"

#include <QDateTime>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QProcess>
#include <QSlider>
#include <QTextStream>
#include <QThread>
#include <QTimer>
#include <QVBoxLayout>
#include <cmath>

static QString panelStyle() {
    return "QWidget{background:#f7f9fc; border:1px solid #d8e0ea; border-radius:14px;}";
}

Vcnl4200Test::Vcnl4200Test(QWidget* parent) : QWidget(parent) {
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(12);

    auto *card = new QWidget;
    card->setStyleSheet(panelStyle());
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(18, 18, 18, 18);
    layout->setSpacing(10);

    m_title = new QLabel("VCNL4200 + Auto Brightness");
    m_title->setStyleSheet("font-size:22px; font-weight:800; color:#17212f;");

    auto *desc = new QLabel("Reads VCNL4200 over CP2112 SMBus bridge on i2c-6 at 0x51, then optionally drives backlight from ALS values.");
    desc->setWordWrap(true);
    desc->setStyleSheet("color:#5f6b7a; font-size:14px;");

    auto *infoRow = new QHBoxLayout;
    m_busLabel = new QLabel("Bus: FT232H @ ftdi://ftdi:232h:1:6/1");
    m_addrLabel = new QLabel("Addr: 0x51");
    for (auto *l : {m_busLabel, m_addrLabel}) {
        l->setStyleSheet("background:#ffffff; border:1px solid #cdd6e1; border-radius:10px; padding:8px 12px; font-size:14px; font-weight:700;");
        infoRow->addWidget(l);
    }
    infoRow->addStretch(1);

    m_status = new QLabel("Ready.");
    m_status->setStyleSheet("color:#2563eb; font-weight:700; font-size:14px;");

    auto makeValue = [](const QString& title) {
        auto *box = new QWidget;
        auto *boxLayout = new QVBoxLayout(box);
        boxLayout->setContentsMargins(12, 10, 12, 10);
        boxLayout->setSpacing(4);
        box->setStyleSheet("background:#ffffff; border:1px solid #cdd6e1; border-radius:12px;");
        auto *t = new QLabel(title);
        t->setStyleSheet("color:#5f6b7a; font-size:12px; font-weight:700;");
        auto *v = new QLabel("-");
        v->setStyleSheet("color:#0f1724; font-size:20px; font-weight:800;");
        boxLayout->addWidget(t);
        boxLayout->addWidget(v);
        return qMakePair(box, v);
    };

    auto idPair = makeValue("Device ID");
    auto alsPair = makeValue("ALS Data");
    auto psPair = makeValue("Proximity");
    m_idValue = idPair.second;
    m_alsValue = alsPair.second;
    m_psValue = psPair.second;

    auto *grid = new QHBoxLayout;
    grid->setSpacing(10);
    grid->addWidget(idPair.first);
    grid->addWidget(alsPair.first);
    grid->addWidget(psPair.first);

    auto *brightnessCard = new QWidget;
    brightnessCard->setStyleSheet("background:#ffffff; border:1px solid #cdd6e1; border-radius:12px;");
    auto *brightnessLayout = new QVBoxLayout(brightnessCard);
    brightnessLayout->setContentsMargins(12, 10, 12, 10);
    brightnessLayout->setSpacing(6);
    auto *brightnessTitle = new QLabel("Display Brightness");
    brightnessTitle->setStyleSheet("color:#5f6b7a; font-size:12px; font-weight:700;");
    m_brightnessValue = new QLabel("-");
    m_brightnessValue->setStyleSheet("color:#0f1724; font-size:20px; font-weight:800;");
    m_brightnessSlider = new QSlider(Qt::Horizontal);
    m_brightnessSlider->setRange(0, readBacklightMax());
    m_brightnessSlider->setValue(readBacklightMax());
    m_brightnessSlider->setStyleSheet("QSlider::groove:horizontal{height:10px;background:#d1d5db;border-radius:5px;} QSlider::sub-page:horizontal{background:#f59e0b;border-radius:5px;} QSlider::add-page:horizontal{background:#e5e7eb;border-radius:5px;} QSlider::handle:horizontal{background:#111827;border:1px solid #111827;width:28px;height:28px;margin:-10px 0;border-radius:14px;}");
    m_autoBtn = new QPushButton("Auto Brightness OFF");
    m_autoBtn->setMinimumHeight(40);
    m_autoBtn->setStyleSheet("font-size:15px; font-weight:700; background:#7a2ea8; color:white; border:1px solid #a15bd1; border-radius:10px; padding:8px 12px;");

    brightnessLayout->addWidget(brightnessTitle);
    brightnessLayout->addWidget(m_brightnessValue);
    brightnessLayout->addWidget(m_brightnessSlider);
    brightnessLayout->addWidget(m_autoBtn);

    m_refreshBtn = new QPushButton("Refresh Now");
    m_refreshBtn->setStyleSheet("font-size:15px; font-weight:700; background:#17304c; color:white; border:1px solid #2d5b89; border-radius:10px; padding:8px 12px;");
    m_refreshBtn->setMinimumHeight(40);

    m_log = new QPlainTextEdit;
    m_log->setReadOnly(true);
    m_log->setPlaceholderText("Sensor log...");
    m_log->setStyleSheet("background:#ffffff; color:#17212f; font-family:monospace; font-size:12px; border:1px solid #cdd6e1; border-radius:10px;");

    layout->addWidget(m_title);
    layout->addWidget(desc);
    layout->addLayout(infoRow);
    layout->addWidget(m_status);
    layout->addLayout(grid);
    layout->addWidget(brightnessCard);
    layout->addWidget(m_refreshBtn);
    layout->addWidget(m_log, 1);
    root->addWidget(card, 1);

    connect(m_refreshBtn, &QPushButton::clicked, this, &Vcnl4200Test::refreshNow);
    connect(m_autoBtn, &QPushButton::clicked, this, &Vcnl4200Test::toggleAutoBrightness);
    connect(m_brightnessSlider, &QSlider::valueChanged, this, &Vcnl4200Test::onBrightnessChanged);

    m_timer = new QTimer(this);
    m_timer->setInterval(300);
    connect(m_timer, &QTimer::timeout, this, &Vcnl4200Test::pollSensor);
    m_timer->start();

    m_retryTimer = new QTimer(this);
    m_retryTimer->setSingleShot(true);
    m_retryTimer->setInterval(4000);
    connect(m_retryTimer, &QTimer::timeout, this, [this]() {
        m_retryPending = false;
        initSensor();
        pollSensor();
    });

    initSensor();
    refreshNow();
}

bool Vcnl4200Test::runCmd(const QStringList& args, QString* out) {
    QProcess p;
    p.start("/usr/sbin/i2cget", args);
    if (!p.waitForFinished(2000)) return false;
    const QString s = QString::fromLocal8Bit(p.readAllStandardOutput()).trimmed();
    if (out) *out = s;
    return p.exitStatus() == QProcess::NormalExit && p.exitCode() == 0;
}

bool Vcnl4200Test::readWord(const QString& bus, const QString& addr, const QString& reg, quint16& value) {
    QString out;
    const QStringList args = {"-y", bus, addr, reg, "w"};
    if (!runCmd(args, &out)) return false;
    bool ok = false;
    value = out.startsWith("0x") ? out.mid(2).toUShort(&ok, 16) : out.toUShort(&ok, 16);
    return ok;
}

bool Vcnl4200Test::writeWord(const QString& bus, const QString& addr, const QString& reg, quint16 value) {
    QProcess p;
    const QStringList args = {"-y", bus, addr, reg, QString("0x%1").arg(value, 4, 16, QLatin1Char('0')).toUpper(), "w"};
    p.start("/usr/sbin/i2cset", args);
    if (!p.waitForFinished(2000)) return false;
    return p.exitStatus() == QProcess::NormalExit && p.exitCode() == 0;
}

static bool isUsbDisconnectLine(const QString& line) {
    return line.contains("USB disconnect", Qt::CaseInsensitive) ||
           line.contains("Transfer timed out", Qt::CaseInsensitive);
}

void Vcnl4200Test::setStatus(const QString& text, bool error) {
    if (m_status) {
        m_status->setText(text);
        m_status->setStyleSheet(error ? "color:#dc2626; font-weight:700; font-size:14px;" : "color:#2563eb; font-weight:700; font-size:14px;");
    }
}

void Vcnl4200Test::appendLog(const QString& text) {
    if (!m_log) return;
    m_log->appendPlainText(QDateTime::currentDateTime().toString(Qt::ISODate) + " " + text);
}

int Vcnl4200Test::readBacklightMax() const {
    QFile f("/sys/class/backlight/backlight-lvds/max_brightness");
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return 255;
    const int v = QString::fromUtf8(f.readAll()).trimmed().toInt();
    return v > 0 ? v : 255;
}

bool Vcnl4200Test::writeBacklight(int value) {
    QFile f("/sys/class/backlight/backlight-lvds/brightness");
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    QTextStream ts(&f);
    ts << value;
    return true;
}


void Vcnl4200Test::refreshNow() {
    if (m_timer) m_timer->stop();
    if (m_retryTimer) m_retryTimer->stop();
    m_retryPending = false;
    m_devicePresent = false;
    initSensor();
    if (m_timer) m_timer->start();
}

bool Vcnl4200Test::initSensor() {
    // Best-effort re-init after USB reconnect.
    // Leave ALS off for this test so we can isolate proximity / bridge behavior.
    const bool okAls = writeWord(m_bus, m_addr, "0x00", 0x0001); // ALS shutdown
    quint16 id = 0;
    const bool okId = readWord(m_bus, m_addr, "0x0e", id);
    if (okId) {
        m_lastId = id;
        m_haveGood = true;
        appendLog(QString("init id=0x%1 als=%2")
                  .arg(id, 4, 16, QLatin1Char('0')).toUpper()
                  .arg(okAls ? "1" : "0"));
        return true;
    }
    appendLog(QString("init failed als=%1").arg(okAls ? "1" : "0"));
    return false;
}

void Vcnl4200Test::pollSensor() {
    if (!m_devicePresent) {
        quint16 probeId = 0;
        if (readWord(m_bus, m_addr, "0x0e", probeId)) {
            m_devicePresent = true;
            m_lastId = probeId;
            m_haveGood = true;
            appendLog(QString("reconnected id=0x%1").arg(probeId, 4, 16, QLatin1Char('0')).toUpper());
            if (m_timer && !m_timer->isActive()) m_timer->start();
        } else {
            setStatus("Waiting for reconnect", true);
            if (m_timer && m_timer->isActive()) m_timer->stop();
            if (!m_retryPending && m_retryTimer) {
                m_retryPending = true;
                m_retryTimer->start();
            }
            return;
        }
    }

    quint16 ps = 0;
    const bool okPs = readWord(m_bus, m_addr, "0x08", ps);
    if (okPs) {
        m_lastPs = ps;
        m_haveGood = true;
        updateDisplayFromGoodValues();
        setStatus(QString("Prox=%1").arg(ps), false);
        appendLog(QString("prox=%1").arg(ps));
    } else {
        setStatus("Prox read failed", true);
        appendLog("prox read failed");
    }
}

void Vcnl4200Test::toggleAutoBrightness() {
    m_autoBrightness = !m_autoBrightness;
    if (m_autoBtn) {
        m_autoBtn->setText(m_autoBrightness ? "Auto Brightness ON" : "Auto Brightness OFF");
    }
    setStatus(m_autoBrightness ? "Auto brightness enabled" : "Auto brightness disabled", false);
}

void Vcnl4200Test::onBrightnessChanged(int value) {
    m_brightnessMax = readBacklightMax();
    if (value < 0) value = 0;
    if (value > m_brightnessMax) value = m_brightnessMax;
    m_lastBrightness = value;
    if (m_brightnessValue) {
        m_brightnessValue->setText(QString("%1 / %2").arg(value).arg(m_brightnessMax));
    }
    writeBacklight(value);
}

void Vcnl4200Test::applyAutoBrightness(quint16 alsValue) {
    Q_UNUSED(alsValue);
    appendLog("auto brightness disabled in ALS-off test mode");
}

void Vcnl4200Test::updateDisplayFromGoodValues() {
    if (m_idValue) m_idValue->setText(QString("0x%1").arg(m_lastId, 4, 16, QLatin1Char('0')).toUpper());
    if (m_alsValue) m_alsValue->setText(QString::number(m_lastAls));
    if (m_psValue) m_psValue->setText(QString::number(m_lastPs));
    if (m_whiteValue) m_whiteValue->setText(QString::number(m_lastWhite));
    if (m_flagValue) m_flagValue->setText(QString("0x%1").arg(m_lastFlags, 2, 16, QLatin1Char('0')).toUpper());
}
