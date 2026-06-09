#pragma once

#include <QWidget>

class QLabel;
class CameraView;

class PCTest : public QWidget {
    Q_OBJECT
public:
    explicit PCTest(QWidget* parent = nullptr);

signals:
    void cameraActivated();
    void cameraDeactivated();

private slots:
    void pollProximity();

private:
    bool readProximityValue(quint16& value);
    void setCameraStatus(bool on);

    bool m_cameraOn = false;
    QLabel* m_proximityLabel = nullptr;
    QLabel* m_cameraStatusLabel = nullptr;
    CameraView* m_cameraView = nullptr;
};
