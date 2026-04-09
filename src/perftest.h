#pragma once

#include <QByteArray>
#include <QWidget>
#include <atomic>
#include <memory>
#include <thread>
#include <vector>

class QLabel;
class QPushButton;
class QTimer;
class UsageGraphWidget;

class PerfTest : public QWidget {
    Q_OBJECT
public:
    explicit PerfTest(QWidget* parent = nullptr);
    ~PerfTest();
    void stopAllLoads();

private:
    void startCpuLoad();
    void stopCpuLoad();
    void startRamLoad();
    void stopRamLoad();
    void updateUsage();
    void refreshButtons();

    QPushButton* cpuLoadBtn = nullptr;
    QPushButton* cpuBtn1 = nullptr;
    QPushButton* cpuBtn2 = nullptr;
    QPushButton* cpuBtn3 = nullptr;
    QPushButton* cpuBtn4 = nullptr;
    QPushButton* ramLoadBtn = nullptr;
    QPushButton* ramBtn256 = nullptr;
    QPushButton* ramBtn512 = nullptr;
    QPushButton* ramBtn1024 = nullptr;
    QPushButton* ramBtn1250 = nullptr;
    QLabel* cpuTemp = nullptr;
    QLabel* cpuDetail = nullptr;
    QLabel* ramDetail = nullptr;
    QLabel* cpuCurrent = nullptr;
    QLabel* ramCurrent = nullptr;
    QTimer* usageTimer = nullptr;
    UsageGraphWidget* cpuGraph = nullptr;
    UsageGraphWidget* ramGraph = nullptr;

    bool cpuRunning = false;
    bool ramRunning = false;
    std::vector<std::thread> cpuThreads;
    std::atomic<bool> cpuStop{false};
    std::vector<std::unique_ptr<QByteArray>> ramBlocks;
    size_t ramTargetBytes = 1024ull * 1024ull * 1024ull;
    size_t ramChunkBytes = 4ull * 1024ull * 1024ull;
    int cpuThreadCount = 0;
    int cpuTargetThreads = 1;
    int ramAllocatedMb = 0;
    QVector<double> cpuHistory;
    QVector<double> ramHistory;
    int historyLimit = 120;
};
