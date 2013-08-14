#include <QKeyEvent>
#include "commandlineedit.h"

CommandLineEdit::CommandLineEdit(QWidget *parent)
    : QLineEdit(parent),
      index(0)
{
    commands.prepend("");
    connect(this, SIGNAL(textEdited(QString)),
            this, SLOT(onTextEdited(QString)));
}

CommandLineEdit::~CommandLineEdit()
{
}

void CommandLineEdit::insertCommand(QString command)
{
    history.prepend(command);
    commands = history;
    commands.prepend("");
    index = 0;
}

void CommandLineEdit::onTextEdited(QString text)
{
    commands[index] = text;
}

void CommandLineEdit::keyPressEvent(QKeyEvent *event)
{
    switch (event->key())
    {
    case Qt::Key_Up:
        if (index + 1 < commands.size()) {
            setText(commands.at(++index));
        }
        break;
    case Qt::Key_Down:
        if (index) {
            setText(commands.at(--index));
        }
        break;
    case Qt::Key_Return:
    case Qt::Key_Enter:
    {
        QString command = text();
        if (!command.isEmpty()) {
            emit enterKeyPressed(command);
            insertCommand(command);
            clear();
        }
        break;
    }
    default:
        commands[index] = text();
        break;
    }

    QLineEdit::keyPressEvent(event);
}
