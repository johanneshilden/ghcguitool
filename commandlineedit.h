#ifndef COMMANDLINEEDIT_H
#define COMMANDLINEEDIT_H

#include <QLineEdit>

class CommandLineEdit : public QLineEdit
{
    Q_OBJECT

public:
    explicit CommandLineEdit(QWidget *parent = 0);
    ~CommandLineEdit();

signals:
    void enterKeyPressed(QString);

public slots:
    void insertCommand(QString command);

protected slots:
    void onTextEdited(QString);

protected:
    void keyPressEvent(QKeyEvent *event);

private:
    QStringList history;
    QStringList commands;
    int index;
};

#endif // COMMANDLINEEDIT_H
