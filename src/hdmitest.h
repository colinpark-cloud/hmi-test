#pragma once

#include <QWidget>

class QLabel;
class QPushButton;
class QPlainTextEdit;
class CameraView;

class HdmiTest : public QWidget {
    Q_OBJECT
public:
    explicit HdmiTest(QWidget* parent = nullptr);
    ~HdmiTest();

private slots:
    void refreshStatus();
    void appendLog(const QString& text);
    void onOutputOff();
    void onMirrorMode();
    void onExtendMode();

private:
    void sendMirrorCmd(const char* cmd);
    void closeHdmiWindow();
    void updateModeButtons(int mode); /* 0=off, 1=mirror, 2=extend */

    QLabel* m_status = nullptr;
    QLabel* m_mode = nullptr;
    QLabel* m_edid = nullptr;
    QLabel* m_currentMode = nullptr;
    QPushButton* m_refreshBtn = nullptr;
    QPushButton* m_offBtn = nullptr;
    QPushButton* m_mirrorBtn = nullptr;
    QPushButton* m_extendBtn = nullptr;
    QPlainTextEdit* m_log = nullptr;

    QWidget* m_hdmiWindow = nullptr;   /* extend mode camera window */
    CameraView* m_hdmiCamera = nullptr;
    int m_activeMode = 1; /* 1=mirror by default */
};
