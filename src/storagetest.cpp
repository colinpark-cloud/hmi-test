#include "storagetest.h"
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QFile>
#include <QCryptographicHash>
#include <QMessageBox>

StorageTest::StorageTest(QWidget* parent): QWidget(parent){
    auto layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel("Storage Test (eMMC/SD)"));
    auto btn = new QPushButton("Write & Verify /tmp/hmi_test.bin");
    layout->addWidget(btn);
    connect(btn,&QPushButton::clicked,this,&StorageTest::doWriteRead);
}

void StorageTest::doWriteRead(){
    const QString path = "/tmp/hmi_test.bin";
    QFile f(path);
    if(!f.open(QIODevice::WriteOnly)){ return; }
    QByteArray data(1024*1024, '\xAA'); // 1MB
    f.write(data); f.close();
    QFile r(path);
    if(!r.open(QIODevice::ReadOnly)) return;
    auto read = r.readAll(); r.close();
    auto wHash = QCryptographicHash::hash(data,QCryptographicHash::Sha256).toHex();
    auto rHash = QCryptographicHash::hash(read,QCryptographicHash::Sha256).toHex();
    QMessageBox::information(this,"Storage Test", QString("W:%1\nR:%2").arg(QString(wHash),QString(rHash)));
}
