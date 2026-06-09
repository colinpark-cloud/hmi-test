#include "pctest.h"
#include "cameraview.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>
#include <QTimer>
#include <QProcess>

PCTest::PCTest(QWidget* parent) : QWidget(parent) {
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(12);

    auto *title = new QLabel("Proximity-Camera Control");
    title->setStyleSheet("font-size:22px; font-weight:800; color:#17212f;");
    root->addWidget(title);

    auto *topRow = new QHBoxLayout;
    auto *infoCol = new QVBoxLayout;

    m_proximityLabel = new QLabel("Proximity: ---");
    m_proximityLabel->setStyleSheet("font-weight:700; font-size:16px;");
    infoCol->addWidget(m_proximityLabel);

    m_cameraStatusLabel = new QLabel("Camera: OFF");
    m_cameraStatusLabel->setStyleSheet("font-weight:700; font-size:16px; color:#dc2626;");
    infoCol->addWidget(m_cameraStatusLabel);
    infoCol->addStretch(1);

    m_cameraView = new CameraView(this);
    m_cameraView->setMinimumHeight(320);

    topRow->addLayout(infoCol, 1);
    topRow->addWidget(m_cameraView, 2);
    root->addLayout(topRow, 1);

    QTimer* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &PCTest::pollProximity);
    timer->start(1000);
}

void PCTest::pollProximity() {
    quint16 proximityValue = 0;
    if (readProximityValue(proximityValue)) {
        m_proximityLabel->setText(QString("Proximity: %1").arg(proximityValue));
        if (!m_cameraOn && proximityValue >= 120) {
            setCameraStatus(true);
        } else if (m_cameraOn && proximityValue <= 80) {
            setCameraStatus(false);
        }
    } else {
        m_proximityLabel->setText("Proximity: ERROR");
        setCameraStatus(false);
    }
}

bool PCTest::readProximityValue(quint16& value) {
    QProcess process;
    process.start("/usr/sbin/i2cget", QStringList() << "-y" << "6" << "0x51" << "0x08" << "w");
    if (!process.waitForFinished(2000)) {
        return false;
    }
    QString output = process.readAllStandardOutput().trimmed();
    bool ok;
    value = output.startsWith("0x") ? output.mid(2).toUShort(&ok, 16) : output.toUShort(&ok, 16);
    return ok;
}

void PCTest::setCameraStatus(bool on) {
    if (m_cameraOn != on) {
        m_cameraOn = on;
        if (m_cameraView) {
            if (on) {
                m_cameraView->startCamera();
                emit cameraActivated();
            } else {
                m_cameraView->stopCamera();
                emit cameraDeactivated();
            }
        }
    }

    if (on) {
        m_cameraStatusLabel->setText("Camera: ON (Person detected)");
        m_cameraStatusLabel->setStyleSheet("font-weight:700; font-size:16px; color:#16a34a;");
    } else {
        m_cameraStatusLabel->setText("Camera: OFF (No person)");
        m_cameraStatusLabel->setStyleSheet("font-weight:700; font-size:16px; color:#dc2626;");
    }
}
