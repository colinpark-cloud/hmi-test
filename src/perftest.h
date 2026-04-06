#pragma once
#include <QWidget>
#include <thread>

class PerfTest : public QWidget {
    Q_OBJECT
public:
    PerfTest(QWidget* parent=nullptr);
    ~PerfTest();
private slots:
    void startCpuLoad();
    void stopCpuLoad();
    void startGpuLoad();
    void stopGpuLoad();
private:
    bool cpuRunning=false;
    bool gpuRunning=false;
    std::thread* cpuThread=nullptr;
    class GLWidget;
    GLWidget* glw=nullptr;
};
