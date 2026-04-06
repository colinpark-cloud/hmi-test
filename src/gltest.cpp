#include "gltest.h"
#include <QOpenGLFunctions>
#include <QPainter>
#include <QPainterPath>

class GLFuncs : public QOpenGLFunctions {};

RotGLWidget::RotGLWidget(QWidget* parent): QOpenGLWidget(parent){
    connect(&timer,&QTimer::timeout,this,&RotGLWidget::onTimeout);
    timer.start(30);
}

void RotGLWidget::initializeGL(){
    GLFuncs funcs; funcs.initializeOpenGLFunctions();
    glClearColor(0,0,0,1);
}
void RotGLWidget::resizeGL(int w,int h){ Q_UNUSED(w); Q_UNUSED(h); }
void RotGLWidget::paintGL(){
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.translate(width()/2, height()/2 + 20);
    p.rotate(angle);
    // draw simple car body
    QRect bodyRect(-90,-30,180,60);
    p.setBrush(carColor);
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(bodyRect, 12, 12);
    // roof
    QPainterPath roof;
    roof.moveTo(-40,-30);
    roof.lineTo(-10,-55);
    roof.lineTo(40,-55);
    roof.lineTo(60,-30);
    roof.closeSubpath();
    p.setBrush(carColor.darker(110));
    p.drawPath(roof);
    // wheels
    p.setBrush(Qt::black);
    p.drawEllipse(QPointF(-50,30),16,16);
    p.drawEllipse(QPointF(50,30),16,16);
    p.end();
}

void RotGLWidget::onTimeout(){ angle += 1.5f; if(angle>360) angle-=360; update(); }
