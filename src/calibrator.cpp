#include "calibrator.h"
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QStandardPaths>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

Calibrator::Calibrator(QWidget* parent): QDialog(parent){
    setWindowTitle("Touch Calibration");
    setModal(true);
    // fix dialog size to avoid Wayland fullscreen protocol errors
    setFixedSize(1024,600);
    // remove window decorations and force stay-on-top so compositor treats it as normal window
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_ShowWithoutActivating);
    auto layout = new QVBoxLayout(this);
    // top control row (compact) to avoid covering targets
    auto ctlRow = new QHBoxLayout;
    auto btnStartLocal = new QPushButton("Start");
    btnStartLocal->setMinimumSize(80,36);
    btnStartLocal->setMaximumWidth(120);
    btnStartLocal->setObjectName("calibStart");
    btnStart = btnStartLocal;
    auto btnCancel = new QPushButton("Cancel");
    btnCancel->setMinimumSize(80,36);
    btnCancel->setMaximumWidth(120);
    btnCancel->setObjectName("calibCancel");
    ctlRow->addWidget(btnStartLocal);
    ctlRow->addStretch(1);
    ctlRow->addWidget(btnCancel);
    layout->addLayout(ctlRow);
    auto lbl = new QLabel("Touch the targets shown (4 points) with your finger\nFollow the big circle and tap it.");
    layout->addWidget(lbl);
    connect(btnStartLocal,&QPushButton::clicked,[this](){ start(); });
    connect(btnCancel,&QPushButton::clicked,[this](){ if(btnStart) btnStart->show(); reject(); });
}

void Calibrator::start(){
    if(btnStart) btnStart->hide(); // hide to avoid covering bottom targets
    srcPoints.clear(); dstPoints.clear(); idx=0; m_valid=false;
    // destination points: 4 corners inset (larger inset to avoid overlap with controls)
    int w = width(); int h = height();
    const int inset = 80;
    dstPoints << QPointF(inset,inset) << QPointF(w-inset,inset) << QPointF(w-inset,h-inset) << QPointF(inset,h-inset);
    showTarget(dstPoints[0]);
}

void Calibrator::keyPressEvent(QKeyEvent* ev){
    if(ev->key()==Qt::Key_Escape){ if(btnStart) btnStart->show(); reject(); }
    else QDialog::keyPressEvent(ev);
}

void Calibrator::showTarget(const QPointF &pt){
    Q_UNUSED(pt);
    update();
}

void Calibrator::mousePressEvent(QMouseEvent* ev){
    if(dstPoints.isEmpty()) return;
    QPointF p = ev->localPos();
    srcPoints << p; // recorded raw reported pos corresponding to dstPoints[idx]
    idx++;
    if(idx<dstPoints.size()){
        showTarget(dstPoints[idx]);
    } else {
        // compute transform mapping srcPoints -> dstPoints
        if(srcPoints.size()>=4){
            QTransform t;
            bool ok = QTransform::quadToQuad(srcPoints, dstPoints, t);
            if(ok){ m_transform = t; m_valid=true;
                // save to file
                QJsonObject obj;
                QJsonArray arr;
                for(int i=0;i<9;i++) arr.append((double)t.m11()); // placeholder
                // better to store matrix values
                QJsonArray mat;
                mat.append(t.m11()); mat.append(t.m12()); mat.append(t.m13());
                mat.append(t.m21()); mat.append(t.m22()); mat.append(t.m23());
                mat.append(t.m31()); mat.append(t.m32()); mat.append(t.m33());
                obj["transform"] = mat;
                QString path = QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/.hmi_touch_calib.json";
                QFile f(path);
                if(f.open(QIODevice::WriteOnly)){
                    f.write(QJsonDocument(obj).toJson()); f.close();
                }
            }
        }
        accept();
    }
}

void Calibrator::paintEvent(QPaintEvent* ev){
    QDialog::paintEvent(ev);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    if(!dstPoints.isEmpty() && idx<dstPoints.size()){
        QPointF d = dstPoints[idx];
        p.setBrush(Qt::red);
        p.setPen(Qt::NoPen);
        p.drawEllipse(d, 20, 20);
    }
}
