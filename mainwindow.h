#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProcess>

class QTextEdit;
class QToolBar;
class CommandLineEdit;

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
    enum EditorTextStyle {
        DefaultStyle     = 0x01,
        ErrorStyle       = 0x02,
        CommandStyle     = 0x04
    };

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected slots:
    void onReadyRead();
    void onReadyReadError();
    void onReadyReadBindings();
    void runCommand(QString command);
    void onCommandStarted();
    void onCommandFinished();
    void onProcessStateChange(QProcess::ProcessState state);
    void onStopAction();

protected:
    void setEditorTextStyle(MainWindow::EditorTextStyle style);

signals:
    void commandStarted();
    void commandFinished();

private:
    void initProcess();
    void read(bool stderror);
    bool readLines(QByteArray &buffer, QTextEdit *out, int *count = 0);

    QProcess   *const process;
    QTextEdit  *const edit;
    QTextEdit  *const bindings;
    CommandLineEdit *const input;
    QStatusBar *const bar;
    QToolBar   *const tools;
    QAction    *const clearAction;
    QAction    *const stopAction;
    QByteArray        stdoutBuffer;
    QByteArray        stderrBuffer;
    QByteArray        metaBuffer;
    bool              restart;
};

#endif // MAINWINDOW_H
