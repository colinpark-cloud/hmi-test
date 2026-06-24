#include "hdmitest.h"

#include <QDateTime>
#include <QFile>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTextStream>
#include <QVBoxLayout>

namespace {
const QString kHdmiBase = "/sys/class/drm/card1-HDMI-A-1";

QString readTextFile(const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return QString();
    return QString::fromUtf8(f.readAll()).trimmed();
}
}

HdmiTest::HdmiTest(QWidget* parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(14);

    auto *title = new QLabel("HDMI Test");
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size:22px; font-weight:700; color:#17212f;");

    auto *desc = new QLabel("Check HDMI connector status and supported modes from /sys/class/drm/card1-HDMI-A-1.");
    desc->setAlignment(Qt::AlignCenter);
    desc->setWordWrap(true);
    desc->setStyleSheet("color:#5f6b7a; font-size:13px;");

    m_status = new QLabel("Status: -");
    m_status->setStyleSheet("font-size:16px; font-weight:700; color:#0f1724;");

    m_mode = new QLabel("Modes: -");
    m_mode->setStyleSheet("font-size:15px; color:#17212f;");
    m_mode->setWordWrap(true);

    m_edid = new QLabel("EDID: -");
    m_edid->setStyleSheet("font-size:15px; color:#17212f;");

    m_refreshBtn = new QPushButton("Refresh HDMI Status");
    m_refreshBtn->setMinimumHeight(44);
    m_refreshBtn->setStyleSheet("font-size:16px; font-weight:700; background:#17304c; color:white; border:1px solid #2d5b89; border-radius:12px; padding:8px 12px;");

    m_log = new QPlainTextEdit;
    m_log->setReadOnly(true);
    m_log->setPlaceholderText("HDMI status log...");
    m_log->setStyleSheet("background:#ffffff; color:#1f2937; font-family:monospace; font-size:12px; border:1px solid #cdd6e1; border-radius:10px;");

    layout->addWidget(title);
    layout->addWidget(desc);
    layout->addWidget(m_status);
    layout->addWidget(m_mode);
    layout->addWidget(m_edid);
    layout->addWidget(m_refreshBtn);
    layout->addWidget(m_log, 1);

    connect(m_refreshBtn, &QPushButton::clicked, this, &HdmiTest::refreshStatus);
    refreshStatus();
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

    const QString modesOneLine = modes.isEmpty() ? QStringLiteral("none") : QString(modes).replace("\n", ", ");
    if (m_status) m_status->setText(QString("Status: %1").arg(status.isEmpty() ? "unknown" : status));
    if (m_mode) m_mode->setText(QString("Modes: %1").arg(modesOneLine));
    if (m_edid) m_edid->setText(QString("EDID: %1").arg(edidState));

    appendLog(QString("status=%1 modes=%2 edid=%3")
              .arg(status.isEmpty() ? "unknown" : status)
              .arg(modesOneLine)
              .arg(edidState));
}
