#pragma once
#include <QWidget>

class StorageTest : public QWidget {
    Q_OBJECT
public:
    StorageTest(QWidget* parent=nullptr);
private slots:
    void doWriteRead();
};
