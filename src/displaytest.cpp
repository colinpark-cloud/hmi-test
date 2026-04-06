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
    layout->addWidget(new QLabel("Display & Touch Test"));
    cw = new ColorWidget;
    cw->setMinimumSize(800,360);
    layout->addWidget(cw);
    auto btn = new QPushButton("Cycle Colors");
    layout->addWidget(btn);
    connect(btn,&QPushButton::clicked,this,&DisplayTest::cycleColors);
}

void DisplayTest::mousePressEvent(QMouseEvent* ev){
    // apply calibration if present, then forward to child widgets
    QPointF p = ev->localPos();
    if(!m_calib.isIdentity()){
        QPointF tp = m_calib.map(p);
        QMouseEvent newEv(ev->type(), tp.toPoint(), ev->windowPos(), ev->screenPos(), ev->button(), ev->buttons(), ev->modifiers());
        QWidget::mousePressEvent(&newEv);
        return;
    }
    QWidget::mousePressEvent(ev);
}

void DisplayTest::cycleColors(){
    idx = (idx+1)%6;
    QColor cols[6]={Qt::red,Qt::green,Qt::blue,Qt::yellow,Qt::magenta,Qt::cyan};
    if(cw) cw->setColor(cols[idx]);
}
