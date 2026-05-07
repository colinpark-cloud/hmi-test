#pragma once
#include <QMainWindow>
#include <QTabWidget>
#include <QTabBar>
#include <QPushButton>
#include <QTransform>

class QProcess;
class QTabWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent=nullptr);
    bool eventFilter(QObject* obj, QEvent* ev) override;

private:
    void applyModelLayout(QTabWidget* tabs);
    QString detectModelName() const;

    QTransform m_calib;
    bool loadCalibration();
    QProcess* m_demoProc = nullptr;
    int m_returnTab = 1;
    bool m_displayWasFullScreen = false;
};
