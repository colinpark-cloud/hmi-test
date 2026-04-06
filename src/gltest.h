#pragma once
#include <QOpenGLWidget>
#include <QTimer>
#include <QColor>

class RotGLWidget : public QOpenGLWidget {
    Q_OBJECT
public:
    RotGLWidget(QWidget* parent=nullptr);
    void setCarColor(const QColor &c){ carColor = c; }
protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
private slots:
    void onTimeout();
private:
    float angle=0.0f;
    QTimer timer;
    QColor carColor = Qt::red;
};
