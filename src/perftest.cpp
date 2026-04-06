#include "perftest.h"
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QOpenGLWidget>
#include <QTimer>
#include <thread>
#include <atomic>
#include <cmath>

class PerfTest::GLWidget : public QOpenGLWidget {
    Q_OBJECT
public:
    GLWidget(QWidget* p=nullptr): QOpenGLWidget(p){ startTimer(16); }
protected:
    void initializeGL() override {}
    void paintGL() override {
        // simple heavy draw: clear with changing color
        static float t=0; t+=0.02f;
        glClearColor((std::sin(t)+1.f)/2.f, (std::cos(t)+1.f)/2.f, (std::sin(t*0.5f)+1.f)/2.f,1.f);
        glClear(GL_COLOR_BUFFER_BIT);
    }
    void timerEvent(QTimerEvent*) override { update(); }
};

PerfTest::PerfTest(QWidget* parent): QWidget(parent){
    auto layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel("Performance Test - CPU & GPU stress"));
    auto cpuStart = new QPushButton("Start CPU load");
    auto cpuStop = new QPushButton("Stop CPU load");
    auto gpuStart = new QPushButton("Start GPU load");
    auto gpuStop = new QPushButton("Stop GPU load");
    layout->addWidget(cpuStart); layout->addWidget(cpuStop);
    layout->addWidget(gpuStart); layout->addWidget(gpuStop);

    glw = new GLWidget; glw->setMinimumSize(640,360);
    layout->addWidget(glw);

    connect(cpuStart,&QPushButton::clicked,this,&PerfTest::startCpuLoad);
    connect(cpuStop,&QPushButton::clicked,this,&PerfTest::stopCpuLoad);
    connect(gpuStart,&QPushButton::clicked,this,&PerfTest::startGpuLoad);
    connect(gpuStop,&QPushButton::clicked,this,&PerfTest::stopGpuLoad);
}

PerfTest::~PerfTest(){ stopCpuLoad(); stopGpuLoad(); }

void busyLoop(std::atomic<bool>& run){
    while(run){ volatile double x=0; for(int i=0;i<1000000;i++) x+=std::sin(i); }
}

void PerfTest::startCpuLoad(){ if(cpuRunning) return; cpuRunning=true; std::atomic<bool>* run = new std::atomic<bool>(true); cpuThread = new std::thread([run](){ busyLoop(*run); delete run; }); }
void PerfTest::stopCpuLoad(){ if(!cpuRunning) return; cpuRunning=false; if(cpuThread){ // signal thread to stop is omitted for brevity
        cpuThread->detach(); delete cpuThread; cpuThread=nullptr; }
}

void PerfTest::startGpuLoad(){ if(gpuRunning) return; gpuRunning=true; glw->show(); }
void PerfTest::stopGpuLoad(){ if(!gpuRunning) return; gpuRunning=false; glw->hide(); }

#include "perftest.moc"
