#include "hdmitest.h"
#include "cameraview.h"

#include <QDateTime>
#include <QFile>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScreen>
#include <QTextStream>
#include <QVBoxLayout>
#include <QWindow>

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>

namespace {
const QString kHdmiBase = "/sys/class/drm/card1-HDMI-A-1";
const char   kCtlSock[] = "/tmp/mirror-ctl.sock";

QString readTextFile(const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return QString();
    return QString::fromUtf8(f.readAll()).trimmed();
}

static const char* kBtnBase =
    "font-size:15px; font-weight:700; border-radius:10px; padding:8px 18px; min-height:40px;";
static const char* kBtnActive =
    "font-size:15px; font-weight:700; border-radius:10px; padding:8px 18px; min-height:40px;"
    "background:#17304c; color:white; border:2px solid #2d5b89;";
static const char* kBtnInactive =
    "font-size:15px; font-weight:700; border-radius:10px; padding:8px 18px; min-height:40px;"
    "background:#e8edf4; color:#455a74; border:2px solid #c5cfe0;";
}

HdmiTest::HdmiTest(QWidget* parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(14);

    auto *title = new QLabel("HDMI Output Control");
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size:22px; font-weight:700; color:#17212f;");

    /* --- Mode control section --- */
    auto *ctlFrame = new QWidget;
    ctlFrame->setStyleSheet("QWidget{background:#f0f4f8; border:1px solid #d0dae6; border-radius:12px;}");
    auto *ctlLayout = new QVBoxLayout(ctlFrame);
    ctlLayout->setContentsMargins(16, 14, 16, 14);
    ctlLayout->setSpacing(10);

    auto *ctlTitle = new QLabel("HDMI 출력 모드");
    ctlTitle->setStyleSheet("font-size:14px; font-weight:700; color:#455a74; border:none; background:transparent;");

    auto *btnRow = new QWidget;
    btnRow->setStyleSheet("background:transparent; border:none;");
    auto *btnLayout = new QHBoxLayout(btnRow);
    btnLayout->setContentsMargins(0, 0, 0, 0);
    btnLayout->setSpacing(10);

    m_offBtn    = new QPushButton("출력 OFF");
    m_mirrorBtn = new QPushButton("복제");
    m_extendBtn = new QPushButton("확장 (카메라)");

    m_offBtn->setMinimumWidth(110);
    m_mirrorBtn->setMinimumWidth(110);
    m_extendBtn->setMinimumWidth(140);

    btnLayout->addWidget(m_offBtn);
    btnLayout->addWidget(m_mirrorBtn);
    btnLayout->addWidget(m_extendBtn);
    btnLayout->addStretch();

    m_currentMode = new QLabel("현재 모드: 복제");
    m_currentMode->setStyleSheet("font-size:13px; color:#17212f; border:none; background:transparent;");

    ctlLayout->addWidget(ctlTitle);
    ctlLayout->addWidget(btnRow);
    ctlLayout->addWidget(m_currentMode);

    /* --- Status section --- */
    m_status = new QLabel("Status: -");
    m_status->setStyleSheet("font-size:15px; font-weight:700; color:#0f1724;");

    m_mode = new QLabel("Modes: -");
    m_mode->setStyleSheet("font-size:14px; color:#17212f;");
    m_mode->setWordWrap(true);

    m_edid = new QLabel("EDID: -");
    m_edid->setStyleSheet("font-size:14px; color:#17212f;");

    m_refreshBtn = new QPushButton("Refresh HDMI Status");
    m_refreshBtn->setMinimumHeight(40);
    m_refreshBtn->setStyleSheet("font-size:14px; font-weight:700; background:#17304c; color:white; border:1px solid #2d5b89; border-radius:10px; padding:6px 12px;");

    m_log = new QPlainTextEdit;
    m_log->setReadOnly(true);
    m_log->setPlaceholderText("HDMI log...");
    m_log->setStyleSheet("background:#ffffff; color:#1f2937; font-family:monospace; font-size:12px; border:1px solid #cdd6e1; border-radius:10px;");

    layout->addWidget(title);
    layout->addWidget(ctlFrame);
    layout->addSpacing(6);
    layout->addWidget(m_status);
    layout->addWidget(m_mode);
    layout->addWidget(m_edid);
    layout->addWidget(m_refreshBtn);
    layout->addWidget(m_log, 1);

    connect(m_offBtn,    &QPushButton::clicked, this, &HdmiTest::onOutputOff);
    connect(m_mirrorBtn, &QPushButton::clicked, this, &HdmiTest::onMirrorMode);
    connect(m_extendBtn, &QPushButton::clicked, this, &HdmiTest::onExtendMode);
    connect(m_refreshBtn, &QPushButton::clicked, this, &HdmiTest::refreshStatus);

    updateModeButtons(1); /* default: mirror */
    refreshStatus();
}

HdmiTest::~HdmiTest() {
    closeHdmiWindow();
}

void HdmiTest::appendLog(const QString& text) {
    if (!m_log) return;
    m_log->appendPlainText(QDateTime::currentDateTime().toString(Qt::ISODate) + " " + text);
}

void HdmiTest::refreshStatus() {
    const QString status = readTextFile(kHdmiBase + "/status");
    const QString modes = readTextFile(kHdmiBase + "/modes");
    QFile edidFile(kHdmiBase + "/edid");
    QString edidState = "missing";
    if (edidFile.exists()) {
        if (edidFile.open(QIODevice::ReadOnly)) {
            const QByteArray edid = edidFile.readAll();
            edidState = edid.isEmpty() ? "empty" : QString("%1 bytes").arg(edid.size());
        } else {
            edidState = "unreadable";
        }
    }

    const QString modesOneLine = modes.isEmpty() ? "none" : QString(modes).replace("\n", ", ");
    if (m_status) m_status->setText(QString("Status: %1").arg(status.isEmpty() ? "unknown" : status));
    if (m_mode) m_mode->setText(QString("Modes: %1").arg(modesOneLine));
    if (m_edid) m_edid->setText(QString("EDID: %1").arg(edidState));

    appendLog(QString("status=%1 edid=%2").arg(status.isEmpty() ? "unknown" : status).arg(edidState));
}

void HdmiTest::sendMirrorCmd(const char* cmd) {
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        appendLog("socket error: cannot open control socket");
        return;
    }
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, kCtlSock, sizeof(addr.sun_path) - 1);

    if (::connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
        ::write(fd, cmd, strlen(cmd));
        appendLog(QString("sent: %1").arg(cmd).trimmed());
    } else {
        appendLog("mirror-ctl socket not available (weston plugin not loaded?)");
    }
    ::close(fd);
}

void HdmiTest::closeHdmiWindow() {
    if (m_hdmiCamera) {
        m_hdmiCamera->stopCamera();
        m_hdmiCamera = nullptr;
    }
    if (m_hdmiWindow) {
        m_hdmiWindow->close();
        m_hdmiWindow->deleteLater();
        m_hdmiWindow = nullptr;
    }
    /* Restore default app_id for any future windows */
    QGuiApplication::setDesktopFileName("hmi-test");
}

void HdmiTest::updateModeButtons(int mode) {
    m_activeMode = mode;
    m_offBtn->setStyleSheet(mode == 0 ? kBtnActive : kBtnInactive);
    m_mirrorBtn->setStyleSheet(mode == 1 ? kBtnActive : kBtnInactive);
    m_extendBtn->setStyleSheet(mode == 2 ? kBtnActive : kBtnInactive);

    static const char* labels[] = {"현재 모드: 출력 OFF", "현재 모드: 복제", "현재 모드: 확장 (카메라)"};
    if (m_currentMode && mode >= 0 && mode <= 2)
        m_currentMode->setText(labels[mode]);
}

void HdmiTest::onOutputOff() {
    closeHdmiWindow();
    sendMirrorCmd("off\n");
    updateModeButtons(0);
    appendLog("HDMI output disabled");
}

void HdmiTest::onMirrorMode() {
    closeHdmiWindow();
    sendMirrorCmd("mirror\n");
    updateModeButtons(1);
    appendLog("HDMI mirror mode enabled");
}

void HdmiTest::onExtendMode() {
    /* Disable mirror plugin first */
    sendMirrorCmd("extend\n");

    /* Find HDMI screen - it's not the primary screen */
    QScreen *hdmiScreen = nullptr;
    QScreen *primary = QGuiApplication::primaryScreen();
    for (QScreen *s : QGuiApplication::screens()) {
        if (s != primary) {
            hdmiScreen = s;
            break;
        }
    }

    if (!hdmiScreen) {
        appendLog("HDMI screen not found - is HDMI connected?");
        updateModeButtons(1);
        sendMirrorCmd("mirror\n"); /* revert */
        return;
    }

    appendLog(QString("HDMI screen: %1 %2x%3")
              .arg(hdmiScreen->name())
              .arg(hdmiScreen->geometry().width())
              .arg(hdmiScreen->geometry().height()));

    /* Close any existing HDMI window */
    closeHdmiWindow();

    /*
     * Change app_id to "hmi-test-hdmi" so kiosk-shell routes this new
     * window to HDMI-A-1 (which has app-ids=hmi-test-hdmi in weston.ini).
     * The main window already sent set_app_id("hmi-test") at creation
     * and is unaffected.
     */
    QGuiApplication::setDesktopFileName("hmi-test-hdmi");

    m_hdmiWindow = new QWidget(nullptr, Qt::Window);
    m_hdmiWindow->setWindowTitle("HDMI Camera");
    m_hdmiWindow->setStyleSheet("background:#000000;");

    m_hdmiCamera = new CameraView(m_hdmiWindow);
    auto *wl = new QVBoxLayout(m_hdmiWindow);
    wl->setContentsMargins(0, 0, 0, 0);
    wl->addWidget(m_hdmiCamera);

    /* Associate with HDMI screen before showing */
    m_hdmiWindow->windowHandle(); /* ensure native window created */
    m_hdmiWindow->showFullScreen();

    /* Move to HDMI screen geometry */
    m_hdmiWindow->setGeometry(hdmiScreen->geometry());

    m_hdmiCamera->startCamera();

    updateModeButtons(2);
    appendLog("HDMI extend mode: camera window opened");
}
