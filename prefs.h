/*
Ted, a simple text editor/IDE.

Copyright 2012, Blitz Research Ltd.

See LICENSE.TXT for licensing terms.
*/

#ifndef PREFS_H
#define PREFS_H

#include "std.h"

class Prefs : public QObject{
    Q_OBJECT

public:

    void setDefaults();
    void setValue( const QString &name,const QVariant &value );
    bool contains(const QString &name) {
        settings()->beginGroup( "userPrefs" );
        bool v = settings()->contains(name);
        settings()->endGroup();
        return v;
    }

    bool getBool( const QString &name, bool def=false ) {
        settings()->beginGroup( "userPrefs" );
        bool v = (settings()->contains(name) ? settings()->value( name ).toBool() : def);
        settings()->endGroup();
        return v;
    }
    int getInt( const QString &name, int def=0 ) {
        QSettings *s = settings();
        s->beginGroup( "userPrefs" );
        int v = (s->contains(name) ? s->value( name ).toInt() : def);
        s->endGroup();
        return v;
    }
    QString getString( const QString &name, QString def="") {
        settings()->beginGroup( "userPrefs" );
        QString v = (settings()->contains(name) ? settings()->value( name ).toString() : def);
        settings()->endGroup();
        return v;
    }
    QFont getFont( const QString &name ) {
        settings()->beginGroup( "userPrefs" );
        QFont v = settings()->value( name ).value<QFont>();
        settings()->endGroup();
        return v;
    }
    QColor getColor( const QString &name ) {
        settings()->beginGroup( "userPrefs" );
        QColor v = settings()->value( name ).value<QColor>();
        settings()->endGroup();
        return v;
    }
    void blockEmitPrefsChanged(bool value, bool emitNow=false);

    static Prefs *prefs();
    static QSettings *settings();

signals:

    void prefsChanged( const QString &name );

private:
    //QSettings _settings;
    bool _blockEmitPrefsChanged;
    Prefs();

};

#endif // PREFS_H
