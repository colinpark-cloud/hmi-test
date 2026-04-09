#pragma once
#include <QWidget>
#include <QPushButton>
#include <QLabel>

class GPIOTest : public QWidget {
    Q_OBJECT
public:
    GPIOTest(QWidget* parent=nullptr);
private slots:
    void toggleBuzzer();
    void updateButtonState();
private:
    class Impl; Impl* d;
    QPushButton* buzzerBtn = nullptr;
    QLabel* statusLabel = nullptr;
    bool buzzerOn = false;
};
