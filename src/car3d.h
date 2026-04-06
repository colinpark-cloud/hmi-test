#pragma once
#include <QWidget>
#include <Qt3DCore/QEntity>
#include <Qt3DExtras/qt3dwindow.h>
#include <Qt3DRender/QCamera>

class Car3DWidget : public QWidget {
    Q_OBJECT
public:
    explicit Car3DWidget(QWidget* parent=nullptr);
public slots:
    void setColor(const QColor &c);
    void toggleLights(bool on);
private:
    Qt3DExtras::Qt3DWindow *view;
    QWidget *container;
    Qt3DCore::QEntity *rootEntity;
    Qt3DCore::QEntity *carEntity;
    Qt3DRender::QCamera *camera;
};
