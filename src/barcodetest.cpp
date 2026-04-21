#include "barcodetest.h"

#include <QDateTime>
#include <QEvent>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

static QString panelStyle() {
    return "QWidget{background:#f7f9fc; border:1px solid #d8e0ea; border-radius:14px;}";
}

BarcodeTest::BarcodeTest(QWidget* parent) : QWidget(parent) {
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(12);

    auto *card = new QWidget;
    card->setStyleSheet(panelStyle());
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(18, 18, 18, 18);
    layout->setSpacing(10);

    m_title = new QLabel("Barcode Scanner");
    m_title->setStyleSheet("font-size:22px; font-weight:800; color:#17212f;");
    m_hint = new QLabel("Scan a barcode. The scanner acts like a keyboard, then sends Enter.");
    m_hint->setWordWrap(true);
    m_hint->setStyleSheet("color:#5f6b7a; font-size:14px;");

    m_status = new QLabel("Waiting for scan...");
    m_status->setStyleSheet("color:#2563eb; font-weight:700; font-size:14px;");

    m_lastScan = new QLabel("-");
    m_lastScan->setStyleSheet("background:#ffffff; border:1px solid #cdd6e1; border-radius:10px; padding:10px; font-size:20px; font-weight:700; color:#0f1724;");

    m_input = new QLineEdit;
    m_input->setPlaceholderText("Focus here and scan...");
    m_input->setStyleSheet("background:#ffffff; border:2px solid #2d5b89; border-radius:10px; padding:10px 12px; font-size:18px;");
    m_input->setMinimumHeight(44);
    m_input->installEventFilter(this);

    m_clearBtn = new QPushButton("Clear History");
    m_clearBtn->setStyleSheet("font-size:15px; font-weight:700; background:#17304c; color:white; border:1px solid #2d5b89; border-radius:10px; padding:8px 12px;");
    m_clearBtn->setMinimumHeight(40);

    m_history = new QListWidget;
    m_history->setStyleSheet("background:#ffffff; border:1px solid #cdd6e1; border-radius:10px; font-size:14px; color:#17212f;");

    auto *buttonRow = new QHBoxLayout;
    buttonRow->addWidget(m_clearBtn);
    buttonRow->addStretch(1);

    layout->addWidget(m_title);
    layout->addWidget(m_hint);
    layout->addWidget(m_status);
    layout->addWidget(m_lastScan);
    layout->addWidget(m_input);
    layout->addLayout(buttonRow);
    layout->addWidget(m_history, 1);
    root->addWidget(card, 1);

    connect(m_input, &QLineEdit::returnPressed, this, &BarcodeTest::commitScan);
    connect(m_clearBtn, &QPushButton::clicked, this, &BarcodeTest::clearScans);

    QTimer::singleShot(0, this, &BarcodeTest::ensureFocus);
}

void BarcodeTest::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    QTimer::singleShot(0, this, &BarcodeTest::ensureFocus);
}

bool BarcodeTest::eventFilter(QObject* obj, QEvent* ev) {
    if (obj == m_input) {
        if (ev->type() == QEvent::KeyPress) {
            auto *ke = static_cast<QKeyEvent*>(ev);
            if (ke->key() == Qt::Key_F5) {
                clearScans();
                return true;
            }
        }
    }
    return QWidget::eventFilter(obj, ev);
}

void BarcodeTest::ensureFocus() {
    if (!m_input) return;
    m_input->setFocus(Qt::OtherFocusReason);
    m_input->selectAll();
}

QString BarcodeTest::sanitizeScan(const QString& text) const {
    QString s = text.trimmed();
    s.replace('\r', ' ');
    s.replace('\n', ' ');
    return s.simplified();
}

void BarcodeTest::setStatus(const QString& text) {
    if (m_status) m_status->setText(text);
}

void BarcodeTest::commitScan() {
    if (!m_input) return;
    const QString raw = m_input->text();
    const QString scan = sanitizeScan(raw);
    m_input->clear();
    if (scan.isEmpty()) {
        setStatus("Empty scan ignored");
        ensureFocus();
        return;
    }
    ++m_scanCount;
    const QString stamp = QDateTime::currentDateTime().toString("HH:mm:ss");
    if (m_history) {
        m_history->insertItem(0, QString("[%1] %2  |  %3").arg(stamp).arg(m_scanCount).arg(scan));
    }
    if (m_lastScan) m_lastScan->setText(scan);
    setStatus(QString("Scanned %1 item(s) at %2").arg(m_scanCount).arg(stamp));
    ensureFocus();
}

void BarcodeTest::clearScans() {
    if (m_history) m_history->clear();
    m_scanCount = 0;
    if (m_lastScan) m_lastScan->setText("-");
    setStatus("History cleared");
    ensureFocus();
}
