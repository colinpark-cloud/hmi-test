#pragma once

#include <QWidget>

class QLabel;
class QPushButton;
class QPlainTextEdit;
class QFile;
class QStackedWidget;
class QQuickWidget;
class CameraView;
class QTimer;

class ProxTest : public QWidget {
    Q_OBJECT
public:
    explicit ProxTest(QWidget* parent = nullptr);
    void setActive(bool active);

private slots:
    void pollProx();
    void setStatus(const QString& text, bool error = false);
    void appendLog(const QString& text);
    void executeI2CCommands();
    void startI2CPolling();
    void reinitializeSensor();

private:
    bool runCmd(const QStringList& args, QString* out = nullptr);
    bool readWord(const QString& bus, const QString& addr, const QString& reg, quint16& value);
    bool writeWord(const QString& bus, const QString& addr, const QString& reg, quint16 value);
    bool initProxSensor();
    int readBacklightMax() const;
    bool writeBacklight(int value);
    int readBacklightCurrent() const;
    int mapAlsToBrightness(quint16 als) const;
    void updateAutoBrightness(quint16 als);
    void tickBrightnessRamp();
    void updatePresenceUi(quint16 ps);

    QLabel* m_status = nullptr;
    QLabel* m_value = nullptr;
    QLabel* m_flagValue = nullptr;
    QLabel* m_lightValue = nullptr;
    QLabel* m_brightnessValue = nullptr;
    QLabel* m_busLabel = nullptr;
    QLabel* m_addrLabel = nullptr;
    QPushButton* m_autoBrightnessBtn = nullptr;
    QPushButton* m_reinitBtn = nullptr;
    QPlainTextEdit* m_log = nullptr;
    QStackedWidget* m_previewStack = nullptr;
    QQuickWidget* m_demoView = nullptr;
    CameraView* m_cameraView = nullptr;
    QString m_bus = "6";
    QString m_addr = "0x51";
    quint16 m_lastPs = 0;
    quint16 m_lastAls = 0;
    quint16 m_lastFlags = 0;
    int m_lastBrightness = 0;
    int m_targetBrightness = 0;
    int m_brightnessMax = 255;
    quint16 m_smoothedAls = 0;
    bool m_autoBrightness = true;
    bool m_initialized = false;
    bool m_personPresent = false;
    bool m_active = true;
    QTimer* m_pollTimer = nullptr;
};
