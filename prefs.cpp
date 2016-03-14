/*
Ted, a simple text editor/IDE.

Copyright 2012, Blitz Research Ltd.

See LICENSE.TXT for licensing terms.
*/

#include "prefs.h"


Prefs::Prefs(){
    _blockEmitPrefsChanged = false;
    isValidMonkeyPath = false;

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
    if (!s) {
        //s = new QSettings(QApplication::applicationDirPath()+"/settings.ini",QSettings::IniFormat);
        s = new QSettings();
        int size = s->allKeys().size();
        // if has no data then import from previous version
        if (size == 0) {
            QString dir = extractDir(s->fileName())+"/";
            QString p = dir + "Jentos IDE.ini";
            QFile f(p);
            if (f.exists()) {
                QString p2 = dir + "Jentos.Code.ini";
                QFile f2(p2);
                if (f2.exists())
                    f2.remove();
                f.copy(p2);
            }
        }
        delete s;
        s = new QSettings();

        prefs()->setDefaults();
    }
    return s;
}

void Prefs::setDefaults() {
    Prefs *p = prefs();
    if (!p->contains("CharsForCompletion"))
        p->setValue("CharsForCompletion", 3);
    if (!p->contains("addVoidForMethods"))
        p->setValue("addVoidForMethods", true);
    if (!p->contains("AutoBracket"))
        p->setValue("AutoBracket", true);

    if (!p->contains("updates")) {
        p->setValue("updates",true);
        p->setValue("tabSize",4);
        p->setValue("fontSize",12);
        p->setValue("highlightLine",true);
        p->setValue("highlightWord",true);
        p->setValue("style","Default");
        p->setValue("showHelpInDock",false);
        p->setValue("replaceDocsStyle",true);
    }
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
