#include "displaytest.h"
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QPainter>
#include <QMouseEvent>

class ColorWidget : public QWidget {
public:
    ColorWidget(QWidget* p=nullptr): QWidget(p){}
    void setColor(const QColor &c){ color=c; update(); }
protected:
    void paintEvent(QPaintEvent*) override { QPainter p(this); p.fillRect(rect(), color); }
private:
    QColor color = Qt::black;
};

DisplayTest::DisplayTest(QWidget* parent): QWidget(parent){
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    cw = new ColorWidget;
    cw->setMinimumSize(960,540);
    layout->addWidget(cw, 1);
    auto btn = new QPushButton("Cycle Colors");
    btn->setFixedHeight(44);
    layout->addWidget(btn);
    connect(btn,&QPushButton::clicked,this,&DisplayTest::cycleColors);
}

void DisplayTest::mousePressEvent(QMouseEvent* ev){
    QWidget::mousePressEvent(ev);
}

void DisplayTest::cycleColors(){
    idx = (idx+1)%6;
    QColor cols[6]={Qt::red,Qt::green,Qt::blue,Qt::yellow,Qt::magenta,Qt::cyan};
    if(cw) cw->setColor(cols[idx]);
}
