#pragma once
#include <QWidget>
#include <QTransform>

class ColorWidget;

class DisplayTest : public QWidget {
    Q_OBJECT
public:
    DisplayTest(QWidget* parent=nullptr);
    void setCalibration(const QTransform &t){ m_calib = t; }
protected:
    void mousePressEvent(QMouseEvent* ev) override;
private slots:
    void cycleColors();
private:
    int idx=0;
    ColorWidget* cw = nullptr;
    QTransform m_calib;
};
