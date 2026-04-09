#include "gpiotest.h"
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QTimer>
#include <QFile>
#include <fstream>
#include <string>
#include <sstream>

// Fallback GPIO control via sysfs if libgpiod not available on target
struct GPIOTest::Impl {
    std::string led_base = "/sys/class/leds/BUZZER";
};

static bool writeFile(const std::string &path, const std::string &val){
    std::ofstream f(path);
    if(!f.is_open()) return false;
    f<<val;
    return true;
}

GPIOTest::GPIOTest(QWidget* parent): QWidget(parent), d(new Impl){
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(14);

    auto *title = new QLabel("Buzzer Test");
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size:22px; font-weight:700; color:#17212f;");
    auto *desc = new QLabel("Toggle the buzzer LED output using the on-board BUZZER class LED.");
    desc->setAlignment(Qt::AlignCenter);
    desc->setStyleSheet("color:#5f6b7a; font-size:13px;");
    desc->setWordWrap(true);

    buzzerBtn = new QPushButton;
    buzzerBtn->setMinimumHeight(54);
    buzzerBtn->setFixedWidth(240);
    buzzerBtn->setCheckable(true);
    buzzerBtn->setStyleSheet("font-size:18px; font-weight:700; background:#d97706; color:white; border:1px solid #f59e0b; border-radius:12px; padding:10px 14px;");

    statusLabel = new QLabel;
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setStyleSheet("color:#5f6b7a; font-family:monospace; font-size:12px;");

    layout->addStretch(1);
    layout->addWidget(title);
    layout->addWidget(desc);
    layout->addSpacing(8);
    auto *btnRow = new QWidget;
    auto *btnRowLayout = new QHBoxLayout(btnRow);
    btnRowLayout->setContentsMargins(0, 0, 0, 0);
    btnRowLayout->addStretch(1);
    btnRowLayout->addWidget(buzzerBtn);
    btnRowLayout->addStretch(1);
    layout->addWidget(btnRow);
    layout->addWidget(statusLabel);
    layout->addStretch(2);

    // Prepare LED buzzer if needed
    writeFile(d->led_base + "/trigger", "none");
    writeFile(d->led_base + "/brightness", "0");

    connect(buzzerBtn, &QPushButton::clicked, this, [=]() {
        toggleBuzzer();
    });
    updateButtonState();
}

void GPIOTest::toggleBuzzer(){
    buzzerOn = !buzzerOn;
    writeFile(d->led_base + "/brightness", buzzerOn ? "1" : "0");
    updateButtonState();
}

void GPIOTest::updateButtonState(){
    if (!buzzerBtn || !statusLabel) return;
    buzzerBtn->setText(buzzerOn ? "Buzzer: ON" : "Buzzer: OFF");
    buzzerBtn->setChecked(buzzerOn);
    buzzerBtn->setStyleSheet(buzzerOn
        ? "font-size:18px; font-weight:700; background:#1f7a5a; color:white; border:1px solid #166648; border-radius:10px; padding:10px 14px;"
        : "font-size:18px; font-weight:700; background:#d97706; color:white; border:1px solid #f59e0b; border-radius:10px; padding:10px 14px;");
    statusLabel->setText(buzzerOn ? "BUZZER: ON" : "BUZZER: OFF");
}
