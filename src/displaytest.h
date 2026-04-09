#pragma once

#include <QTransform>
#include <QWidget>

class ColorWidget;
class QLabel;
class QPushButton;
class QSlider;
class QTimer;

class DisplayTest : public QWidget {
    Q_OBJECT
public:
    explicit DisplayTest(QWidget* parent = nullptr);
    void setCalibration(const QTransform &t) { m_calib = t; }

signals:
    void started();
    void finished();

protected:
    bool eventFilter(QObject* obj, QEvent* ev) override;
    void resizeEvent(QResizeEvent* ev) override;

private:
    void startManual();
    void startAuto();
    void startBrightnessAuto();
    void tickBrightnessAuto();
    void onBrightnessSliderChanged(int value);
    void advanceColor();
    void resetToIntro();
    bool writeBacklight(int value);
    int readBacklightMax() const;
    int readBacklightCurrent() const;

    int idx = -1;
    ColorWidget* cw = nullptr;
    QWidget* intro = nullptr;
    QLabel* brightnessValue = nullptr;
    QSlider* brightnessSlider = nullptr;
    QPushButton* autoBtn = nullptr;
    QPushButton* brightnessAutoBtn = nullptr;
    QTimer* autoTimer = nullptr;
    QTimer* brightnessTimer = nullptr;
    bool brightnessAutoMode = false;
    bool brightnessRising = false;
    int brightnessCurrent = 0;
    int brightnessMax = 255;
    int brightnessStep = 1;
    int savedBrightness = -1;
    QString backlightPath = "/sys/class/backlight/backlight-lvds/brightness";
    QString backlightMaxPath = "/sys/class/backlight/backlight-lvds/max_brightness";
    QTransform m_calib;
};
