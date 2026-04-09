#include "perftest.h"

#include <QByteArray>
#include <QFile>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QPushButton>
#include <QThread>
#include <QTimer>
#include <QVBoxLayout>
#include <cmath>
#include <cstring>

static QString onStyle() {
    return "font-size:16px; font-weight:700; background:#1f7a5a; color:white; border:1px solid #166648; border-radius:12px; padding:10px 14px;";
}

static QString offStyle() {
    return "font-size:16px; font-weight:700; background:#17304c; color:white; border:1px solid #2d5b89; border-radius:12px; padding:10px 14px;";
}

static double readMemTotalKb() {
    QFile f("/proc/meminfo");
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return 0.0;
    while (!f.atEnd()) {
        const QByteArray line = f.readLine();
        if (line.startsWith("MemTotal:")) {
            const QList<QByteArray> parts = line.simplified().split(' ');
            if (parts.size() >= 2) return parts[1].toDouble();
        }
    }
    return 0.0;
}

static double readMemAvailableKb() {
    QFile f("/proc/meminfo");
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return 0.0;
    while (!f.atEnd()) {
        const QByteArray line = f.readLine();
        if (line.startsWith("MemAvailable:")) {
            const QList<QByteArray> parts = line.simplified().split(' ');
            if (parts.size() >= 2) return parts[1].toDouble();
        }
    }
    return 0.0;
}

static double readCpuUsage() {
    static quint64 prevTotal = 0;
    static quint64 prevIdle = 0;
    QFile f("/proc/stat");
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return 0.0;
    const QByteArray line = f.readLine();
    const QList<QByteArray> parts = line.simplified().split(' ');
    if (parts.size() < 5) return 0.0;
    quint64 user = parts.value(1).toULongLong();
    quint64 nice = parts.value(2).toULongLong();
    quint64 system = parts.value(3).toULongLong();
    quint64 idle = parts.value(4).toULongLong();
    quint64 iowait = parts.size() > 5 ? parts.value(5).toULongLong() : 0;
    quint64 irq = parts.size() > 6 ? parts.value(6).toULongLong() : 0;
    quint64 softirq = parts.size() > 7 ? parts.value(7).toULongLong() : 0;
    quint64 steal = parts.size() > 8 ? parts.value(8).toULongLong() : 0;
    quint64 total = user + nice + system + idle + iowait + irq + softirq + steal;
    if (prevTotal == 0) {
        prevTotal = total;
        prevIdle = idle + iowait;
        return 0.0;
    }
    quint64 totald = total - prevTotal;
    quint64 idled = (idle + iowait) - prevIdle;
    prevTotal = total;
    prevIdle = idle + iowait;
    if (totald == 0) return 0.0;
    return qBound(0.0, 1.0 - (double)idled / (double)totald, 1.0);
}

static double readRamUsageRatio() {
    return 0.0;
}

class UsageGraphWidget : public QWidget {
public:
    explicit UsageGraphWidget(const QString& title, QColor line, QWidget* parent = nullptr)
        : QWidget(parent), m_title(title), m_line(line) {
        setMinimumHeight(170);
    }
    void setHistory(const QVector<double>& history, double current, const QString& suffix) {
        m_history = history;
        m_current = current;
        m_suffix = suffix;
        update();
    }
protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.fillRect(rect(), QColor("#ffffff"));
        p.setPen(QPen(QColor("#d8e0ea"), 1));
        p.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 14, 14);

        QRectF inner = rect().adjusted(16, 28, -16, -16);
        p.setPen(QPen(QColor("#eef2f7"), 1));
        for (int i = 1; i < 4; ++i) {
            const qreal y = inner.top() + inner.height() * i / 4.0;
            p.drawLine(QPointF(inner.left(), y), QPointF(inner.right(), y));
        }
        for (int i = 1; i < 7; ++i) {
            const qreal x = inner.left() + inner.width() * i / 7.0;
            p.drawLine(QPointF(x, inner.top()), QPointF(x, inner.bottom()));
        }

        p.setPen(QColor("#17212f"));
        QFont f = p.font();
        f.setBold(true);
        f.setPointSize(14);
        p.setFont(f);
        p.drawText(QRectF(16, 6, width() - 32, 20), Qt::AlignLeft | Qt::AlignVCenter, m_title);
        p.setPen(QColor("#5f6b7a"));
        f.setBold(false);
        f.setPointSize(10);
        p.setFont(f);
        p.drawText(QRectF(16, 6, width() - 32, 20), Qt::AlignRight | Qt::AlignVCenter, QString::number((int)std::round(m_current * 100.0)) + "%");

        if (m_history.size() < 2) return;
        QPainterPath path;
        const int count = m_history.size();
        for (int i = 0; i < count; ++i) {
            const double v = qBound(0.0, m_history[i], 1.0);
            const qreal x = inner.left() + inner.width() * i / double(count - 1);
            const qreal y = inner.bottom() - inner.height() * v;
            if (i == 0) path.moveTo(x, y); else path.lineTo(x, y);
        }
        QPainterPath fill = path;
        fill.lineTo(inner.right(), inner.bottom());
        fill.lineTo(inner.left(), inner.bottom());
        fill.closeSubpath();
        QColor fillColor = m_line;
        fillColor.setAlpha(42);
        p.fillPath(fill, fillColor);
        p.setPen(QPen(m_line, 3.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.drawPath(path);

        p.setPen(QColor("#5f6b7a"));
        f.setPointSize(10);
        p.setFont(f);
        p.drawText(QRectF(16, height() - 18, width() - 32, 14), Qt::AlignRight | Qt::AlignVCenter, m_suffix);
    }
private:
    QString m_title;
    QColor m_line;
    QVector<double> m_history;
    double m_current = 0.0;
    QString m_suffix;
};

PerfTest::PerfTest(QWidget* parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(12);

    auto *title = new QLabel("Stress");
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size:22px; font-weight:700; color:#17212f;");
    auto *desc = new QLabel("Task Manager style graphs for CPU and memory load.");
    desc->setAlignment(Qt::AlignCenter);
    desc->setStyleSheet("color:#5f6b7a; font-size:13px;");
    desc->setWordWrap(true);

    cpuLoadBtn = new QPushButton("CPU Load");
    cpuBtn1 = new QPushButton("thread 1");
    cpuBtn2 = new QPushButton("thread 2");
    cpuBtn3 = new QPushButton("thread 3");
    ramLoadBtn = new QPushButton("RAM Load");
    ramBtn256 = new QPushButton("256 MB");
    ramBtn512 = new QPushButton("512 MB");
    ramBtn1024 = new QPushButton("1 GB");
    for (auto *b : {cpuLoadBtn, cpuBtn1, cpuBtn2, cpuBtn3, ramLoadBtn, ramBtn256, ramBtn512}) b->setStyleSheet(offStyle());
    ramBtn1024->setStyleSheet("font-size:16px; font-weight:700; background:#d97706; color:white; border:1px solid #f59e0b; border-radius:12px; padding:10px 14px;");

    cpuTargetThreads = 3;
    cpuDetail = new QLabel("3 worker threads");
    ramDetail = new QLabel("0 MB allocated");
    cpuCurrent = new QLabel("0%");
    ramCurrent = new QLabel("0%");
    for (auto *l : {cpuDetail, ramDetail, cpuCurrent, ramCurrent}) {
        l->setAlignment(Qt::AlignCenter);
        l->setStyleSheet("color:#5f6b7a; font-size:12px;");
    }
    cpuCurrent->setStyleSheet("font-size:26px; font-weight:800; color:#17304c;");
    ramCurrent->setStyleSheet("font-size:26px; font-weight:800; color:#17304c;");

    cpuGraph = new UsageGraphWidget("CPU", QColor("#00a3ff"));
    ramGraph = new UsageGraphWidget("Memory", QColor("#1f7a5a"));

    auto *cpuGraphCard = new QWidget;
    cpuGraphCard->setStyleSheet("QWidget{background:#f7f9fc; border:1px solid #d8e0ea; border-radius:16px;}");
    auto *cpuGraphLayout = new QVBoxLayout(cpuGraphCard);
    cpuGraphLayout->setContentsMargins(12, 12, 12, 12);
    cpuGraphLayout->addWidget(cpuGraph);

    auto *cpuControlCard = new QWidget;
    cpuControlCard->setStyleSheet("QWidget{background:#ffffff; border:1px solid #d8e0ea; border-radius:16px;}");
    auto *cpuControlLayout = new QHBoxLayout(cpuControlCard);
    cpuControlLayout->setContentsMargins(12, 10, 12, 10);
    cpuControlLayout->setSpacing(8);
    cpuControlLayout->addWidget(cpuLoadBtn);
    cpuControlLayout->addWidget(cpuBtn1);
    cpuControlLayout->addWidget(cpuBtn2);
    cpuControlLayout->addWidget(cpuBtn3);
    cpuControlLayout->addWidget(cpuCurrent);
    cpuControlLayout->addWidget(cpuDetail, 1);

    auto *ramGraphCard = new QWidget;
    ramGraphCard->setStyleSheet("QWidget{background:#f7f9fc; border:1px solid #d8e0ea; border-radius:16px;}");
    auto *ramGraphLayout = new QVBoxLayout(ramGraphCard);
    ramGraphLayout->setContentsMargins(12, 12, 12, 12);
    ramGraphLayout->addWidget(ramGraph);

    auto *ramControlCard = new QWidget;
    ramControlCard->setStyleSheet("QWidget{background:#ffffff; border:1px solid #d8e0ea; border-radius:16px;}");
    auto *ramControlLayout = new QHBoxLayout(ramControlCard);
    ramControlLayout->setContentsMargins(12, 10, 12, 10);
    ramControlLayout->setSpacing(8);
    ramControlLayout->addWidget(ramLoadBtn);
    ramControlLayout->addWidget(ramBtn256);
    ramControlLayout->addWidget(ramBtn512);
    ramControlLayout->addWidget(ramBtn1024);
    ramControlLayout->addWidget(ramCurrent);
    ramControlLayout->addWidget(ramDetail, 1);

    layout->addStretch(1);
    layout->addWidget(title);
    layout->addWidget(desc);
    layout->addSpacing(8);
    layout->addWidget(cpuGraphCard);
    layout->addWidget(cpuControlCard);
    layout->addWidget(ramGraphCard);
    layout->addWidget(ramControlCard);
    layout->addStretch(2);

    usageTimer = new QTimer(this);
    usageTimer->setInterval(500);
    connect(usageTimer, &QTimer::timeout, this, &PerfTest::updateUsage);
    usageTimer->start();
    updateUsage();

    connect(cpuLoadBtn, &QPushButton::clicked, this, [this]() {
        if (cpuRunning) stopCpuLoad();
        else startCpuLoad();
    });
    connect(cpuBtn1, &QPushButton::clicked, this, [this]() { cpuTargetThreads = 1; if (cpuRunning) stopCpuLoad(); startCpuLoad(); });
    connect(cpuBtn2, &QPushButton::clicked, this, [this]() { cpuTargetThreads = 2; if (cpuRunning) stopCpuLoad(); startCpuLoad(); });
    connect(cpuBtn3, &QPushButton::clicked, this, [this]() { cpuTargetThreads = 3; if (cpuRunning) stopCpuLoad(); startCpuLoad(); });
    connect(ramLoadBtn, &QPushButton::clicked, this, [this]() {
        if (ramRunning) stopRamLoad();
        else startRamLoad();
    });
    connect(ramBtn256, &QPushButton::clicked, this, [this]() { ramTargetBytes = 256ull * 1024ull * 1024ull; if (ramRunning) stopRamLoad(); startRamLoad(); });
    connect(ramBtn512, &QPushButton::clicked, this, [this]() { ramTargetBytes = 512ull * 1024ull * 1024ull; if (ramRunning) stopRamLoad(); startRamLoad(); });
    connect(ramBtn1024, &QPushButton::clicked, this, [this]() { ramTargetBytes = 1024ull * 1024ull * 1024ull; if (ramRunning) stopRamLoad(); startRamLoad(); });
}

PerfTest::~PerfTest() {
    stopCpuLoad();
    stopRamLoad();
}

void PerfTest::startCpuLoad() {
    if (cpuRunning) return;
    cpuRunning = true;
    cpuStop = false;
    const int threads = qBound(1, cpuTargetThreads, 3);
    cpuThreadCount = threads;
    cpuThreads.clear();
    cpuThreads.reserve(threads);
    for (int i = 0; i < threads; ++i) {
        cpuThreads.emplace_back([this]() {
            volatile double x = 0.0;
            while (!cpuStop.load(std::memory_order_relaxed)) {
                for (int i = 0; i < 50000; ++i) x += std::sin((double)i);
            }
            (void)x;
        });
    }
    refreshButtons();
}

void PerfTest::stopCpuLoad() {
    if (!cpuRunning) return;
    cpuStop = true;
    for (auto &t : cpuThreads) {
        if (t.joinable()) t.join();
    }
    cpuThreads.clear();
    cpuRunning = false;
    cpuThreadCount = 0;
    refreshButtons();
}

void PerfTest::startRamLoad() {
    if (ramRunning) return;
    ramRunning = true;
    ramBlocks.clear();
    size_t allocated = 0;
    while (allocated < ramTargetBytes) {
        auto block = std::make_unique<QByteArray>();
        block->resize((int)ramChunkBytes);
        memset(block->data(), 0xA5, (size_t)block->size());
        allocated += (size_t)block->size();
        ramBlocks.push_back(std::move(block));
    }
    ramAllocatedMb = (int)(allocated / (1024ull * 1024ull));
    refreshButtons();
}

void PerfTest::stopRamLoad() {
    if (!ramRunning) return;
    ramBlocks.clear();
    ramRunning = false;
    ramAllocatedMb = 0;
    refreshButtons();
}

void PerfTest::stopAllLoads() {
    stopCpuLoad();
    stopRamLoad();
}

void PerfTest::refreshButtons() {
    if (cpuLoadBtn) cpuLoadBtn->setText(cpuRunning ? "CPU Load: ON" : "CPU Load");
    if (cpuLoadBtn) cpuLoadBtn->setStyleSheet(cpuRunning ? onStyle() : offStyle());
    if (cpuBtn1) cpuBtn1->setStyleSheet(cpuRunning && cpuTargetThreads == 1 ? onStyle() : offStyle());
    if (cpuBtn2) cpuBtn2->setStyleSheet(cpuRunning && cpuTargetThreads == 2 ? onStyle() : offStyle());
    if (cpuBtn3) cpuBtn3->setStyleSheet(cpuRunning && cpuTargetThreads == 3 ? onStyle() : offStyle());
    if (ramLoadBtn) ramLoadBtn->setText(ramRunning ? "RAM Load: ON" : "RAM Load");
    if (ramLoadBtn) ramLoadBtn->setStyleSheet(ramRunning ? onStyle() : offStyle());
    if (ramBtn256) ramBtn256->setStyleSheet(ramRunning && ramTargetBytes == 256ull * 1024ull * 1024ull ? onStyle() : offStyle());
    if (ramBtn512) ramBtn512->setStyleSheet(ramRunning && ramTargetBytes == 512ull * 1024ull * 1024ull ? onStyle() : offStyle());
    if (ramBtn1024) ramBtn1024->setStyleSheet(ramRunning && ramTargetBytes == 1024ull * 1024ull * 1024ull ? onStyle() : "font-size:16px; font-weight:700; background:#d97706; color:white; border:1px solid #f59e0b; border-radius:12px; padding:10px 14px;");
    if (cpuDetail) cpuDetail->setText(cpuRunning ? QString::number(cpuThreadCount) + " worker threads" : "0 worker threads");
    if (ramDetail) ramDetail->setText(ramRunning ? QString::number(ramAllocatedMb) + " MB allocated" : "0 MB allocated");
}

void PerfTest::updateUsage() {
    const double cpuUsage = ::readCpuUsage();
    const double ramUsage = ramRunning ? qBound(0.0, double(ramAllocatedMb) / 2048.0, 1.0) : 0.0;
    cpuHistory.push_back(cpuUsage);
    ramHistory.push_back(ramUsage);
    while (cpuHistory.size() > historyLimit) cpuHistory.pop_front();
    while (ramHistory.size() > historyLimit) ramHistory.pop_front();
    if (cpuGraph) cpuGraph->setHistory(cpuHistory, cpuUsage, cpuRunning ? QString("%1 worker threads").arg(cpuThreadCount) : QString("0 worker threads"));
    if (ramGraph) ramGraph->setHistory(ramHistory, ramUsage, ramRunning ? QString("%1 / 2048 MB").arg(ramAllocatedMb) : QString("0 / 2048 MB"));
    if (cpuCurrent) cpuCurrent->setText(QString::number((int)std::round(cpuUsage * 100.0)) + "%");
    if (ramCurrent) ramCurrent->setText(QString::number((int)std::round(ramUsage * 100.0)) + "%");
}
