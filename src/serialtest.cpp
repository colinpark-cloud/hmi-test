#include "serialtest.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QPlainTextEdit>
#include <QDateTime>
#include <QFile>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>

static bool writeTextFile(const QString& path, const QString& value) {
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    f.write(value.toUtf8());
    return true;
}

SerialTest::SerialTest(QWidget* parent): QWidget(parent){
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(12);

    auto *title = new QLabel("Serial Test (RS232 / RS422 / RS485)");
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size:22px; font-weight:700; color:#17212f;");
    auto *desc = new QLabel("Choose the port first, then select RS422 or RS485 when using ttymxc3.");
    desc->setAlignment(Qt::AlignCenter);
    desc->setStyleSheet("color:#5f6b7a; font-size:13px;");
    desc->setWordWrap(true);

    rs232Btn = new QPushButton("RS232");
    rs42xBtn = new QPushButton("RS422 / RS485");
    modeBtn = new QPushButton("Mode: RS422");
    modeHint = new QLabel("ttymxc2 = RS232   |   ttymxc3 = RS422 / RS485");
    modeHint->setStyleSheet("color:#5f6b7a; font-family:monospace; font-size:12px;");

    for (auto *b : {rs232Btn, rs42xBtn, modeBtn}) {
        b->setMinimumHeight(48);
        b->setStyleSheet("font-size:16px; font-weight:700; background:#17304c; color:white; border:1px solid #2d5b89; border-radius:12px; padding:10px 14px;");
    }
    modeBtn->setCheckable(true);

    sendBtn = new QPushButton("Send 'test'");
    sendBtn->setMinimumHeight(48);
    sendBtn->setStyleSheet("font-size:16px; font-weight:700; background:#d97706; color:white; border:1px solid #f59e0b; border-radius:12px; padding:10px 14px;");

    log = new QPlainTextEdit;
    log->setReadOnly(true);
    log->setPlaceholderText("Serial log will appear here...");
    log->setStyleSheet("background:#ffffff; color:#1f2937; font-family:monospace; font-size:12px; border:1px solid #cdd6e1; border-radius:12px;");

    layout->addWidget(title);
    layout->addWidget(desc);
    auto *topRow = new QWidget;
    auto *topRowLayout = new QHBoxLayout(topRow);
    topRowLayout->setContentsMargins(0, 0, 0, 0);
    topRowLayout->setSpacing(8);
    topRowLayout->addWidget(rs232Btn, 1);
    topRowLayout->addWidget(rs42xBtn, 1);
    topRowLayout->addWidget(modeBtn, 1);

    auto *actionRow = new QWidget;
    auto *actionRowLayout = new QHBoxLayout(actionRow);
    actionRowLayout->setContentsMargins(0, 0, 0, 0);
    actionRowLayout->setSpacing(8);
    actionRowLayout->addWidget(sendBtn, 1);

    layout->addWidget(topRow);
    layout->addWidget(modeHint);
    layout->addWidget(actionRow);
    layout->addWidget(log, 1);

    connect(rs232Btn, &QPushButton::clicked, this, [=]() {
        if(fd!=-1){ ::close(fd); fd=-1; }
        portOpen = false;
        rs232Btn->setChecked(true);
        rs42xBtn->setChecked(false);
        rs485Mode = false;
        updatePortButtons();
        updateModeButton();
        appendLog("selected: RS232 /dev/ttymxc2");
        openPort();
    });
    connect(rs42xBtn, &QPushButton::clicked, this, [=]() {
        if(fd!=-1){ ::close(fd); fd=-1; }
        portOpen = false;
        rs232Btn->setChecked(false);
        rs42xBtn->setChecked(true);
        updatePortButtons();
        updateModeButton();
        appendLog("selected: RS422/RS485 /dev/ttymxc3");
        openPort();
    });
    connect(modeBtn, &QPushButton::clicked, this, [=]() {
        if (!isRs42x()) return;
        rs485Mode = !rs485Mode;
        updateModeButton();
        appendLog(rs485Mode ? "mode selected: RS485" : "mode selected: RS422");
    });
    connect(sendBtn,&QPushButton::clicked,this,[=]() {
        appendLog("send requested: test");
        sendTest();
    });

    updatePortButtons();
    updateModeButton();
}

void SerialTest::appendLog(const QString& line){
    if (!log) return;
    log->appendPlainText(QDateTime::currentDateTime().toString(Qt::ISODate) + " " + line);
}

QString SerialTest::currentPort() const {
    return isRs42x() ? QString("/dev/ttymxc3") : QString("/dev/ttymxc2");
}

bool SerialTest::isRs232() const {
    return rs232Btn && rs232Btn->isChecked();
}

bool SerialTest::isRs42x() const {
    return rs42xBtn && rs42xBtn->isChecked();
}

void SerialTest::updatePortButtons(){
    if (!rs232Btn || !rs42xBtn) return;
    const auto on = QString("font-size:16px; font-weight:700; background:#cdebd9; color:#0f3d27; border:1px solid #8bc59e; border-radius:10px; padding:10px 14px;");
    const auto off = QString("font-size:16px; font-weight:600; background:#e8f1fb; color:#0f1724; border:1px solid #b8c7d9; border-radius:10px; padding:10px 14px;");
    rs232Btn->setCheckable(true);
    rs42xBtn->setCheckable(true);
    rs232Btn->setChecked(isRs232());
    rs42xBtn->setChecked(isRs42x());
    rs232Btn->setStyleSheet(isRs232() ? on : off);
    rs42xBtn->setStyleSheet(isRs42x() ? on : off);
}

void SerialTest::updateModeButton(){
    if (!modeBtn || !modeHint) return;
    if (!isRs42x()) {
        modeBtn->setEnabled(false);
        modeBtn->setText("Mode: RS422");
        modeBtn->setChecked(false);
        modeBtn->setStyleSheet("font-size:16px; font-weight:600; background:#eef3f8; color:#9aa7b5; border:1px solid #d5dce4; border-radius:10px; padding:10px 14px;");
        modeHint->setText("Select RS422 / RS485 first");
        return;
    }
    modeBtn->setEnabled(true);
    modeBtn->setChecked(rs485Mode);
    modeBtn->setText(rs485Mode ? "Mode: RS485" : "Mode: RS422");
    modeBtn->setStyleSheet(rs485Mode
        ? "font-size:16px; font-weight:700; background:#cdebd9; color:#0f3d27; border:1px solid #8bc59e; border-radius:10px; padding:10px 14px;"
        : "font-size:16px; font-weight:600; background:#e8f1fb; color:#0f1724; border:1px solid #b8c7d9; border-radius:10px; padding:10px 14px;");
    modeHint->setText("ttymxc3: low = RS422, high = RS485");
}

void SerialTest::openPort(){
    if (portOpen) {
        if(fd!=-1){ ::close(fd); fd=-1; }
        portOpen = false;
        appendLog(QString("port closed: %1").arg(currentPort()));
        return;
    }

    if (isRs232()) {
        writeTextFile("/sys/class/leds/COM_TYPE/brightness", "0");
        writeTextFile("/sys/class/leds/COM_TYPE_COM2/brightness", "1");
    } else if (isRs42x()) {
        writeTextFile("/sys/class/leds/COM_TYPE_COM2/brightness", "0");
        writeTextFile("/sys/class/leds/COM_TYPE/brightness", rs485Mode ? "1" : "0");
    }
    fd = open(currentPort().toStdString().c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if(fd<0){
        appendLog("open failed");
        perror("open");
        return;
    }
    struct termios tio; memset(&tio,0,sizeof(tio));
    cfsetispeed(&tio,B115200); cfsetospeed(&tio,B115200);
    tio.c_cflag = CS8 | CLOCAL | CREAD;
    tio.c_iflag = IGNPAR;
    tio.c_oflag = 0; tio.c_lflag = 0;
    tcflush(fd, TCIFLUSH);
    tcsetattr(fd,TCSANOW,&tio);
    portOpen = true;
    appendLog(QString("port opened: %1").arg(currentPort()));
    updatePortButtons();
    updateModeButton();
}

void SerialTest::sendTest(){
    if(fd!=-1){
        write(fd,"test\n",5);
        appendLog("sent: test");
    } else {
        appendLog("send skipped: port not open");
    }
}
