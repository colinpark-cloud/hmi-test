#include "storagetest.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>
#include <QPlainTextEdit>
#include <QFile>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QStorageInfo>
#include <QTimer>

static QString passStyle(){ return "QPushButton{font-size:16px; font-weight:700; background:#dff3e6; color:#0f3d27; border:1px solid #8bc59e; border-radius:16px; padding:12px 16px;} QPushButton:pressed{padding-top:14px; padding-bottom:10px;}"; }
static QString failStyle(){ return "QPushButton{font-size:16px; font-weight:700; background:#fae0e2; color:#842029; border:1px solid #f1aeb5; border-radius:16px; padding:12px 16px;} QPushButton:pressed{padding-top:14px; padding-bottom:10px;}"; }
static QString neutralStyle(){ return "QPushButton{font-size:16px; font-weight:600; background:#eef3f8; color:#17212f; border:1px solid #c9d3df; border-radius:16px; padding:12px 16px;} QPushButton:pressed{padding-top:14px; padding-bottom:10px;}"; }

StorageTest::StorageTest(QWidget* parent): QWidget(parent){
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(12);

    emmcBtn = new QPushButton("eMMC");
    sdBtn = new QPushButton("SD card");
    usbBtn = new QPushButton("USB");
    autoBtn = new QPushButton("Auto");
    for (auto *b : {emmcBtn, sdBtn, usbBtn, autoBtn}) {
        b->setMinimumHeight(48);
        b->setStyleSheet("QPushButton{font-size:16px; font-weight:700; background:#17304c; color:white; border:1px solid #2d5b89; border-radius:12px; padding:10px 14px;} QPushButton:pressed{padding-top:14px; padding-bottom:10px;}");
    }

    emmcResult = new QLabel("--");
    sdResult = new QLabel("--");
    usbResult = new QLabel("--");
    autoResult = new QLabel("--");
    for (auto *r : {emmcResult, sdResult, usbResult, autoResult}) {
        r->setAlignment(Qt::AlignCenter);
        r->setMinimumHeight(34);
        r->setStyleSheet("font-size:15px; font-weight:700; color:#5f6b7a; background:#ffffff; border:1px solid #cdd6e1; border-radius:10px;");
    }

    auto makeRow = [&](QPushButton* btn, QLabel* res) {
        auto *row = new QWidget;
        row->setStyleSheet("QWidget{background:#f7f9fc; border:1px solid #d8e0ea; border-radius:16px;}");
        auto *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(10, 8, 10, 8);
        rowLayout->setSpacing(10);
        rowLayout->addWidget(btn, 3);
        rowLayout->addWidget(res, 1);
        return row;
    };

    auto *grid = new QWidget;
    auto *gridLayout = new QVBoxLayout(grid);
    gridLayout->setContentsMargins(0, 0, 0, 0);
    gridLayout->setSpacing(8);
    gridLayout->addWidget(makeRow(emmcBtn, emmcResult));
    gridLayout->addWidget(makeRow(sdBtn, sdResult));
    gridLayout->addWidget(makeRow(usbBtn, usbResult));
    gridLayout->addWidget(makeRow(autoBtn, autoResult));

    status = new QLabel("Ready");
    status->setAlignment(Qt::AlignCenter);
    status->setStyleSheet("color:#5f6b7a; font-family:monospace; font-size:12px;");

    log = new QPlainTextEdit;
    log->setReadOnly(true);
    log->setPlaceholderText("Storage results will appear here...");
    log->setStyleSheet("background:#ffffff; color:#1f2937; font-family:monospace; font-size:12px; border:1px solid #cdd6e1; border-radius:12px;");

    layout->addWidget(grid);
    layout->addWidget(status);
    layout->addWidget(log, 1);

    connect(emmcBtn, &QPushButton::clicked, this, &StorageTest::testEmmc);
    connect(sdBtn, &QPushButton::clicked, this, &StorageTest::testSd);
    connect(usbBtn, &QPushButton::clicked, this, &StorageTest::testUsb);
    connect(autoBtn, &QPushButton::clicked, this, &StorageTest::testAuto);
}

void StorageTest::appendLog(const QString& line){
    if (!log) return;
    log->appendPlainText(QDateTime::currentDateTime().toString(Qt::ISODate) + " " + line);
}

QString StorageTest::kindName(StorageKind kind) const {
    switch (kind) {
        case StorageKind::Emmc: return "eMMC";
        case StorageKind::Sd: return "SD";
        case StorageKind::Usb: return "USB";
    }
    return "Unknown";
}

QString StorageTest::findStorageRoot(StorageKind kind) const{
    const auto mounts = QStorageInfo::mountedVolumes();
    for (const auto &m : mounts) {
        if (!m.isValid() || !m.isReady() || m.isReadOnly()) continue;
        const QString path = m.rootPath();
        const QString name = m.device();
        const bool mmc = name.contains("mmcblk2", Qt::CaseInsensitive) || path.contains("mmcblk2", Qt::CaseInsensitive);
        const bool sd = name.contains("mmcblk1", Qt::CaseInsensitive) || path.contains("mmcblk1", Qt::CaseInsensitive);
        const bool usb = name.contains("sda", Qt::CaseInsensitive) || path.contains("sda", Qt::CaseInsensitive);
        const bool mountedHint = path.startsWith("/run/media") || path.startsWith("/media") || path.startsWith("/mnt");
        if (kind == StorageKind::Emmc && mmc && !sd) return path;
        if (kind == StorageKind::Sd && sd) return path;
        if (kind == StorageKind::Usb && usb) return path;
        if (kind == StorageKind::Emmc && mountedHint && mmc) return path;
        if (kind == StorageKind::Sd && mountedHint && sd) return path;
        if (kind == StorageKind::Usb && mountedHint && usb) return path;
    }
    return QString();
}

void StorageTest::setResult(const QString& key, const QString& value, const QString& style){
    QLabel* label = nullptr;
    if (key == "eMMC") label = emmcResult;
    else if (key == "SD") label = sdResult;
    else if (key == "USB") label = usbResult;
    else if (key == "AUTO") label = autoResult;
    if (label) {
        label->setText(value);
        label->setStyleSheet(value == "PASS"
            ? "font-size:15px; font-weight:700; color:#0f3d27; background:#cdebd9; border:1px solid #8bc59e; border-radius:8px;"
            : "font-size:15px; font-weight:700; color:#842029; background:#f8d7da; border:1px solid #f1aeb5; border-radius:8px;");
    }
    Q_UNUSED(style)
}

bool StorageTest::runStorageTest(StorageKind kind, const QString& label){
    QPushButton* btn = nullptr;
    if (label == "eMMC") btn = emmcBtn;
    else if (label == "SD") btn = sdBtn;
    else if (label == "USB") btn = usbBtn;
    if (btn) btn->setDown(true);
    appendLog(QString("searching mounted %1 storage").arg(label));

    const QString root = findStorageRoot(kind);
    if (root.isEmpty()) {
        appendLog(QString("no %1 storage found").arg(label));
        setResult(label, "FAIL", failStyle());
        if (btn) QTimer::singleShot(180, btn, [btn]() { btn->setDown(false); });
        return false;
    }

    const QString path = QDir(root).filePath(QString("hmi_%1_test.bin").arg(label.toLower()));
    appendLog(QString("target path: %1").arg(path));
    QFile f(path);
    if(!f.open(QIODevice::WriteOnly)){
        appendLog("write open failed");
        setResult(label, "FAIL", failStyle());
        if (btn) QTimer::singleShot(180, btn, [btn]() { btn->setDown(false); });
        return false;
    }
    QByteArray data(256 * 1024, '\xAA');
    f.write(data); f.close();
    appendLog("write complete");

    QFile r(path);
    if(!r.open(QIODevice::ReadOnly)){
        appendLog("read open failed");
        setResult(label, "FAIL", failStyle());
        if (btn) QTimer::singleShot(180, btn, [btn]() { btn->setDown(false); });
        return false;
    }
    auto read = r.readAll(); r.close();
    auto wHash = QCryptographicHash::hash(data,QCryptographicHash::Sha256).toHex();
    auto rHash = QCryptographicHash::hash(read,QCryptographicHash::Sha256).toHex();
    appendLog(QString("write hash: %1").arg(QString(wHash)));
    appendLog(QString("read  hash: %1").arg(QString(rHash)));
    const bool ok = (wHash == rHash);
    setResult(label, ok ? "PASS" : "FAIL", ok ? passStyle() : failStyle());
    if (btn) QTimer::singleShot(180, btn, [btn]() { btn->setDown(false); });
    return ok;
}

void StorageTest::testEmmc(){
    runStorageTest(StorageKind::Emmc, "eMMC");
}

void StorageTest::testSd(){
    runStorageTest(StorageKind::Sd, "SD");
}

void StorageTest::testUsb(){
    runStorageTest(StorageKind::Usb, "USB");
}

void StorageTest::testAuto(){
    if (autoBtn) autoBtn->setDown(true);
    appendLog("AUTO start");
    const bool e = runStorageTest(StorageKind::Emmc, "eMMC");
    const bool s = runStorageTest(StorageKind::Sd, "SD");
    const bool u = runStorageTest(StorageKind::Usb, "USB");
    const bool ok = e && s && u;
    appendLog(QString("AUTO result: eMMC=%1 SD=%2 USB=%3").arg(e ? "PASS" : "FAIL", s ? "PASS" : "FAIL", u ? "PASS" : "FAIL"));
    setResult("AUTO", ok ? "PASS" : "FAIL", ok ? passStyle() : failStyle());
    if (autoBtn) QTimer::singleShot(180, autoBtn, [autoBtn = autoBtn]() { autoBtn->setDown(false); });
}
