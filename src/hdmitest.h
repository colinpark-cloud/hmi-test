#pragma once

#include <QWidget>

class QLabel;
class QPushButton;
class QPlainTextEdit;

class HdmiTest : public QWidget {
    Q_OBJECT
public:
    explicit HdmiTest(QWidget* parent = nullptr);

private slots:
    void refreshStatus();
    void appendLog(const QString& text);

private:
    QLabel* m_status = nullptr;
    QLabel* m_mode = nullptr;
    QLabel* m_edid = nullptr;
    QPushButton* m_refreshBtn = nullptr;
    QPlainTextEdit* m_log = nullptr;
};
