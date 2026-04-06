#include "serialtest.h"
#include <QVBoxLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>

SerialTest::SerialTest(QWidget* parent): QWidget(parent){
    auto layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel("Serial Test (RS232/RS485)"));
    pathEdit = new QLineEdit("/dev/ttymxc2");
    layout->addWidget(pathEdit);
    auto openBtn = new QPushButton("Open Port");
    auto sendBtn = new QPushButton("Send 'test' ");
    layout->addWidget(openBtn);
    layout->addWidget(sendBtn);

    connect(openBtn,&QPushButton::clicked,this,&SerialTest::openPort);
    connect(sendBtn,&QPushButton::clicked,this,&SerialTest::sendTest);
}

void SerialTest::openPort(){
    if(fd!=-1){ ::close(fd); fd=-1; }
    fd = open(pathEdit->text().toStdString().c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if(fd<0){ perror("open"); return; }
    struct termios tio; memset(&tio,0,sizeof(tio));
    cfsetispeed(&tio,B115200); cfsetospeed(&tio,B115200);
    tio.c_cflag = CS8 | CLOCAL | CREAD;
    tio.c_iflag = IGNPAR;
    tio.c_oflag = 0; tio.c_lflag = 0;
    tcflush(fd, TCIFLUSH);
    tcsetattr(fd,TCSANOW,&tio);
}

void SerialTest::sendTest(){ if(fd!=-1){ write(fd,"test\n",5); } }
