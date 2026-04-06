#pragma once
#include <QDialog>
#include <QTransform>
#include <QVector>
#include <QPointF>
#include <QMouseEvent>
#include <QPushButton>
#include <QKeyEvent>

class Calibrator : public QDialog {
public:
    explicit Calibrator(QWidget* parent=nullptr);
    QTransform transform() const { return m_transform; }
    bool isValid() const { return m_valid; }
private:
    void start();
    QTransform m_transform;
    bool m_valid=false;
    QVector<QPointF> srcPoints;
    QVector<QPointF> dstPoints;
    int idx=0;
    void showTarget(const QPointF &pt);
    QPushButton* btnStart=nullptr;
protected:
    void mousePressEvent(QMouseEvent* ev) override;
    void paintEvent(QPaintEvent* ev) override;
    void keyPressEvent(QKeyEvent* ev) override;
};
