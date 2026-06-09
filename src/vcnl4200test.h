#pragma once

#include <QWidget>

class QLabel;
class QPushButton;
class QSlider;
class QTimer;
class QPlainTextEdit;

class Vcnl4200Test : public QWidget {
    Q_OBJECT
public:
    explicit Vcnl4200Test(QWidget* parent = nullptr);

private slots:
    void refreshNow();
    void pollSensor();
    void toggleAutoBrightness();
    void onBrightnessChanged(int value);
    bool initSensor();

private:
    bool runCmd(const QStringList& args, QString* out = nullptr);
    bool readWord(const QString& bus, const QString& addr, const QString& reg, quint16& value);
    bool writeWord(const QString& bus, const QString& addr, const QString& reg, quint16 value);
    void setStatus(const QString& text, bool error = false);
    void appendLog(const QString& text);
    void applyAutoBrightness(quint16 alsValue);
    int readBacklightMax() const;
    bool writeBacklight(int value);
    void updateDisplayFromGoodValues();
    bool triggerProxMeasurement();

    QLabel* m_title = nullptr;
    QLabel* m_status = nullptr;
    QLabel* m_busLabel = nullptr;
    QLabel* m_addrLabel = nullptr;
    QLabel* m_idValue = nullptr;
    QLabel* m_alsValue = nullptr;
    QLabel* m_psValue = nullptr;
    QLabel* m_whiteValue = nullptr;
    QLabel* m_flagValue = nullptr;
    QLabel* m_brightnessValue = nullptr;
    QPushButton* m_refreshBtn = nullptr;
    QPushButton* m_autoBtn = nullptr;
    QPushButton* m_proxBtn = nullptr;
    QSlider* m_brightnessSlider = nullptr;
    QTimer* m_timer = nullptr;
    QTimer* m_retryTimer = nullptr;
    QString m_bus = "6";
    QString m_addr = "0x51";
    QPlainTextEdit* m_log = nullptr;
    bool m_autoBrightness = false;
    int m_brightnessMax = 255;
    bool m_haveGood = false;
    bool m_devicePresent = false;
    bool m_retryPending = false;
    quint16 m_lastId = 0;
    quint16 m_lastAls = 0;
    quint16 m_lastPs = 0;
    quint16 m_lastWhite = 0;
    quint16 m_lastFlags = 0;
    int m_lastBrightness = -1;
};
