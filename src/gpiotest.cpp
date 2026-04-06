#include "gpiotest.h"
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <fstream>
#include <string>
#include <sstream>

// Fallback GPIO control via sysfs if libgpiod not available on target
struct GPIOTest::Impl {
    std::string gpio_base = "/sys/class/gpio";
    int status_gpio = (3*32)+16; // gpio3_16 -> gpio number math may vary; adjust if needed
    int buzzer_gpio = (3*32)+20;
};

static bool writeFile(const std::string &path, const std::string &val){
    std::ofstream f(path);
    if(!f.is_open()) return false;
    f<<val;
    return true;
}

GPIOTest::GPIOTest(QWidget* parent): QWidget(parent), d(new Impl){
    auto layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel("GPIO & Buzzer Test"));
    auto btn1 = new QPushButton("Toggle STATUS LED");
    auto btn2 = new QPushButton("Toggle BUZZER");
    layout->addWidget(btn1); layout->addWidget(btn2);

    // export GPIOs if needed
    writeFile(d->gpio_base+"/export", std::to_string(d->status_gpio));
    writeFile(d->gpio_base+"/export", std::to_string(d->buzzer_gpio));
    writeFile(d->gpio_base+"/gpio"+std::to_string(d->status_gpio)+"/direction","out");
    writeFile(d->gpio_base+"/gpio"+std::to_string(d->buzzer_gpio)+"/direction","out");

    connect(btn1, &QPushButton::clicked, this, &GPIOTest::toggleStatus);
    connect(btn2, &QPushButton::clicked, this, &GPIOTest::toggleBuzzer);
}

void GPIOTest::toggleStatus(){
    std::string valpath = d->gpio_base+"/gpio"+std::to_string(d->status_gpio)+"/value";
    std::ifstream f(valpath);
    int v=0; if(f.is_open()) f>>v; f.close();
    writeFile(valpath, v?"0":"1");
}
void GPIOTest::toggleBuzzer(){
    std::string valpath = d->gpio_base+"/gpio"+std::to_string(d->buzzer_gpio)+"/value";
    std::ifstream f(valpath);
    int v=0; if(f.is_open()) f>>v; f.close();
    writeFile(valpath, v?"0":"1");
}
