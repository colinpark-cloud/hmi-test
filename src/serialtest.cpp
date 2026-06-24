#include "serialtest.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QPlainTextEdit>
#include <QDateTime>
#include <QFile>
#include <QSocketNotifier>
#include <QComboBox>
#include <QTimer>
#include <QSplitter>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>

static const QString BTN_ON  = "font-size:14px;font-weight:700;background:#1a5c38;color:white;border:1px solid #2d8a57;border-radius:10px;padding:8px 12px;";
static const QString BTN_OFF = "font-size:14px;font-weight:600;background:#e8f1fb;color:#0f1724;border:1px solid #b8c7d9;border-radius:10px;padding:8px 12px;";
static const QString BTN_DIS = "font-size:14px;font-weight:600;background:#f0f3f7;color:#9aa7b5;border:1px solid #d5dce4;border-radius:10px;padding:8px 12px;";
static const QString BTN_ACT = "font-size:14px;font-weight:700;background:#d97706;color:white;border:1px solid #f59e0b;border-radius:10px;padding:8px 12px;";
static const QString BTN_TX  = "font-size:14px;font-weight:700;background:#1d4ed8;color:white;border:1px solid #3b82f6;border-radius:10px;padding:8px 12px;";
static const QString BTN_RX  = "font-size:14px;font-weight:700;background:#7c3aed;color:white;border:1px solid #a78bfa;border-radius:10px;padding:8px 12px;";

static bool writeTextFile(const QString &path, const QString &value) {
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    f.write(value.toUtf8());
    return true;
}

SerialTest::SerialTest(QWidget *parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(8);

    auto *title = new QLabel("Serial Test  (COM1 / COM2 / COM3)");
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size:20px;font-weight:700;color:#17212f;");

    auto *desc = new QLabel(
        "COM1 = ttymxc3  |  COM2 = ttymxc2  |  COM3 = ttyACM0 (MCP2221A)\n"
        "COM1_SEL / COM2_SEL :  0 = RS232   1 = RS422/485");
    desc->setAlignment(Qt::AlignCenter);
    desc->setStyleSheet("color:#5f6b7a;font-size:12px;font-family:monospace;");
    desc->setWordWrap(true);

    // ── Port row ──────────────────────────────────────────────────────────
    com1Btn = new QPushButton("COM1\nttymxc3");
    com2Btn = new QPushButton("COM2\nttymxc2");
    com3Btn = new QPushButton("COM3\nttyACM0");
    for (auto *b : {com1Btn, com2Btn, com3Btn}) b->setMinimumHeight(52);

    auto *portRow = new QWidget;
    auto *portLayout = new QHBoxLayout(portRow);
    portLayout->setContentsMargins(0,0,0,0); portLayout->setSpacing(8);
    portLayout->addWidget(com1Btn, 1);
    portLayout->addWidget(com2Btn, 1);
    portLayout->addWidget(com3Btn, 1);

    // ── Mode row ──────────────────────────────────────────────────────────
    rs232Btn = new QPushButton("RS232\n(SEL=0)");
    rs42xBtn = new QPushButton("RS422 / RS485\n(SEL=1)");
    for (auto *b : {rs232Btn, rs42xBtn}) b->setMinimumHeight(52);

    auto *modeRow = new QWidget;
    auto *modeLayout = new QHBoxLayout(modeRow);
    modeLayout->setContentsMargins(0,0,0,0); modeLayout->setSpacing(8);
    modeLayout->addWidget(rs232Btn, 1);
    modeLayout->addWidget(rs42xBtn, 1);

    // ── TX / RX direction row ─────────────────────────────────────────────
    txDirBtn = new QPushButton("TX  (송신 모드)");
    rxDirBtn = new QPushButton("RX  (수신 모드)");
    for (auto *b : {txDirBtn, rxDirBtn}) b->setMinimumHeight(48);

    auto *dirRow = new QWidget;
    auto *dirLayout = new QHBoxLayout(dirRow);
    dirLayout->setContentsMargins(0,0,0,0); dirLayout->setSpacing(8);
    dirLayout->addWidget(txDirBtn, 1);
    dirLayout->addWidget(rxDirBtn, 1);

    // ── Baud + Open row ───────────────────────────────────────────────────
    baudBox = new QComboBox;
    baudBox->setStyleSheet("font-size:14px;padding:6px 10px;border:1px solid #b8c7d9;border-radius:8px;background:#fff;");
    baudBox->setMinimumHeight(40);
    for (const char *b : {"1200","2400","4800","9600","19200","38400","57600","115200"})
        baudBox->addItem(b);
    baudBox->setCurrentText("9600");

    openBtn = new QPushButton("Open");
    openBtn->setMinimumHeight(40);

    auto *baudRow = new QWidget;
    auto *baudLayout = new QHBoxLayout(baudRow);
    baudLayout->setContentsMargins(0,0,0,0); baudLayout->setSpacing(8);
    baudLayout->addWidget(new QLabel("Baud:"));
    baudLayout->addWidget(baudBox, 1);
    baudLayout->addWidget(openBtn, 1);

    statusLabel = new QLabel("포트를 선택하세요");
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setStyleSheet("color:#5f6b7a;font-size:12px;font-family:monospace;");

    // ── TX controls (only visible in TX mode) ─────────────────────────────
    sendBtn = new QPushButton("Send  'test'");
    sendBtn->setMinimumHeight(44);
    sendBtn->setStyleSheet(BTN_ACT);

    autoBtn = new QPushButton("Auto Send  OFF");
    autoBtn->setMinimumHeight(44);
    autoBtn->setStyleSheet(BTN_OFF);

    auto *txCtrlRow = new QWidget;
    auto *txCtrlLayout = new QHBoxLayout(txCtrlRow);
    txCtrlLayout->setContentsMargins(0,0,0,0); txCtrlLayout->setSpacing(8);
    txCtrlLayout->addWidget(sendBtn, 1);
    txCtrlLayout->addWidget(autoBtn, 1);

    // ── TX log ────────────────────────────────────────────────────────────
    auto *txLabel = new QLabel("TX  (송신)");
    txLabel->setStyleSheet("font-size:13px;font-weight:700;color:#1d4ed8;");
    txLog = new QPlainTextEdit;
    txLog->setReadOnly(true);
    txLog->setPlaceholderText("송신 로그...");
    txLog->setStyleSheet("background:#eff6ff;color:#1e3a5f;font-family:monospace;font-size:12px;"
                         "border:1px solid #93c5fd;border-radius:10px;");
    txLog->setMaximumHeight(160);

    // ── RX log ────────────────────────────────────────────────────────────
    auto *rxLabel = new QLabel("RX  (수신)");
    rxLabel->setStyleSheet("font-size:13px;font-weight:700;color:#7c3aed;");
    rxLog = new QPlainTextEdit;
    rxLog->setReadOnly(true);
    rxLog->setPlaceholderText("수신 로그...");
    rxLog->setStyleSheet("background:#f5f3ff;color:#3b1a6e;font-family:monospace;font-size:12px;"
                         "border:1px solid #c4b5fd;border-radius:10px;");

    layout->addWidget(title);
    layout->addWidget(desc);
    layout->addWidget(portRow);
    layout->addWidget(modeRow);
    layout->addWidget(dirRow);
    layout->addWidget(baudRow);
    layout->addWidget(statusLabel);
    layout->addWidget(txCtrlRow);
    layout->addWidget(txLabel);
    layout->addWidget(txLog);
    layout->addWidget(rxLabel);
    layout->addWidget(rxLog, 1);

    // ── Auto-send timer ───────────────────────────────────────────────────
    autoTimer = new QTimer(this);
    autoTimer->setInterval(1000);
    connect(autoTimer, &QTimer::timeout, this, &SerialTest::sendOnce);

    rxFlushTimer = new QTimer(this);
    rxFlushTimer->setSingleShot(true);
    rxFlushTimer->setInterval(50);
    connect(rxFlushTimer, &QTimer::timeout, this, [this]() {
        if (!rxBuf.isEmpty()) {
            QString s = QString::fromLatin1(rxBuf).trimmed();
            rxBuf.clear();
            if (!s.isEmpty()) appendRx(s);
        }
    });

    // ── Connections ───────────────────────────────────────────────────────
    connect(com1Btn,  &QPushButton::clicked, this, [=]() { selectPort(Port::COM1); });
    connect(com2Btn,  &QPushButton::clicked, this, [=]() { selectPort(Port::COM2); });
    connect(com3Btn,  &QPushButton::clicked, this, [=]() { selectPort(Port::COM3); });
    connect(rs232Btn, &QPushButton::clicked, this, [=]() { selectMode(Mode::RS232); });
    connect(rs42xBtn, &QPushButton::clicked, this, [=]() { selectMode(Mode::RS42x); });
    connect(txDirBtn, &QPushButton::clicked, this, [=]() { selectDir(TestDir::TX); });
    connect(rxDirBtn, &QPushButton::clicked, this, [=]() { selectDir(TestDir::RX); });
    connect(openBtn,  &QPushButton::clicked, this, &SerialTest::openPort);
    connect(sendBtn,  &QPushButton::clicked, this, &SerialTest::sendOnce);
    connect(autoBtn,  &QPushButton::clicked, this, &SerialTest::toggleAutoSend);

    updateUI();
}

// ── Slots ─────────────────────────────────────────────────────────────────

void SerialTest::onReadReady() {
    char buf[256];
    ssize_t n = ::read(fd, buf, sizeof(buf) - 1);
    if (n <= 0) return;
    rxBuf.append(buf, static_cast<int>(n));

    // flush complete lines immediately
    while (true) {
        int nl = rxBuf.indexOf('\n');
        if (nl < 0) break;
        QString s = QString::fromLatin1(rxBuf.left(nl)).trimmed();
        rxBuf.remove(0, nl + 1);
        if (!s.isEmpty()) appendRx(s);
    }
    // flush remainder after 50ms silence (for data without newline)
    if (!rxBuf.isEmpty())
        rxFlushTimer->start();
}

void SerialTest::sendOnce() {
    if (fd < 0 || !portOpen) { appendTx("전송 실패: 포트가 열려있지 않음"); return; }
    if (curDir != TestDir::TX)  { appendTx("TX 모드가 아님"); return; }
    const char msg[] = "test\n";
    ::write(fd, msg, sizeof(msg) - 1);
    appendTx("test");
}

void SerialTest::toggleAutoSend() {
    if (!portOpen || curDir != TestDir::TX) return;
    autoSending = !autoSending;
    if (autoSending) autoTimer->start();
    else             autoTimer->stop();
    updateUI();
}

// ── Selection helpers ─────────────────────────────────────────────────────

void SerialTest::selectPort(Port p) {
    if (portOpen) {
        autoTimer->stop(); autoSending = false;
        delete notifier; notifier = nullptr;
        ::close(fd); fd = -1; portOpen = false;
    }
    curPort = p;
    if (p == Port::COM3) curMode = Mode::RS232;
    updateUI();
    appendTx(QString("포트 선택: %1").arg(portDevice()));
}

void SerialTest::selectMode(Mode m) {
    if (curPort == Port::COM3) return;
    curMode = m;
    updateUI();
    appendTx(QString("모드: %1  (SEL=%2)").arg(m == Mode::RS232 ? "RS232" : "RS422/485").arg(m == Mode::RS232 ? 0 : 1));
}

void SerialTest::selectDir(TestDir d) {
    if (autoSending) { autoTimer->stop(); autoSending = false; }
    curDir = d;
    updateUI();
    appendTx(curDir == TestDir::TX ? "→ TX 송신 모드" : "← RX 수신 모드");
}

// ── Port device path ─────────────────────────────────────────────────────

QString SerialTest::portDevice() const {
    switch (curPort) {
    case Port::COM1: return "/dev/ttymxc3";
    case Port::COM2: return "/dev/ttymxc2";
    case Port::COM3: return "/dev/ttyACM0";
    default:         return "";
    }
}

// ── GPIO ─────────────────────────────────────────────────────────────────

void SerialTest::applyGpio() {
    const QString sel = (curMode == Mode::RS42x) ? "1" : "0";
    if (curPort == Port::COM1) {
        writeTextFile("/sys/class/leds/COM_TYPE/brightness",      sel);
        writeTextFile("/sys/class/leds/COM_TYPE_COM2/brightness", "0");
    } else if (curPort == Port::COM2) {
        writeTextFile("/sys/class/leds/COM_TYPE/brightness",      "0");
        writeTextFile("/sys/class/leds/COM_TYPE_COM2/brightness", sel);
    }
    appendTx(QString("GPIO  COM1_SEL=%1  COM2_SEL=%2")
        .arg(curPort == Port::COM1 ? sel : "0")
        .arg(curPort == Port::COM2 ? sel : "0"));
}

// ── Open / Close ─────────────────────────────────────────────────────────

void SerialTest::openPort() {
    if (curPort == Port::None) { appendTx("포트를 먼저 선택하세요"); return; }
    if (portOpen) {
        autoTimer->stop(); autoSending = false;
        delete notifier; notifier = nullptr;
        ::close(fd); fd = -1; portOpen = false;
        appendTx(QString("닫힘: %1").arg(portDevice()));
        updateUI();
        return;
    }
    rxBuf.clear();
    rxFlushTimer->stop();
    applyGpio();
    fd = ::open(portDevice().toStdString().c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) {
        appendTx(QString("open 실패: %1  (errno=%2)").arg(portDevice()).arg(errno));
        return;
    }
    static const struct { int bps; speed_t spd; } baudTable[] = {
        {1200,B1200},{2400,B2400},{4800,B4800},{9600,B9600},
        {19200,B19200},{38400,B38400},{57600,B57600},{115200,B115200}
    };
    speed_t speed = B9600;
    int selectedBaud = baudBox->currentText().toInt();
    for (auto &e : baudTable) if (e.bps == selectedBaud) { speed = e.spd; break; }

    struct termios tio; memset(&tio, 0, sizeof(tio));
    cfsetispeed(&tio, speed); cfsetospeed(&tio, speed);
    tio.c_cflag = CS8 | CLOCAL | CREAD;
    tio.c_iflag = IGNPAR;
    tcflush(fd, TCIFLUSH);
    tcsetattr(fd, TCSANOW, &tio);
    portOpen = true;

    // RX notifier always active (both modes can receive)
    notifier = new QSocketNotifier(fd, QSocketNotifier::Read, this);
    connect(notifier, &QSocketNotifier::activated, this, &SerialTest::onReadReady);

    QString modeStr = curPort == Port::COM3 ? "RS485(MCP2221A)" : curMode == Mode::RS232 ? "RS232" : "RS422/485";
    appendTx(QString("열림: %1  [%2]  [%3]")
        .arg(portDevice()).arg(modeStr)
        .arg(curDir == TestDir::TX ? "TX모드" : "RX모드"));
    updateUI();
}

// ── Log helpers ───────────────────────────────────────────────────────────

void SerialTest::appendTx(const QString &line) {
    txLog->appendPlainText(QDateTime::currentDateTime().toString("hh:mm:ss") + "  " + line);
}

void SerialTest::appendRx(const QString &line) {
    rxLog->appendPlainText(QDateTime::currentDateTime().toString("hh:mm:ss") + "  " + line);
}

// ── UI update ─────────────────────────────────────────────────────────────

void SerialTest::updateUI() {
    com1Btn->setStyleSheet(curPort == Port::COM1 ? BTN_ON : BTN_OFF);
    com2Btn->setStyleSheet(curPort == Port::COM2 ? BTN_ON : BTN_OFF);
    com3Btn->setStyleSheet(curPort == Port::COM3 ? BTN_ON : BTN_OFF);

    bool modeEnabled = (curPort == Port::COM1 || curPort == Port::COM2);
    rs232Btn->setEnabled(modeEnabled);
    rs42xBtn->setEnabled(modeEnabled);
    rs232Btn->setStyleSheet(!modeEnabled ? BTN_DIS : curMode == Mode::RS232 ? BTN_ON : BTN_OFF);
    rs42xBtn->setStyleSheet(!modeEnabled ? BTN_DIS : curMode == Mode::RS42x ? BTN_ON : BTN_OFF);

    txDirBtn->setStyleSheet(curDir == TestDir::TX ? BTN_TX : BTN_OFF);
    rxDirBtn->setStyleSheet(curDir == TestDir::RX ? BTN_RX : BTN_OFF);

    if (curPort == Port::None) {
        openBtn->setText("Open"); openBtn->setStyleSheet(BTN_DIS); openBtn->setEnabled(false);
    } else if (portOpen) {
        openBtn->setText("Close");
        openBtn->setStyleSheet("font-size:14px;font-weight:700;background:#b91c1c;color:white;border:1px solid #ef4444;border-radius:10px;padding:8px 12px;");
        openBtn->setEnabled(true);
    } else {
        openBtn->setText("Open"); openBtn->setStyleSheet(BTN_OFF); openBtn->setEnabled(true);
    }

    bool isTx = (curDir == TestDir::TX) && portOpen;
    sendBtn->setEnabled(isTx);
    autoBtn->setEnabled(isTx);
    sendBtn->setStyleSheet(isTx ? BTN_ACT : BTN_DIS);
    autoBtn->setText(autoSending ? "Auto Send  ON  ■" : "Auto Send  OFF");
    autoBtn->setStyleSheet(autoSending ? BTN_ON : (isTx ? BTN_OFF : BTN_DIS));

    if (curPort == Port::None) {
        statusLabel->setText("포트를 선택하세요");
    } else {
        QString modeStr = curPort == Port::COM3 ? "RS485(MCP2221A)"
                        : curMode == Mode::RS232  ? "RS232(SEL=0)" : "RS422/485(SEL=1)";
        QString dirStr  = curDir == TestDir::TX ? "→ TX" : "← RX";
        statusLabel->setText(QString("%1  ·  %2  ·  %3  ·  %4")
            .arg(portDevice()).arg(modeStr).arg(dirStr)
            .arg(portOpen ? "● 열림" : "○ 닫힘"));
    }
}
