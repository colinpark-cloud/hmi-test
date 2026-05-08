#include "irdatest.h"

#include <QDateTime>
#include <QFile>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QTextStream>
#include <QVBoxLayout>
#include <QHBoxLayout>

IrdaTest::IrdaTest(QWidget* parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(14);

    auto *title = new QLabel("IrDA Test");
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size:22px; font-weight:700; color:#17212f;");

    auto *desc = new QLabel("Send MCP2120 / TFDU4101 test data through /dev/ttyACM0.");
    desc->setAlignment(Qt::AlignCenter);
    desc->setWordWrap(true);
    desc->setStyleSheet("color:#5f6b7a; font-size:13px;");

    m_status = new QLabel("UART: /dev/ttyACM0");
    m_status->setAlignment(Qt::AlignCenter);
    m_status->setStyleSheet("color:#0f1724; font-size:13px; font-weight:700;");

    m_input = new QLineEdit;
    m_input->setPlaceholderText("Send text to /dev/ttyACM0");
    m_input->setMinimumHeight(42);
    m_input->setStyleSheet("font-size:15px; padding:6px 10px; border:1px solid #cdd6e1; border-radius:10px;");

    m_sendBtn = new QPushButton("Send IrDA Test");
    m_sendBtn->setMinimumHeight(44);
    m_sendBtn->setStyleSheet("font-size:16px; font-weight:700; background:#0f766e; color:white; border:1px solid #14b8a6; border-radius:12px; padding:8px 12px;");

    m_log = new QPlainTextEdit;
    m_log->setReadOnly(true);
    m_log->setPlaceholderText("IrDA send log will appear here...");
    m_log->setStyleSheet("background:#ffffff; color:#1f2937; font-family:monospace; font-size:12px; border:1px solid #cdd6e1; border-radius:10px;");

    auto *row = new QWidget;
    auto *rowLayout = new QHBoxLayout(row);
    rowLayout->setContentsMargins(0, 0, 0, 0);
    rowLayout->setSpacing(12);
    rowLayout->addWidget(m_input, 1);
    rowLayout->addWidget(m_sendBtn);

    layout->addWidget(title);
    layout->addWidget(desc);
    layout->addWidget(m_status);
    layout->addWidget(row);
    layout->addWidget(m_log, 1);

    connect(m_sendBtn, &QPushButton::clicked, this, &IrdaTest::sendTest);
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
    const QString text = m_input && !m_input->text().isEmpty() ? m_input->text() : QStringLiteral("TEST123");
    QProcess p;
    p.start("/bin/sh", {"-lc", QString("stty -F /dev/ttyACM0 9600 raw -echo -echoe -echok -crtscts && printf '%1' > /dev/ttyACM0").arg(text)});
    if (!p.waitForFinished(3000) || p.exitStatus() != QProcess::NormalExit || p.exitCode() != 0) {
        setStatus("IrDA send failed", true);
        appendLog("send failed");
        return;
    }
    setStatus(QString("IrDA sent: %1").arg(text), false);
    appendLog(QString("sent: %1").arg(text));
}
