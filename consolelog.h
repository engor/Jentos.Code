#ifndef DEBUGWINDOW_H
#define DEBUGWINDOW_H

#include <qstring.h>
#include <qwidget.h>


class ConsoleLog {
public:
    ConsoleLog();
    void debug( const QString &str );

private:
    void print(const QString &str , QString kind);
    virtual void runCommand( QString cmd ,QWidget *fileWidget) = 0;
    virtual void build( QString mode,QString pathmonkey) = 0;
};

#endif // DEBUGWINDOW_H
