#pragma once
#include <QWidget>

class QPushButton;
class QPlainTextEdit;
class QLabel;

class StorageTest : public QWidget {
    Q_OBJECT
public:
    StorageTest(QWidget* parent=nullptr);

private slots:
    void testEmmc();
    void testSd();
    void testUsb();
    void testAuto();

private:
    enum class StorageKind { Emmc, Sd, Usb };

    void appendLog(const QString& line);
    QString findStorageRoot(StorageKind kind) const;
    bool runStorageTest(StorageKind kind, const QString& label);
    void setResult(const QString& key, const QString& value, const QString& style);
    QString kindName(StorageKind kind) const;

    QPushButton* emmcBtn = nullptr;
    QPushButton* sdBtn = nullptr;
    QPushButton* usbBtn = nullptr;
    QPushButton* autoBtn = nullptr;
    QLabel* emmcResult = nullptr;
    QLabel* sdResult = nullptr;
    QLabel* usbResult = nullptr;
    QLabel* autoResult = nullptr;
    QLabel* status = nullptr;
    QPlainTextEdit* log = nullptr;
};
