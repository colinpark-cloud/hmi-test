#include "proxtest.h"

#include <QDateTime>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QProcess>
#include <QTextStream>
#include <QVBoxLayout>
#include <QTimer>
#include <QThread>

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

    auto *desc = new QLabel("Reads proximity data only from VCNL4200 on i2c-6 at 0x51.");
    desc->setWordWrap(true);
    desc->setStyleSheet("color:#5f6b7a; font-size:14px;");

    auto *infoRow = new QHBoxLayout;
    m_busLabel = new QLabel("Bus: MCP2221 @ /dev/i2c-6");
    m_addrLabel = new QLabel("Addr: 0x51");
    for (auto *l : {m_busLabel, m_addrLabel}) {
        l->setStyleSheet("background:#ffffff; border:1px solid #cdd6e1; border-radius:10px; padding:8px 12px; font-size:14px; font-weight:700;");
        infoRow->addWidget(l);
    }
    infoRow->addStretch(1);

    m_status = new QLabel("Ready.");
    m_status->setStyleSheet("color:#2563eb; font-weight:700; font-size:14px;");

    auto *valueBox = new QWidget;
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
    auto *flagTitle = new QLabel("Interrupt Flags");
    flagTitle->setStyleSheet("color:#5f6b7a; font-size:12px; font-weight:700;");
    m_flagValue = new QLabel("-");
    m_flagValue->setStyleSheet("color:#0f1724; font-size:20px; font-weight:800;");
    valueLayout->addWidget(vTitle);
    valueLayout->addWidget(m_value);
    valueLayout->addWidget(lightTitle);
    valueLayout->addWidget(m_lightValue);
    valueLayout->addWidget(flagTitle);
    valueLayout->addWidget(m_flagValue);

    m_log = new QPlainTextEdit;
    m_log->setReadOnly(true);
    m_log->setPlaceholderText("Proximity log...");
    m_log->setStyleSheet("background:#ffffff; color:#17212f; font-family:monospace; font-size:12px; border:1px solid #cdd6e1; border-radius:10px;");

    layout->addWidget(title);
    layout->addWidget(desc);
    layout->addLayout(infoRow);
    layout->addWidget(m_status);
    layout->addWidget(valueBox);
    layout->addWidget(m_log, 1);
    root->addWidget(card, 1);

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

void ProxTest::pollProx() {
    executeI2CCommands();
}

void ProxTest::executeI2CCommands() {
    initProxSensor();

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
    setStatus((okPs || okAls) ? QString("PS=0x%1 ALS=0x%2 REG03=0x%3 REG04=0x%4")
                      .arg(ps, 4, 16, QLatin1Char('0')).toUpper()
                      .arg(als, 4, 16, QLatin1Char('0')).toUpper()
                      .arg(reg03, 4, 16, QLatin1Char('0')).toUpper()
                      .arg(reg04, 4, 16, QLatin1Char('0')).toUpper()
                   : "Sensor read failed",
              !(okPs || okAls));
}

void ProxTest::startI2CPolling() {
    auto timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this]() {
        quint16 ps = 0;
        quint16 als = 0;
        const bool okPs = readWord(m_bus, m_addr, "0x08", ps);
        const bool okAls = readWord(m_bus, m_addr, "0x09", als) || readWord(m_bus, m_addr, "0x0A", als);
        if (m_value) m_value->setText(okPs ? QString("0x%1").arg(ps, 4, 16, QLatin1Char('0')).toUpper() : "ERR");
        if (m_lightValue) m_lightValue->setText(okAls ? QString("0x%1").arg(als, 4, 16, QLatin1Char('0')).toUpper() : "ERR");
        if (okPs || okAls) {
            setStatus(QString("watch: PS=0x%1 ALS=0x%2")
                      .arg(ps, 4, 16, QLatin1Char('0')).toUpper()
                      .arg(als, 4, 16, QLatin1Char('0')).toUpper(), false);
        } else {
            setStatus("Polling failed for I2C", true);
        }
    });
    timer->start(1000);
}
