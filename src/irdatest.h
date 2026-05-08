#pragma once

#include <QWidget>

class QLabel;
class QPushButton;
class QPlainTextEdit;
class QSocketNotifier;
class QFile;

class IrdaTest : public QWidget {
    Q_OBJECT
public:
    explicit IrdaTest(QWidget* parent = nullptr);
    ~IrdaTest();

private slots:
    void startRx();
    void stopRx();
    void clearLog();
    void handleReadyRead();
    void setStatus(const QString& text, bool error = false);
    void appendLog(const QString& text);

private:
    QLabel* m_status = nullptr;
    QPushButton* m_startBtn = nullptr;
    QPushButton* m_stopBtn = nullptr;
    QPushButton* m_clearBtn = nullptr;
    QPlainTextEdit* m_log = nullptr;
    QFile* m_uartFile = nullptr;
    QSocketNotifier* m_rxNotifier = nullptr;
};
