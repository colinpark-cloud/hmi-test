#include "commtest.h"

#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QProcess>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <cstring>

static QString panelStyle() {
    return "QWidget{background:#f7f9fc; border:1px solid #d8e0ea; border-radius:14px;}";
}
static QString titleStyle() {
    return "QLabel{font-size:18px; font-weight:800; color:#17212f;}";
}
static QString detailStyle() {
    return "QLabel{font-size:15px; color:#5f6b7a;}";
}
static QString runStyle(bool running) {
    return running
        ? "QPushButton{font-size:16px; font-weight:800; background:#22c55e; color:white; border:1px solid #16a34a; border-radius:14px; padding:12px 14px;}"
          "QPushButton:pressed{padding-top:14px; padding-bottom:10px;}"
        : "QPushButton{font-size:16px; font-weight:800; background:#e8f1fb; color:#0f1724; border:1px solid #b8c7d9; border-radius:14px; padding:12px 14px;}"
          "QPushButton:pressed{padding-top:14px; padding-bottom:10px;}";
}
static QString badgeStyle(const QString& bg, const QString& fg, const QString& border) {
    return QString("QLabel{font-size:22px; font-weight:900; background:%1; color:%2; border:1px solid %3; border-radius:10px; min-height:48px;}")
        .arg(bg, fg, border);
}

static bool applyIpAddress(const QString& iface, const QString& ip) {
    const QStringList candidates = {
        "ifconfig",
        "/sbin/ifconfig",
        "/usr/sbin/ifconfig",
        "/bin/ifconfig"
    };
    QString ifconfig;
    for (const auto& c : candidates) {
        if (QFile::exists(c)) { ifconfig = c; break; }
    }
    if (ifconfig.isEmpty()) return false;
    const QStringList args = {iface, ip, "netmask", "255.255.255.0", "up"};
    return QProcess::execute(ifconfig, args) == 0;
}

static QString peerLanIp(const QString& ip) {
    QStringList parts = ip.split('.');
    if (parts.size() != 4) return ip;
    bool ok = false;
    int last = parts.last().toInt(&ok);
    if (!ok) return ip;
    if ((last % 2) == 0) ++last; else --last;
    if (last < 0 || last > 255) return ip;
    parts[3] = QString::number(last);
    return parts.join('.');
}

CommTest::CommTest(QWidget* parent) : QWidget(parent) {
    buildUi();
    m_timer = new QTimer(this);
    m_timer->setInterval(1000);
    connect(m_timer, &QTimer::timeout, this, &CommTest::onTick);
    m_clockTimer = new QTimer(this);
    m_clockTimer->setInterval(1000);
    connect(m_clockTimer, &QTimer::timeout, this, &CommTest::updateClock);
    m_clockTimer->start();
    setRunButton();
}

void CommTest::buildUi() {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 0, 12, 4);
    layout->setSpacing(1);

    auto *grid = new QGridLayout;
    m_clockLabel = new QLabel;
    m_clockLabel->setFixedHeight(32);
    m_clockLabel->setStyleSheet("QLabel{font-size:14px; font-weight:800; color:#1f2937; background:#ffffff; border:1px solid #cdd6e1; border-radius:10px; padding:1px 10px;}");
    layout->addWidget(m_clockLabel);
    updateClock();
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setHorizontalSpacing(10);
    grid->setVerticalSpacing(2);
    grid->setColumnStretch(0, 4);
    grid->setColumnStretch(1, 1);
    grid->setColumnStretch(2, 1);
    grid->setColumnStretch(3, 1);

    auto *headName = new QLabel(" ");
    auto *headCount = new QLabel("Count");
    auto *headPass = new QLabel("PASS");
    auto *headFail = new QLabel("FAIL");
    for (auto *h : {headName, headCount, headPass, headFail}) {
        h->setAlignment(Qt::AlignCenter);
        h->setStyleSheet("font-size:19px; font-weight:900; color:#5f6b7a;");
    }
    grid->addWidget(headName, 0, 0);
    grid->addWidget(headCount, 0, 1);
    grid->addWidget(headPass, 0, 2);
    grid->addWidget(headFail, 0, 3);

    m_rows = {
        {TargetKind::Com1, "COM1", "RS485", "/dev/ttymxc3", QString(), nullptr, nullptr, nullptr, nullptr, 0, 0, 0},
        {TargetKind::Com2, "COM2", "RS232", "/dev/ttymxc2", QString(), nullptr, nullptr, nullptr, nullptr, 0, 0, 0},
        {TargetKind::Lan1, "LAN1", "192.168.1.100", QString(), "192.168.1.100", nullptr, nullptr, nullptr, nullptr, 0, 0, 0},
        {TargetKind::Lan2, "LAN2", "192.168.2.100", QString(), "192.168.2.100", nullptr, nullptr, nullptr, nullptr, 0, 0, 0},
    };

    for (int i = 0; i < m_rows.size(); ++i) {
        auto &row = m_rows[i];

        auto *rowWidget = new QWidget;
        rowWidget->setStyleSheet(panelStyle());
        auto *rowLayout = new QHBoxLayout(rowWidget);
        rowLayout->setContentsMargins(10, 4, 10, 4);
        rowLayout->setSpacing(10);

        auto *left = new QWidget;
        auto *leftLayout = new QVBoxLayout(left);
        leftLayout->setContentsMargins(0, 0, 0, 0);
        leftLayout->setSpacing(2);
        row.titleLabel = new QLabel(row.title);
        row.titleLabel->setStyleSheet(titleStyle());
        leftLayout->addWidget(row.titleLabel);
        if (row.kind == TargetKind::Lan1 || row.kind == TargetKind::Lan2) {
            auto *buttonRow = new QWidget;
            auto *buttonLayout = new QHBoxLayout(buttonRow);
            buttonLayout->setContentsMargins(0, 0, 0, 0);
            buttonLayout->setSpacing(6);
            const QStringList values = (row.kind == TargetKind::Lan1)
                ? QStringList({"192.168.1.100", "192.168.1.101", "192.168.1.102", "192.168.1.103"})
                : QStringList({"192.168.2.100", "192.168.2.101", "192.168.2.102", "192.168.2.103"});
            row.ip = values.value(0);
            const QString iface = (row.kind == TargetKind::Lan1) ? "eth0" : "eth1";
            for (int j = 0; j < 4; ++j) {
                row.ipButtons[j] = new QPushButton(values.value(j));
                row.ipButtons[j]->setCheckable(true);
                row.ipButtons[j]->setFixedWidth(128);
                row.ipButtons[j]->setMinimumHeight(32);
                row.ipButtons[j]->setStyleSheet("QPushButton{font-size:12px; font-weight:700; background:#f1f5f9; color:#17212f; border:1px solid #c9d3df; border-radius:9px; padding:3px 6px;} QPushButton:checked{background:#cdebd9; color:#0f3d27; border:1px solid #8bc59e;}");
                buttonLayout->addWidget(row.ipButtons[j]);
                connect(row.ipButtons[j], &QPushButton::clicked, this, [this, &row, values, iface, j]() {
                    for (int k = 0; k < 4; ++k) {
                        if (row.ipButtons[k]) row.ipButtons[k]->setChecked(k == j);
                    }
                    const QString ip = values.value(j);
                    if (applyIpAddress(iface, ip)) {
                        row.ip = ip;
                    }
                });
            }
            row.ipButtons[0]->setChecked(true);
            leftLayout->addWidget(buttonRow);
        } else {
            auto *spacer = new QWidget;
            spacer->setFixedHeight(42);
            row.detailLabel = new QLabel(row.detail);
            row.detailLabel->setStyleSheet(detailStyle());
            leftLayout->addWidget(row.detailLabel);
        }

        row.totalLabel = new QLabel("0");
        row.passLabel = new QLabel("0");
        row.failLabel = new QLabel("0");
        for (auto *label : {row.totalLabel, row.passLabel, row.failLabel}) {
            label->setAlignment(Qt::AlignCenter);
            label->setMinimumWidth(84);
        }
        row.totalLabel->setStyleSheet(badgeStyle("#eef3f8", "#17212f", "#c9d3df"));
        row.passLabel->setStyleSheet(badgeStyle("#dff3e6", "#0f3d27", "#8bc59e"));
        row.failLabel->setStyleSheet(badgeStyle("#fae0e2", "#842029", "#f1aeb5"));

        rowLayout->addWidget(left, 4);
        rowLayout->addWidget(row.totalLabel, 1);
        rowLayout->addWidget(row.passLabel, 1);
        rowLayout->addWidget(row.failLabel, 1);
        grid->addWidget(rowWidget, i + 1, 0, 1, 4);
    }

    auto *bottom = new QHBoxLayout;
    bottom->setContentsMargins(0, 0, 0, 0);
    bottom->setSpacing(10);

    m_runBtn = new QPushButton("off");
    m_runBtn->setFixedHeight(64);
    m_runBtn->setMinimumWidth(140);
    m_runBtn->setStyleSheet(runStyle(false));
    bottom->addStretch(1);
    bottom->addWidget(m_runBtn);

    layout->addLayout(grid);
    layout->addLayout(bottom);

    connect(m_runBtn, &QPushButton::clicked, this, &CommTest::toggleRun);
}

void CommTest::setRunButton() {
    m_runBtn->setText(m_running ? "on" : "off");
    m_runBtn->setStyleSheet(runStyle(m_running));
}

void CommTest::resetCounters() {
    m_cycle = 0;
    for (auto &row : m_rows) {
        row.totalCount = 0;
        row.passCount = 0;
        row.failCount = 0;
        updateRow(row);
    }
}

void CommTest::updateRow(TargetRow& row) {
    if (row.totalLabel) row.totalLabel->setText(QString::number(row.totalCount));
    if (row.passLabel) row.passLabel->setText(QString::number(row.passCount));
    if (row.failLabel) row.failLabel->setText(QString::number(row.failCount));
}

bool CommTest::checkComPort(const QString& device) const {
    QFileInfo info(device);
    return info.exists();
}

bool CommTest::checkLanLink(const QString& ip) const {
    const QStringList candidates = {
        "ping",
        "/bin/ping",
        "/usr/bin/ping",
        "/sbin/ping",
        "/usr/sbin/ping"
    };
    QString pingCmd;
    for (const auto& c : candidates) {
        if (QFile::exists(c)) { pingCmd = c; break; }
    }
    if (pingCmd.isEmpty() || ip.isEmpty()) return false;

    QProcess proc;
    proc.start(pingCmd, {"-n", "-c", "1", "-W", "1", ip});
    if (!proc.waitForStarted(1000)) return false;
    if (!proc.waitForFinished(1500)) {
        proc.kill();
        proc.waitForFinished(200);
        return false;
    }
    return proc.exitStatus() == QProcess::NormalExit && proc.exitCode() == 0;
}

bool CommTest::checkLanAnyLink(const QStringList& ips, const QString& skipIp) const {
    for (const auto& ip : ips) {
        if (!skipIp.isEmpty() && ip == skipIp) continue;
        if (checkLanLink(ip)) return true;
    }
    return false;
}

bool CommTest::checkSerialRoundTrip(const QString& device) const {
    const QByteArray payload("test\n");
    int fd = ::open(device.toStdString().c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) return false;

    struct termios tio;
    memset(&tio, 0, sizeof(tio));
    cfsetispeed(&tio, B115200);
    cfsetospeed(&tio, B115200);
    tio.c_cflag = CS8 | CLOCAL | CREAD;
    tio.c_iflag = IGNPAR;
    tio.c_oflag = 0;
    tio.c_lflag = 0;
    tcflush(fd, TCIFLUSH);
    tcsetattr(fd, TCSANOW, &tio);

    const ssize_t wr = ::write(fd, payload.constData(), payload.size());
    if (wr != payload.size()) {
        ::close(fd);
        return false;
    }

    char buf[64] = {0};
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);
    timeval tv{0, 200000};
    const int sel = ::select(fd + 1, &rfds, nullptr, nullptr, &tv);
    bool ok = false;
    if (sel > 0 && FD_ISSET(fd, &rfds)) {
        const ssize_t rd = ::read(fd, buf, sizeof(buf));
        ok = (rd > 0);
    }

    ::close(fd);
    return ok;
}

void CommTest::bumpTotal(int rowIndex) {
    if (rowIndex < 0 || rowIndex >= m_rows.size()) return;
    auto &row = m_rows[rowIndex];
    row.totalCount++;
    updateRow(row);
}

void CommTest::addResult(int rowIndex, bool ok) {
    if (rowIndex < 0 || rowIndex >= m_rows.size()) return;
    auto &row = m_rows[rowIndex];
    if (ok) row.passCount++; else row.failCount++;
    updateRow(row);
}

void CommTest::onTick() {
    if (!m_running) return;
    ++m_cycle;
    updateClock();
    for (int i = 0; i < m_rows.size(); ++i) {
        bumpTotal(i);
    }

    addResult(0, checkSerialRoundTrip(m_rows[0].device));
    addResult(1, checkSerialRoundTrip(m_rows[1].device));
    const QStringList lan1Peers = {"192.168.1.100", "192.168.1.101", "192.168.1.102", "192.168.1.103"};
    const QStringList lan2Peers = {"192.168.2.100", "192.168.2.101", "192.168.2.102", "192.168.2.103"};
    addResult(2, checkLanAnyLink(lan1Peers, m_rows[2].ip));
    addResult(3, checkLanAnyLink(lan2Peers, m_rows[3].ip));
}

void CommTest::updateClock() {
    if (!m_clockLabel) return;
    const auto now = QDateTime::currentDateTime();
    const QString days[] = {"일", "월", "화", "수", "목", "금", "토"};
    const QString text = QString("%1/%2/%3(%4) %5:%6:%7")
        .arg(now.date().year())
        .arg(now.date().month(), 2, 10, QChar('0'))
        .arg(now.date().day(), 2, 10, QChar('0'))
        .arg(days[now.date().dayOfWeek() - 1])
        .arg(now.time().hour(), 2, 10, QChar('0'))
        .arg(now.time().minute(), 2, 10, QChar('0'))
        .arg(now.time().second(), 2, 10, QChar('0'));
    m_clockLabel->setText(text);
}

void CommTest::toggleRun() {
    m_running = !m_running;
    setRunButton();
    if (m_running) {
        if (m_timer) m_timer->start();
    } else {
        if (m_timer) m_timer->stop();
        resetCounters();
    }
}
