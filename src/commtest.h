#pragma once

#include <QWidget>

class QLabel;
class QPushButton;
class QTimer;

class CommTest : public QWidget {
    Q_OBJECT
public:
    explicit CommTest(QWidget* parent = nullptr);

private slots:
    void toggleRun();
    void onTick();

private:
    enum class TargetKind { Com1, Com2, Lan1, Lan2 };
    struct TargetRow {
        TargetKind kind;
        QString title;
        QString detail;
        QString device;
        QString ip;
        QLabel* titleLabel = nullptr;
        QPushButton* ipButtons[4] = {nullptr, nullptr, nullptr, nullptr};
        QLabel* detailLabel = nullptr;
        QLabel* totalLabel = nullptr;
        QLabel* passLabel = nullptr;
        QLabel* failLabel = nullptr;
        int totalCount = 0;
        int passCount = 0;
        int failCount = 0;
    };

    void buildUi();
    void resetCounters();
    void updateRow(TargetRow& row);
    bool checkComPort(const QString& device) const;
    bool checkLanLink(const QString& ip) const;
    bool checkSerialRoundTrip(const QString& device) const;
    void setRunButton();
    void updateClock();
    void bumpTotal(int rowIndex);
    void addResult(int rowIndex, bool ok);

    QVector<TargetRow> m_rows;
    QLabel* m_clockLabel = nullptr;
    QPushButton* m_runBtn = nullptr;
    QTimer* m_timer = nullptr;
    QTimer* m_clockTimer = nullptr;
    int m_cycle = 0;
    bool m_running = false;
};
