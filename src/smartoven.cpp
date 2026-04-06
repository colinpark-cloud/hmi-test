#include "smartoven.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QPainter>

SmartOven::SmartOven(QWidget* parent): QWidget(parent){
    auto v = new QVBoxLayout(this);
    auto top = new QHBoxLayout;
    btnPower = new QPushButton("Power Off"); btnPower->setCheckable(true);
    connect(btnPower,&QPushButton::clicked,this,&SmartOven::togglePower);
    top->addWidget(btnPower);
    lblMode = new QLabel("Mode: Bake");
    top->addWidget(lblMode);
    v->addLayout(top);

    lblTemp = new QLabel("Current: 25°C | Target: 180°C");
    v->addWidget(lblTemp);

    sTemp = new QSlider(Qt::Horizontal); sTemp->setRange(50,250); sTemp->setValue(targetTemp);
    connect(sTemp,&QSlider::valueChanged,this,&SmartOven::setTarget);
    v->addWidget(sTemp);

    auto row = new QHBoxLayout;
    auto btnMode = new QPushButton("Change Mode");
    connect(btnMode,&QPushButton::clicked,this,&SmartOven::changeMode);
    row->addWidget(btnMode);
    btnStart = new QPushButton("Start");
    connect(btnStart,&QPushButton::clicked,[this](){ if(!power) togglePower(); });
    row->addWidget(btnStart);
    v->addLayout(row);

    connect(&timer,&QTimer::timeout,this,&SmartOven::tick);
    timer.start(1000);
}

void SmartOven::togglePower(){
    power = !power;
    btnPower->setText(power?"Power On":"Power Off");
}

void SmartOven::changeMode(){
    mode = (mode+1)%3;
    const char* names[] = {"Bake","Grill","Toast"};
    lblMode->setText(QString("Mode: %1").arg(names[mode]));
}

void SmartOven::setTarget(int v){ targetTemp=v; lblTemp->setText(QString("Current: %1°C | Target: %2°C").arg(currentTemp).arg(targetTemp)); }

void SmartOven::tick(){
    if(power){
        if(currentTemp<targetTemp) currentTemp += qMax(1, (targetTemp-currentTemp)/20);
        else if(currentTemp>targetTemp) currentTemp -= qMax(1, (currentTemp-targetTemp)/40);
    } else {
        if(currentTemp>25) currentTemp -= 1;
    }
    lblTemp->setText(QString("Current: %1°C | Target: %2°C").arg(currentTemp).arg(targetTemp));
    update();
}

void SmartOven::paintEvent(QPaintEvent*){
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    int w = width(); int h = height();
    QRect gauge(10,h-110,w-20,100);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(30,30,30));
    p.drawRect(gauge);
    float frac = float(currentTemp-25)/float(250-25);
    frac = qBound(0.0f, frac, 1.0f);
    QRect fill = gauge.adjusted(5,5,-5,-5);
    fill.setWidth(int(fill.width()*frac));
    p.setBrush(power?QColor(220,80,40):QColor(120,120,120));
    p.drawRect(fill);
}
