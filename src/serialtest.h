#pragma once
#include <QWidget>

class QPushButton;
class QPlainTextEdit;
class QLabel;

class SerialTest : public QWidget {
    Q_OBJECT
public:
    SerialTest(QWidget* parent=nullptr);
private slots:
    void openPort();
    void sendTest();
private:
    void appendLog(const QString& line);
    QString currentPort() const;
    bool isRs232() const;
    bool isRs42x() const;
    void updatePortButtons();
    void updateModeButton();

    int fd = -1;
    QPushButton* rs232Btn = nullptr;
    QPushButton* rs42xBtn = nullptr;
    QPushButton* modeBtn = nullptr;
    QPushButton* sendBtn = nullptr;
    QLabel* modeHint = nullptr;
    QPlainTextEdit* log = nullptr;
    bool rs485Mode = false;
    bool portOpen = false;
};
