#pragma once

#include <QWidget>

class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;

class BarcodeTest : public QWidget {
    Q_OBJECT
public:
    explicit BarcodeTest(QWidget* parent = nullptr);

protected:
    void showEvent(QShowEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* ev) override;

private slots:
    void commitScan();
    void clearScans();
    void ensureFocus();

private:
    void setStatus(const QString& text);
    QString sanitizeScan(const QString& text) const;

    QLabel* m_title = nullptr;
    QLabel* m_hint = nullptr;
    QLabel* m_status = nullptr;
    QLabel* m_lastScan = nullptr;
    QLineEdit* m_input = nullptr;
    QListWidget* m_history = nullptr;
    QPushButton* m_clearBtn = nullptr;
    int m_scanCount = 0;
};
