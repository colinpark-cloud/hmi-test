#include "mramtest.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QPlainTextEdit>
#include <QDateTime>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <mtd/mtd-user.h>

static const char* MTD_DEV = "/dev/mtd0";
static const int   TEST_SIZE = 4096;          // one erase block is typically 4K for MRAM
static const quint32 TEST_OFFSET = 0;

MramTest::MramTest(QWidget* parent): QWidget(parent){
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(14);

    auto *title = new QLabel("MRAM Test");
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size:22px; font-weight:700; color:#17212f;");

    auto *desc = new QLabel("EM008LXQBDH13IS2T — 1 MB QSPI MRAM (/dev/mtd0)\nErase → Write → Read → Verify");
    desc->setAlignment(Qt::AlignCenter);
    desc->setStyleSheet("color:#5f6b7a; font-size:13px;");
    desc->setWordWrap(true);

    testBtn = new QPushButton("Run MRAM Test");
    testBtn->setMinimumHeight(54);
    testBtn->setFixedWidth(240);
    testBtn->setStyleSheet("font-size:18px; font-weight:700; background:#17304c; color:white; border:1px solid #2d5b89; border-radius:12px; padding:10px 14px;");

    resultLabel = new QLabel("--");
    resultLabel->setAlignment(Qt::AlignCenter);
    resultLabel->setMinimumHeight(40);
    resultLabel->setStyleSheet("font-size:16px; font-weight:700; color:#5f6b7a; background:#ffffff; border:1px solid #cdd6e1; border-radius:10px;");

    log = new QPlainTextEdit;
    log->setReadOnly(true);
    log->setPlaceholderText("Test results will appear here...");
    log->setStyleSheet("background:#ffffff; color:#1f2937; font-family:monospace; font-size:12px; border:1px solid #cdd6e1; border-radius:12px;");

    auto *btnRow = new QWidget;
    auto *btnRowLayout = new QHBoxLayout(btnRow);
    btnRowLayout->setContentsMargins(0,0,0,0);
    btnRowLayout->addStretch(1);
    btnRowLayout->addWidget(testBtn);
    btnRowLayout->addStretch(1);

    layout->addStretch(1);
    layout->addWidget(title);
    layout->addWidget(desc);
    layout->addSpacing(8);
    layout->addWidget(btnRow);
    layout->addWidget(resultLabel);
    layout->addWidget(log, 2);
    layout->addStretch(1);

    connect(testBtn, &QPushButton::clicked, this, &MramTest::runTest);
}

void MramTest::appendLog(const QString& line){
    if (!log) return;
    log->appendPlainText(QDateTime::currentDateTime().toString("hh:mm:ss.zzz") + " " + line);
}

void MramTest::setResult(bool pass, const QString& detail){
    if (!resultLabel) return;
    const QString text = pass ? "PASS" : ("FAIL" + (detail.isEmpty() ? "" : ": " + detail));
    resultLabel->setText(text);
    resultLabel->setStyleSheet(pass
        ? "font-size:16px; font-weight:700; color:#0f3d27; background:#cdebd9; border:1px solid #8bc59e; border-radius:10px;"
        : "font-size:16px; font-weight:700; color:#842029; background:#f8d7da; border:1px solid #f1aeb5; border-radius:10px;");
}

void MramTest::runTest(){
    log->clear();
    resultLabel->setText("Testing...");
    resultLabel->setStyleSheet("font-size:16px; font-weight:700; color:#5f6b7a; background:#ffffff; border:1px solid #cdd6e1; border-radius:10px;");
    testBtn->setEnabled(false);
    QCoreApplication::processEvents();

    // Check device exists
    if (!QFileInfo::exists(MTD_DEV)) {
        appendLog(QString("ERROR: %1 not found").arg(MTD_DEV));
        appendLog("MRAM driver not loaded — JEDEC ID 6b bb 14 10 49 5d not registered.");
        appendLog("BSP kernel patch required: add EM008LXQBDH13IS2T to spi-nor driver.");
        setResult(false, "device not found");
        testBtn->setEnabled(true);
        return;
    }

    // Open MTD device
    int fd = ::open(MTD_DEV, O_RDWR);
    if (fd < 0) {
        appendLog(QString("ERROR: open %1 failed: %2").arg(MTD_DEV).arg(strerror(errno)));
        setResult(false, "open failed");
        testBtn->setEnabled(true);
        return;
    }

    // Get MTD info
    struct mtd_info_user mtd{};
    if (::ioctl(fd, MEMGETINFO, &mtd) < 0) {
        appendLog(QString("ERROR: MEMGETINFO failed: %1").arg(strerror(errno)));
        ::close(fd);
        setResult(false, "ioctl failed");
        testBtn->setEnabled(true);
        return;
    }
    appendLog(QString("MTD size: %1 bytes, erase size: %2 bytes, type: %3")
              .arg(mtd.size).arg(mtd.erasesize).arg(mtd.type));

    const int testSize = qMin((int)mtd.erasesize, TEST_SIZE);

    // Erase
    struct erase_info_user ei{};
    ei.start  = TEST_OFFSET;
    ei.length = mtd.erasesize;
    appendLog(QString("Erasing %1 bytes at offset 0x%2...").arg(ei.length).arg(ei.start, 0, 16));
    if (::ioctl(fd, MEMERASE, &ei) < 0) {
        appendLog(QString("WARNING: erase failed (%1) — MRAM may not need erase, continuing").arg(strerror(errno)));
    } else {
        appendLog("Erase OK");
    }

    // Build test pattern
    QByteArray writeData(testSize, 0);
    for (int i = 0; i < testSize; ++i)
        writeData[i] = static_cast<char>(i & 0xFF);

    // Write
    appendLog(QString("Writing %1 bytes...").arg(testSize));
    if (::lseek(fd, TEST_OFFSET, SEEK_SET) < 0) {
        appendLog(QString("ERROR: lseek failed: %1").arg(strerror(errno)));
        ::close(fd);
        setResult(false, "lseek failed");
        testBtn->setEnabled(true);
        return;
    }
    ssize_t written = ::write(fd, writeData.constData(), testSize);
    if (written != testSize) {
        appendLog(QString("ERROR: write returned %1 (expected %2): %3").arg(written).arg(testSize).arg(strerror(errno)));
        ::close(fd);
        setResult(false, "write failed");
        testBtn->setEnabled(true);
        return;
    }
    appendLog(QString("Write OK (%1 bytes)").arg(written));

    // Read back
    appendLog("Reading back...");
    if (::lseek(fd, TEST_OFFSET, SEEK_SET) < 0) {
        appendLog(QString("ERROR: lseek failed: %1").arg(strerror(errno)));
        ::close(fd);
        setResult(false, "lseek failed");
        testBtn->setEnabled(true);
        return;
    }
    QByteArray readData(testSize, 0);
    ssize_t didRead = ::read(fd, readData.data(), testSize);
    ::close(fd);
    if (didRead != testSize) {
        appendLog(QString("ERROR: read returned %1 (expected %2): %3").arg(didRead).arg(testSize).arg(strerror(errno)));
        setResult(false, "read failed");
        testBtn->setEnabled(true);
        return;
    }
    appendLog(QString("Read OK (%1 bytes)").arg(didRead));

    // Verify
    const auto wHash = QCryptographicHash::hash(writeData, QCryptographicHash::Sha256).toHex();
    const auto rHash = QCryptographicHash::hash(readData,  QCryptographicHash::Sha256).toHex();
    appendLog(QString("Write SHA256: %1").arg(QString(wHash)));
    appendLog(QString("Read  SHA256: %1").arg(QString(rHash)));

    if (wHash == rHash) {
        appendLog("VERIFY OK — data matches");
        setResult(true);
    } else {
        appendLog("VERIFY FAIL — data mismatch");
        // show first differing byte
        for (int i = 0; i < testSize; ++i) {
            if (writeData[i] != readData[i]) {
                appendLog(QString("First diff at byte %1: wrote 0x%2, read 0x%3")
                          .arg(i)
                          .arg((quint8)writeData[i], 2, 16, QChar('0'))
                          .arg((quint8)readData[i],  2, 16, QChar('0')));
                break;
            }
        }
        setResult(false, "data mismatch");
    }

    testBtn->setEnabled(true);
}
