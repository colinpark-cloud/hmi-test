#pragma once
#include <QWidget>

class QPushButton;
class QPlainTextEdit;
class QLabel;
class QComboBox;
class QTimer;
class QSocketNotifier;

class SerialTest : public QWidget {
    Q_OBJECT
public:
    SerialTest(QWidget* parent=nullptr);
    void setActive(bool active);
private slots:
    void openPort();
    void sendOnce();
    void toggleAutoSend();
    void onReadReady();
private:
    enum class Port    { None, COM1, COM2, COM3 };
    enum class Mode    { RS232, RS42x };
    enum class TestDir { TX, RX };

    void appendTx(const QString& line);
    void appendRx(const QString& line);
    QString portDevice() const;
    void applyGpio();
    void updateUI();
    void selectPort(Port p);
    void selectMode(Mode m);
    void selectDir(TestDir d);

    int fd = -1;
    bool portOpen    = false;
    bool autoSending = false;
    bool hwFlow      = false;
    Port    curPort = Port::None;
    Mode    curMode = Mode::RS232;
    TestDir curDir  = TestDir::TX;

    QSocketNotifier *notifier     = nullptr;
    QTimer          *autoTimer    = nullptr;
    QTimer          *rxFlushTimer = nullptr;
    QTimer          *modemTimer   = nullptr;
    QComboBox       *baudBox      = nullptr;
    QByteArray       rxBuf;

    QPushButton *com1Btn     = nullptr;
    QPushButton *com2Btn     = nullptr;
    QPushButton *com3Btn     = nullptr;
    QPushButton *rs232Btn    = nullptr;
    QPushButton *rs42xBtn    = nullptr;
    QPushButton *txDirBtn    = nullptr;
    QPushButton *rxDirBtn    = nullptr;
    QPushButton *openBtn     = nullptr;
    QPushButton *sendBtn     = nullptr;
    QPushButton *autoBtn     = nullptr;
    QPushButton *hwFlowBtn   = nullptr;
    QLabel      *statusLabel = nullptr;
    QLabel      *modemLabel  = nullptr;
    QPlainTextEdit *txLog = nullptr;
    QPlainTextEdit *rxLog = nullptr;
};
