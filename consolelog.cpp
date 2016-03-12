#include "consolelog.h"

#include <QDebug>

ConsoleLog::ConsoleLog() {}

void ConsoleLog::debug( const QString &str ){
    print( str, "debug" );
}

void ConsoleLog::print( const QString &str , QString kind ){
    if(kind == "") {
        qInfo() << str;
    } else if(kind == "error") {
        qDebug() << str;
    } else if(kind == "finish") {
        qInfo() << str;
    } else if(kind == "debug") {
        qWarning() << str;
    } else if(kind == "fatal") {
        QByteArray ba = str.toLatin1();
        qFatal(ba.data());
    } else if(kind == "critical") {
        qCritical() << str;
    }
}
