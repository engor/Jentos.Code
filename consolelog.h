#ifndef CONSOLELOG_H
#define CONSOLELOG_H

#include <qstring.h>
#include <qwidget.h>


class ConsoleLog {
public:
    virtual void debug( const QString &str ) = 0;

protected:
    virtual void print(const QString &str , QString kind) = 0;
    virtual void runCommand( QString cmd ,QWidget *fileWidget) = 0;
    virtual void build( QString mode,QString pathmonkey) = 0;
};

#endif // CONSOLELOG_H