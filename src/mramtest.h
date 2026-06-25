#pragma once
#include <QWidget>

class QPushButton;
class QLabel;
class QPlainTextEdit;

class MramTest : public QWidget {
    Q_OBJECT
public:
    explicit MramTest(QWidget* parent = nullptr);

private slots:
    void runTest();

private:
    QPushButton *testBtn = nullptr;
    QLabel      *resultLabel = nullptr;
    QPlainTextEdit *log = nullptr;

    void appendLog(const QString& line);
    void setResult(bool pass, const QString& detail = {});
};
