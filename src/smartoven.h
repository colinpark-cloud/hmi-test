#pragma once
#include <QWidget>
#include <QTimer>

class QLabel;
class QPushButton;
class QSlider;

class SmartOven : public QWidget {
    Q_OBJECT
public:
    SmartOven(QWidget* parent=nullptr);
protected:
    void paintEvent(QPaintEvent* ev) override;
private slots:
    void togglePower();
    void changeMode();
    void setTarget(int v);
    void tick();
private:
    bool power=false;
    int mode=0; // 0=bake,1=grill,2=toast
    int targetTemp=180;
    int currentTemp=25;
    int timerMinutes=0;
    QTimer timer;
    QLabel* lblTemp=nullptr;
    QLabel* lblMode=nullptr;
    QPushButton* btnPower=nullptr;
    QPushButton* btnStart=nullptr;
    QSlider* sTemp=nullptr;
};
