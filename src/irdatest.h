#pragma once

#include <QWidget>

class QLabel;
class QPushButton;
class QLineEdit;
class QPlainTextEdit;

class IrdaTest : public QWidget {
    Q_OBJECT
public:
    explicit IrdaTest(QWidget* parent = nullptr);

private slots:
    void sendTest();
    void setStatus(const QString& text, bool error = false);
    void appendLog(const QString& text);

private:
    QLabel* m_status = nullptr;
    QLineEdit* m_input = nullptr;
    QPushButton* m_sendBtn = nullptr;
    QPlainTextEdit* m_log = nullptr;
};
