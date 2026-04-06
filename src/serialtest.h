#pragma once
#include <QWidget>

class QLineEdit;

class SerialTest : public QWidget {
    Q_OBJECT
public:
    SerialTest(QWidget* parent=nullptr);
private slots:
    void openPort();
    void sendTest();
private:
    int fd = -1;
    QLineEdit* pathEdit = nullptr;
};
