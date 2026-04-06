#pragma once
#include <QMainWindow>
#include <QTabWidget>
#include <QTabBar>
#include <QPushButton>
#include <QTransform>
class QProcess;  // forward declare

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget* parent=nullptr);
    bool eventFilter(QObject* obj, QEvent* ev) override;
private:
    QTransform m_calib;
    bool loadCalibration();
    bool m_handlingEvent=false;
    // process handle for launched demo
    QProcess* m_demoProc = nullptr;
    // UI members for tab-click handling
    QTabBar* m_tabBar = nullptr;
    QPushButton* m_launchBtn = nullptr;
    QPushButton* m_headerOverlay = nullptr;
    int m_carIndex = -1;
    // provide access to calibration for widgets
    QTransform calibration() const { return m_calib; }
};
