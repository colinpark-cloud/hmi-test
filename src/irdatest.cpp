#include "irdatest.h"

#include <QDateTime>
#include <QFile>
#include <QHBoxLayout>
#include <QIODevice>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QSocketNotifier>
#include <QVBoxLayout>

IrdaTest::IrdaTest(QWidget* parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(14);

    auto *title = new QLabel("IrDA Test");
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size:22px; font-weight:700; color:#17212f;");

    auto *desc = new QLabel("Send and receive MCP2120 / TFDU4101 data through /dev/ttyACM0.");
    desc->setAlignment(Qt::AlignCenter);
    desc->setWordWrap(true);
    desc->setStyleSheet("color:#5f6b7a; font-size:13px;");

    m_status = new QLabel("UART: /dev/ttyACM0 (stopped)");
    m_status->setAlignment(Qt::AlignCenter);
    m_status->setStyleSheet("color:#0f1724; font-size:13px; font-weight:700;");

    m_sendInput = new QLineEdit;
    m_sendInput->setPlaceholderText("Send text to /dev/ttyACM0");
    m_sendInput->setMinimumHeight(42);
    m_sendInput->setStyleSheet("font-size:15px; padding:6px 10px; border:1px solid #cdd6e1; border-radius:10px;");

    m_sendBtn = new QPushButton("Send Test");
    m_startBtn = new QPushButton("Start RX");
    m_stopBtn = new QPushButton("Stop RX");
    m_clearBtn = new QPushButton("Clear Log");

    for (auto *b : {m_sendBtn, m_startBtn, m_stopBtn, m_clearBtn}) {
        b->setMinimumHeight(44);
        b->setStyleSheet("font-size:16px; font-weight:700; border-radius:12px; padding:8px 12px;");
    }
    m_sendBtn->setStyleSheet("font-size:16px; font-weight:700; background:#7a2ea8; color:white; border:1px solid #a15bd1; border-radius:12px; padding:8px 12px;");
    m_startBtn->setStyleSheet("font-size:16px; font-weight:700; background:#0f766e; color:white; border:1px solid #14b8a6; border-radius:12px; padding:8px 12px;");
    m_stopBtn->setStyleSheet("font-size:16px; font-weight:700; background:#b91c1c; color:white; border:1px solid #ef4444; border-radius:12px; padding:8px 12px;");
    m_clearBtn->setStyleSheet("font-size:16px; font-weight:700; background:#17304c; color:white; border:1px solid #2d5b89; border-radius:12px; padding:8px 12px;");

    m_log = new QPlainTextEdit;
    m_log->setReadOnly(true);
    m_log->setPlaceholderText("IrDA TX/RX log will appear here...");
    m_log->setStyleSheet("background:#ffffff; color:#1f2937; font-family:monospace; font-size:12px; border:1px solid #cdd6e1; border-radius:10px;");

    auto *sendRow = new QWidget;
    auto *sendRowLayout = new QHBoxLayout(sendRow);
    sendRowLayout->setContentsMargins(0, 0, 0, 0);
    sendRowLayout->setSpacing(12);
    sendRowLayout->addWidget(m_sendInput, 1);
    sendRowLayout->addWidget(m_sendBtn);

    auto *rxRow = new QWidget;
    auto *rxRowLayout = new QHBoxLayout(rxRow);
    rxRowLayout->setContentsMargins(0, 0, 0, 0);
    rxRowLayout->setSpacing(12);
    rxRowLayout->addWidget(m_startBtn);
    rxRowLayout->addWidget(m_stopBtn);
    rxRowLayout->addWidget(m_clearBtn);

    layout->addWidget(title);
    layout->addWidget(desc);
    layout->addWidget(m_status);
    layout->addWidget(sendRow);
    layout->addWidget(rxRow);
    layout->addWidget(m_log, 1);

    connect(m_sendBtn, &QPushButton::clicked, this, &IrdaTest::sendTest);
    connect(m_startBtn, &QPushButton::clicked, this, &IrdaTest::startRx);
    connect(m_stopBtn, &QPushButton::clicked, this, &IrdaTest::stopRx);
    connect(m_clearBtn, &QPushButton::clicked, this, &IrdaTest::clearLog);

    m_stopBtn->setEnabled(false);
}

IrdaTest::~IrdaTest() {
    stopRx();
}

void IrdaTest::setStatus(const QString& text, bool error) {
    if (!m_status) return;
    m_status->setText(text);
    m_status->setStyleSheet(QString("color:%1; font-size:13px; font-weight:700;")
                            .arg(error ? "#b91c1c" : "#0f1724"));
}

void IrdaTest::appendLog(const QString& text) {
    if (!m_log) return;
    m_log->appendPlainText(QDateTime::currentDateTime().toString(Qt::ISODate) + " " + text);
}

void IrdaTest::sendTest() {
    const QString text = m_sendInput && !m_sendInput->text().isEmpty() ? m_sendInput->text() : QStringLiteral("TEST123");
    QProcess p;
    p.start("/bin/sh", {"-lc", QString("stty -F /dev/ttyACM0 9600 raw -echo -echoe -echok -crtscts && printf '%1' > /dev/ttyACM0").arg(text)});
    if (!p.waitForFinished(3000) || p.exitStatus() != QProcess::NormalExit || p.exitCode() != 0) {
        setStatus("IrDA send failed", true);
        appendLog("TX failed");
        return;
    }
    setStatus(QString("IrDA sent: %1").arg(text), false);
    appendLog(QString("TX: %1").arg(text));
}

void IrdaTest::startRx() {
    stopRx();

    QProcess p;
    p.start("/bin/sh", {"-lc", "stty -F /dev/ttyACM0 9600 raw -echo -echoe -echok -crtscts min 0 time 1"});
    if (!p.waitForFinished(3000) || p.exitStatus() != QProcess::NormalExit || p.exitCode() != 0) {
        setStatus("IrDA RX setup failed", true);
        appendLog("stty setup failed");
        return;
    }

    m_uartFile = new QFile("/dev/ttyACM0", this);
    if (!m_uartFile->open(QIODevice::ReadOnly | QIODevice::Unbuffered)) {
        setStatus("IrDA RX open failed", true);
        appendLog("open /dev/ttyACM0 failed");
        delete m_uartFile;
        m_uartFile = nullptr;
        return;
    }

    m_rxNotifier = new QSocketNotifier(m_uartFile->handle(), QSocketNotifier::Read, this);
    connect(m_rxNotifier, &QSocketNotifier::activated, this, &IrdaTest::handleReadyRead);

    setStatus("UART: /dev/ttyACM0 (receiving)", false);
    appendLog("RX started");
    m_startBtn->setEnabled(false);
    m_stopBtn->setEnabled(true);
}

void IrdaTest::stopRx() {
    if (m_rxNotifier) {
        m_rxNotifier->setEnabled(false);
        delete m_rxNotifier;
        m_rxNotifier = nullptr;
    }
    if (m_uartFile) {
        if (m_uartFile->isOpen()) m_uartFile->close();
        delete m_uartFile;
        m_uartFile = nullptr;
    }
    if (m_startBtn) m_startBtn->setEnabled(true);
    if (m_stopBtn) m_stopBtn->setEnabled(false);
    setStatus("UART: /dev/ttyACM0 (stopped)", false);
}

void IrdaTest::clearLog() {
    if (m_log) m_log->clear();
}

void IrdaTest::handleReadyRead() {
    if (!m_uartFile) return;
    const QByteArray data = m_uartFile->readAll();
    if (data.isEmpty()) return;

    QString hex;
    for (unsigned char c : data) {
        hex += QString("%1 ").arg(c, 2, 16, QLatin1Char('0')).toUpper();
    }
    const QString text = QString::fromLatin1(data);
    appendLog(QString("RX %1 bytes | HEX: %2| TXT: %3")
              .arg(data.size())
              .arg(hex)
              .arg(text));
}
