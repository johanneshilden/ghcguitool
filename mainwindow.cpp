#include <QStringBuilder>
#include <QDebug>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QPushButton>
#include <QKeyEvent>
#include <QEvent>
#include <QSplitter>
#include <QStatusBar>
#include <QToolBar>
#include <QAction>
#include <QTextDocumentFragment>
#include "mainwindow.h"
#include "commandlineedit.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      process(new QProcess(this)),
      edit(new QTextEdit),
      bindings(new QTextEdit),
      input(new CommandLineEdit),
      bar(statusBar()),
      tools(addToolBar("")),
      clearAction(tools->addAction(tr("Clear buffer"))),
      stopAction(tools->addAction(tr("Stop"))),
      restart(false)
{
    stopAction->setEnabled(false);

    tools->setMovable(false);
    tools->setContextMenuPolicy(Qt::CustomContextMenu);

    edit->setReadOnly(true);
    bindings->setReadOnly(true);

    edit->setLineWrapMode(QTextEdit::WidgetWidth);
    bindings->setLineWrapMode(QTextEdit::NoWrap);

    edit->setUndoRedoEnabled(false);
    bindings->setUndoRedoEnabled(false);

    QFont font("monospace");

    edit->setFont(font);
    input->setFont(font);
    bindings->setFont(font);

    QWidget *widget = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout;

    QSplitter *splitter = new QSplitter(Qt::Horizontal);
    splitter->addWidget(edit);
    splitter->addWidget(bindings);

    QList<int> sizes;
    sizes.push_back(490);
    sizes.push_back(160);
    splitter->setSizes(sizes);

    widget->setMinimumSize(650, 400);

    widget->setLayout(layout);
    layout->addWidget(splitter);
    layout->addWidget(input);
    setCentralWidget(widget);

    connect(this, SIGNAL(commandStarted()), this, SLOT(onCommandStarted()));
    connect(this, SIGNAL(commandFinished()), this, SLOT(onCommandFinished()));

    connect(clearAction, SIGNAL(triggered()), edit, SLOT(clear()));
    connect(stopAction, SIGNAL(triggered()), this, SLOT(onStopAction()));

    connect(input, SIGNAL(enterKeyPressed(QString)),
            this, SLOT(runCommand(QString)));

    initProcess();
}

MainWindow::~MainWindow()
{
    process->terminate();
    process->waitForFinished(2000);
    process->deleteLater();
}

void MainWindow::onReadyRead()
{
    setEditorTextStyle(MainWindow::DefaultStyle);

    stdoutBuffer.append(process->readAllStandardOutput());

    int n;
    if (readLines(stdoutBuffer, edit, &n)) {
        emit onCommandFinished();
        disconnect(process, SIGNAL(readyReadStandardOutput()), 0, 0);
        connect(process, SIGNAL(readyReadStandardOutput()),
                this, SLOT(onReadyReadBindings()));
        bindings->clear();
        process->write(":show bindings\n");
    }

    if (n) {
        edit->moveCursor(QTextCursor::End);
        QList<QTextEdit::ExtraSelection> extraSelections;
        QTextEdit::ExtraSelection selection;
        QColor lineColor = QColor(Qt::yellow).lighter(140);
        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = edit->textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
        edit->setExtraSelections(extraSelections);
    }
}

void MainWindow::onReadyReadError()
{
    setEditorTextStyle(MainWindow::ErrorStyle);

    stderrBuffer.append(process->readAllStandardError());
    int n;
    readLines(stderrBuffer, edit, &n);

    if (n) {
        edit->moveCursor(QTextCursor::End);
        QList<QTextEdit::ExtraSelection> extraSelections;
        edit->setExtraSelections(extraSelections);
    }
}

void MainWindow::onReadyReadBindings()
{
    metaBuffer.append(process->readAllStandardOutput());
    if (readLines(metaBuffer, bindings)) {
        disconnect(process, SIGNAL(readyReadStandardOutput()), 0, 0);
        connect(process, SIGNAL(readyReadStandardOutput()),
                this, SLOT(onReadyRead()));
    }
}

void MainWindow::runCommand(QString command)
{
    QByteArray ba = command.toLocal8Bit();
    setEditorTextStyle(MainWindow::CommandStyle);
    edit->append(ba);
    ba.append("\n");

    process->write(ba);
    emit commandStarted();

    edit->moveCursor(QTextCursor::End);
    QList<QTextEdit::ExtraSelection> extraSelections;
    edit->setExtraSelections(extraSelections);
}

void MainWindow::onCommandStarted()
{
    stopAction->setEnabled(true);
    bar->showMessage("...");
}

void MainWindow::onCommandFinished()
{
    stopAction->setEnabled(false);
    bar->clearMessage();
}

void MainWindow::onProcessStateChange(QProcess::ProcessState state)
{
    Q_UNUSED(state)

    //qDebug() << "State change";
}

void MainWindow::onStopAction()
{
    process->kill();
    process->waitForFinished(1500);
    stdoutBuffer.clear();
    stderrBuffer.clear();
    metaBuffer.clear();
    restart = true;
    initProcess();
}

void MainWindow::setEditorTextStyle(MainWindow::EditorTextStyle style)
{
    switch (style)
    {
    case MainWindow::DefaultStyle:
        edit->setTextColor(Qt::black);
        edit->setFontWeight(QFont::Normal);
        edit->setTextBackgroundColor(Qt::transparent);
        break;
    case MainWindow::ErrorStyle:
        edit->setTextColor(QColor(Qt::red).darker(110));
        edit->setFontWeight(QFont::Normal);
        edit->setTextBackgroundColor(Qt::transparent);
        break;
    case MainWindow::CommandStyle:
        edit->setTextColor(QColor(Qt::blue));
        edit->setFontWeight(QFont::Bold);
        edit->setTextBackgroundColor(QColor(Qt::gray).lighter(130));
        break;
    default:
        break;
    }
}

void MainWindow::initProcess()
{
    process->setProcessChannelMode(QProcess::SeparateChannels);

    connect(process, SIGNAL(readyReadStandardOutput()),
            this, SLOT(onReadyRead()));
    connect(process, SIGNAL(readyReadStandardError()),
            this, SLOT(onReadyReadError()));

    connect(process, SIGNAL(stateChanged(QProcess::ProcessState)),
            this, SLOT(onProcessStateChange(QProcess::ProcessState)));

    QStringList options;
    options.append("-ignore-dot-ghci");

    QByteArray program = "/usr/bin/ghci";
    process->start(program, options);
}

bool MainWindow::readLines(QByteArray &buffer, QTextEdit *out, int *count)
{
    QList<QByteArray> xs = buffer.split('\n');
    if (count)
        *count = 0;
    if (xs.count() > 1) {
        QList<QByteArray>::const_iterator i;
        for (i = xs.constBegin(); i != xs.constEnd() - 1; ++i) {
            QByteArray ba = *i;
            if (!ba.isEmpty()) {
                out->append(ba);
                if (count)
                    ++(*count);
            }
        }
        buffer = *i;
    }

    bool ret = false;
    while (buffer.startsWith("Prelude> ")) {
        buffer = buffer.mid(9);
        ret = true;
    }
    return ret;
}
