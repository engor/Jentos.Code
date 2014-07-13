/*
Ted, a simple text editor/IDE.

Copyright 2012, Blitz Research Ltd.

See LICENSE.TXT for licensing terms.
*/

#include "prefs.h"


Prefs::Prefs(){
    _blockEmitPrefsChanged = false;

}

void Prefs::setValue( const QString &name,const QVariant &value ){
    settings()->beginGroup( "userPrefs" );
    settings()->setValue( name,value );
    settings()->endGroup();
    if(!_blockEmitPrefsChanged) {
        emit prefsChanged( name );
    }
}

QSettings* Prefs::settings() {
    static QSettings *s = 0;
    if(!s) {
        //s = new QSettings(QApplication::applicationDirPath()+"/settings.ini",QSettings::IniFormat);
        s = new QSettings();
        //qDebug()<<"settings.filename:"<<s->fileName();
    }
    return s;
}

Prefs *Prefs::prefs() {
    static Prefs *_prefs;
    if( !_prefs ) {
        _prefs = new Prefs;
    }
    return _prefs;
}

void Prefs::blockEmitPrefsChanged(bool value, bool emitNow) {
    _blockEmitPrefsChanged = value;
    if(emitNow) {
        emit prefsChanged( "" );
    }
}
