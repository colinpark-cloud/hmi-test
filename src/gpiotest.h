#pragma once
#include <QWidget>

class GPIOTest : public QWidget {
    Q_OBJECT
public:
    GPIOTest(QWidget* parent=nullptr);
private slots:
    void toggleStatus();
    void toggleBuzzer();
private:
    class Impl; Impl* d;
};
