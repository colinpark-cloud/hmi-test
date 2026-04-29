#pragma once

#include <QWidget>

class QLabel;
class QPushButton;
class QPlainTextEdit;

class ProxTest : public QWidget {
    Q_OBJECT
public:
    explicit ProxTest(QWidget* parent = nullptr);

private slots:
    void pollProx();
    void setStatus(const QString& text, bool error = false);
    void appendLog(const QString& text);
    void executeI2CCommands();
    void startI2CPolling();

private:
    bool runCmd(const QStringList& args, QString* out = nullptr);
    bool readWord(const QString& bus, const QString& addr, const QString& reg, quint16& value);
    bool writeWord(const QString& bus, const QString& addr, const QString& reg, quint16 value);
    bool initProxSensor();

    QLabel* m_status = nullptr;
    QLabel* m_value = nullptr;
    QLabel* m_flagValue = nullptr;
    QLabel* m_lightValue = nullptr;
    QLabel* m_busLabel = nullptr;
    QLabel* m_addrLabel = nullptr;
    QPlainTextEdit* m_log = nullptr;
    QString m_bus = "6";
    QString m_addr = "0x51";
    quint16 m_lastPs = 0;
    quint16 m_lastAls = 0;
    quint16 m_lastFlags = 0;
    bool m_initialized = false;
};
