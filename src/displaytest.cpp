#include "displaytest.h"

#include <QApplication>
#include <QFile>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QResizeEvent>
#include <QSlider>
#include <QTextStream>
#include <QTimer>
#include <QVBoxLayout>

class ColorWidget : public QWidget {
public:
    explicit ColorWidget(QWidget* p = nullptr) : QWidget(p) {
        setAttribute(Qt::WA_AcceptTouchEvents, true);
        setAttribute(Qt::WA_NoSystemBackground, true);
        setAttribute(Qt::WA_OpaquePaintEvent, true);
        setFocusPolicy(Qt::StrongFocus);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }
    void setColor(const QColor &c) { color = c; update(); }
protected:
    void paintEvent(QPaintEvent*) override { QPainter p(this); p.fillRect(rect(), color); }
private:
    QColor color = Qt::black;
};

DisplayTest::DisplayTest(QWidget* parent) : QWidget(parent) {
    setContentsMargins(0, 0, 0, 0);
    savedBrightness = readBacklightCurrent();

    intro = new QWidget(this);
    auto *introLayout = new QVBoxLayout(intro);
    introLayout->setContentsMargins(18, 22, 18, 22);
    introLayout->setSpacing(10);

    auto *title = new QLabel("Display Test", intro);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size:22px; font-weight:700; color:#243041;");
    introLayout->addSpacing(10);
    introLayout->addWidget(title);

    auto *btnRow = new QWidget(intro);
    auto *btnLayout = new QHBoxLayout(btnRow);
    btnLayout->setContentsMargins(0, 0, 0, 0);
    btnLayout->setSpacing(10);

    auto *manualBtn = new QPushButton("Manual", btnRow);
    manualBtn->setFixedSize(180, 68);
    manualBtn->setStyleSheet("font-size:20px; font-weight:700; background:#17304c; color:white; border:1px solid #2d5b89; border-radius:14px;");
    autoBtn = new QPushButton("Auto", btnRow);
    autoBtn->setFixedSize(180, 68);
    autoBtn->setStyleSheet("font-size:20px; font-weight:700; background:#7a2ea8; color:white; border:1px solid #a15bd1; border-radius:14px;");
    brightnessAutoBtn = new QPushButton("Brightness Auto", btnRow);
    brightnessAutoBtn->setFixedSize(220, 68);
    brightnessAutoBtn->setStyleSheet("font-size:18px; font-weight:700; background:#d97706; color:white; border:1px solid #f59e0b; border-radius:14px;");

    btnLayout->addStretch(1);
    btnLayout->addWidget(manualBtn);
    btnLayout->addWidget(autoBtn);
    btnLayout->addWidget(brightnessAutoBtn);
    btnLayout->addStretch(1);
    introLayout->addWidget(btnRow);

    auto *brightnessRow = new QWidget(intro);
    auto *brightnessRowLayout = new QVBoxLayout(brightnessRow);
    brightnessRowLayout->setContentsMargins(0, 0, 0, 0);
    brightnessRowLayout->setSpacing(6);

    brightnessSlider = new QSlider(Qt::Horizontal, brightnessRow);
    brightnessSlider->setTracking(true);
    brightnessSlider->setRange(0, readBacklightMax());
    brightnessSlider->setValue(readBacklightMax());
    brightnessSlider->setFixedWidth(400);
    brightnessSlider->setStyleSheet("QSlider::groove:horizontal{height:10px;background:#d1d5db;border-radius:5px;} QSlider::sub-page:horizontal{background:#f59e0b;border-radius:5px;} QSlider::add-page:horizontal{background:#e5e7eb;border-radius:5px;} QSlider::handle:horizontal{background:#111827;border:1px solid #111827;width:28px;height:28px;margin:-10px 0;border-radius:14px;}");
    brightnessSlider->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    brightnessValue = new QLabel(brightnessRow);
    brightnessValue->setAlignment(Qt::AlignCenter);
    brightnessValue->setStyleSheet("font-size:18px; font-weight:700; color:#1f2937; background:transparent;");

    brightnessRowLayout->addWidget(brightnessSlider, 0, Qt::AlignHCenter);
    brightnessRowLayout->addWidget(brightnessValue, 0, Qt::AlignHCenter);
    introLayout->addWidget(brightnessRow);

    auto *hint = new QLabel("Manual = touch to change color   |   Auto = automatic sequence   |   Brightness Auto = backlight ramp", intro);
    hint->setAlignment(Qt::AlignCenter);
    hint->setStyleSheet("color:#5f6b7a; font-size:13px;");
    introLayout->addWidget(hint);
    introLayout->addSpacing(14);

    cw = new ColorWidget(this);
    cw->setGeometry(rect());
    cw->hide();
    cw->installEventFilter(this);

    autoTimer = new QTimer(this);
    autoTimer->setInterval(850);
    connect(autoTimer, &QTimer::timeout, this, &DisplayTest::advanceColor);

    brightnessTimer = new QTimer(this);
    brightnessTimer->setInterval(1);
    connect(brightnessTimer, &QTimer::timeout, this, &DisplayTest::tickBrightnessAuto);

    connect(manualBtn, &QPushButton::clicked, this, &DisplayTest::startManual);
    connect(autoBtn, &QPushButton::clicked, this, &DisplayTest::startAuto);
    connect(brightnessSlider, &QSlider::valueChanged, this, &DisplayTest::onBrightnessSliderChanged);
    connect(brightnessSlider, &QSlider::sliderMoved, this, &DisplayTest::onBrightnessSliderChanged);
    connect(brightnessSlider, &QSlider::sliderPressed, this, [this]() {
        if (brightnessSlider) onBrightnessSliderChanged(brightnessSlider->value());
    });
    connect(brightnessAutoBtn, &QPushButton::clicked, this, &DisplayTest::startBrightnessAuto);

    brightnessMax = qMax(1, readBacklightMax());
    brightnessSlider->setRange(0, brightnessMax);
    brightnessSlider->setValue(brightnessMax);
    brightnessCurrent = brightnessMax;
    brightnessValue->setText(QString("Brightness: %1 / %2").arg(brightnessCurrent).arg(brightnessMax));
}

void DisplayTest::startManual() {
    idx = -1;
    brightnessAutoMode = false;
    if (autoTimer) autoTimer->stop();
    if (brightnessTimer) brightnessTimer->stop();
    intro->hide();
    cw->show();
    cw->raise();
    emit started();
    advanceColor();
}

void DisplayTest::startAuto() {
    idx = -1;
    brightnessAutoMode = false;
    if (brightnessTimer) brightnessTimer->stop();
    intro->hide();
    cw->show();
    cw->raise();
    emit started();
    advanceColor();
    if (autoTimer) autoTimer->start();
}

void DisplayTest::onBrightnessSliderChanged(int value) {
    brightnessCurrent = value;
    writeBacklight(value);
    if (brightnessValue) {
        brightnessValue->setText(QString("Brightness: %1 / %2").arg(value).arg(brightnessMax));
    }
}

void DisplayTest::startBrightnessAuto() {
    brightnessMax = qMax(1, readBacklightMax());
    brightnessCurrent = brightnessMax;
    brightnessAutoMode = true;
    brightnessRising = false;
    brightnessStep = 5;
    brightnessSlider->setRange(0, brightnessMax);
    brightnessSlider->setValue(brightnessCurrent);
    brightnessValue->setText(QString("Brightness: %1 / %2").arg(brightnessCurrent).arg(brightnessMax));
    writeBacklight(brightnessCurrent);
    if (brightnessTimer) {
        brightnessTimer->stop();
        brightnessTimer->start();
    }
}

void DisplayTest::tickBrightnessAuto() {
    if (!brightnessAutoMode) return;
    if (!brightnessRising) {
        brightnessCurrent -= brightnessStep;
        if (brightnessCurrent <= 0) {
            brightnessCurrent = 0;
            brightnessRising = true;
        }
    } else {
        brightnessCurrent += brightnessStep;
        if (brightnessCurrent >= brightnessMax) {
            brightnessCurrent = brightnessMax;
            brightnessAutoMode = false;
            if (brightnessTimer) brightnessTimer->stop();
        }
    }
    onBrightnessSliderChanged(brightnessCurrent);
}

void DisplayTest::resetToIntro() {
    if (cw) cw->hide();
    if (intro) intro->show();
    emit finished();
}

bool DisplayTest::eventFilter(QObject* obj, QEvent* ev) {
    if (obj == cw && (ev->type() == QEvent::MouseButtonPress || ev->type() == QEvent::TouchBegin)) {
        if (autoTimer && autoTimer->isActive()) return true;
        advanceColor();
        return true;
    }
    return QWidget::eventFilter(obj, ev);
}

void DisplayTest::resizeEvent(QResizeEvent* ev) {
    QWidget::resizeEvent(ev);
    if (cw) cw->setGeometry(rect());
    if (intro) intro->setGeometry(rect());
}

void DisplayTest::advanceColor() {
    idx++;
    QColor cols[6] = {QColor("#101820"), QColor("#1756c6"), QColor("#1a7f37"), QColor("#d4a017"), QColor("#b31b1b"), QColor("#7a2ea8")};
    if (cw) cw->setColor(cols[idx % 6]);
    if (idx >= 5) {
        if (autoTimer) autoTimer->stop();
        resetToIntro();
    }
}

bool DisplayTest::writeBacklight(int value) {
    QFile f(backlightPath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    QTextStream ts(&f);
    ts << value;
    return true;
}

int DisplayTest::readBacklightMax() const {
    QFile f(backlightMaxPath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return 100;
    const int v = QString::fromUtf8(f.readAll()).trimmed().toInt();
    return v > 0 ? v : 100;
}

int DisplayTest::readBacklightCurrent() const {
    QFile f(backlightPath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return -1;
    return QString::fromUtf8(f.readAll()).trimmed().toInt();
}
