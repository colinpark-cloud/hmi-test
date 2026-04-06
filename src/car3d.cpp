#include "car3d.h"
#include <Qt3DExtras/QOrbitCameraController>
#include <Qt3DExtras/QPhongMaterial>
#include <Qt3DExtras/QCuboidMesh>
#include <Qt3DExtras/QPlaneMesh>
#include <QHBoxLayout>

Car3DWidget::Car3DWidget(QWidget* parent): QWidget(parent){
    view = new Qt3DExtras::Qt3DWindow();
    container = QWidget::createWindowContainer(view, this);
    auto layout = new QHBoxLayout(this);
    layout->addWidget(container);
    rootEntity = new Qt3DCore::QEntity();

    // camera
    camera = view->camera();
    camera->lens()->setPerspectiveProjection(45.0f, 16.0f/9.0f, 0.1f, 1000.0f);
    camera->setPosition(QVector3D(0, 5, 20));
    camera->setViewCenter(QVector3D(0,0,0));

    // light and ground
    auto ground = new Qt3DCore::QEntity(rootEntity);
    auto plane = new Qt3DExtras::QPlaneMesh();
    plane->setWidth(40); plane->setHeight(40);
    ground->addComponent(plane);
    auto groundMat = new Qt3DExtras::QPhongMaterial(rootEntity);
    groundMat->setDiffuse(QColor(50,50,50));
    ground->addComponent(groundMat);

    // car entity (simple cuboid body)
    carEntity = new Qt3DCore::QEntity(rootEntity);
    auto body = new Qt3DExtras::QCuboidMesh();
    body->setXExtent(6); body->setYExtent(1.5); body->setZExtent(3);
    carEntity->addComponent(body);
    auto mat = new Qt3DExtras::QPhongMaterial(rootEntity);
    mat->setDiffuse(QColor(200,0,0));
    carEntity->addComponent(mat);

    // orbit controller
    auto camController = new Qt3DExtras::QOrbitCameraController(rootEntity);
    camController->setCamera(camera);

    view->setRootEntity(rootEntity);
}

void Car3DWidget::setColor(const QColor &c){
    // find material on carEntity and set diffuse
    auto comps = carEntity->components();
    for(auto comp: comps){
        if(auto mat = qobject_cast<Qt3DExtras::QPhongMaterial*>(comp)){
            mat->setDiffuse(c);
            return;
        }
    }
}

void Car3DWidget::toggleLights(bool on){
    // simple visual: change ground brightness
    Q_UNUSED(on);
}
