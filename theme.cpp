#include "theme.h"
#include "prefs.h"

QString Theme::ANDROID_STUDIO = "Android Studio";
QString Theme::NETBEANS = "NetBeans";
QString Theme::QT_CREATOR = "Qt Creator";
QString Theme::DARK_SODA = "Monokai Dark Soda";
QString Theme::LIGHT_TABLE = "Light Table";

QString Theme::_theme;
QString Theme::_prevTheme;
bool Theme::_isDark;
QStringList Theme::_themes;

Theme::Theme(QObject *parent) : QObject(parent) {

}

QString Theme::getDefault()
{
    return QT_CREATOR;
}

void Theme::init() {
    Prefs *p = Prefs::prefs();

    if(!p->contains("fontFamily"))
        p->setValue( "fontFamily","Courier" );
    if(!p->contains("fontSize"))
        p->setValue( "fontSize",12 );
    if(!p->contains("tabSize"))
        p->setValue( "tabSize",4 );
    if(!p->contains("smoothFonts"))
        p->setValue( "smoothFonts",true );

    _themes.clear();
    _themes <<ANDROID_STUDIO<<NETBEANS<<QT_CREATOR<<DARK_SODA<<LIGHT_TABLE;

    _prevTheme = "";
    QString s = p->getString("theme", "err");
    bool err = (s == "err");
    if (err) {
        s = getDefault();
    }
    set(s, err);
}

void Theme::set(QString kind, bool setColors) {
    instance()->setLocal(kind, setColors);
}

void Theme::setLocal(QString kind, bool setColors) {
    emit beginChange();
    _prevTheme = _theme;
    _theme = _themes.contains(kind) ? kind : getDefault();
    _isDark = (_theme==ANDROID_STUDIO || _theme==DARK_SODA || _theme==LIGHT_TABLE);
    save(setColors);
    emit endChange();
}

void Theme::save(bool setColors) {

    Prefs::prefs()->setValue("theme", _theme);

    if (!setColors)
        return;

    adjustDefaultColors(false);
}

void Theme::adjustDefaultColors(bool emitSignals)
{
    Prefs *prefs = Prefs::prefs();

    if (!emitSignals)
        prefs->blockEmitPrefsChanged(true);

    if (_theme == ANDROID_STUDIO) {
        prefs->setValue( "backgroundColor",QColor( 0x2b2b2b ) );
        prefs->setValue( "defaultColor",QColor( 0xAbB9C8 ) );
        prefs->setValue( "numbersColor",QColor( 0x6897BB ) );
        prefs->setValue( "stringsColor",QColor( 0x6A8759 ) );
        prefs->setValue( "identifiersColor",QColor( 0xCEDDE5 ) );
        prefs->setValue( "keywordsColor",QColor(  0xCC7832 ) );
        prefs->setValue( "constantsColor",QColor( 0x9876AA ) );
        prefs->setValue( "funcDeclsColor",QColor( 0xFFC66D ) );
        prefs->setValue( "commentsColor",QColor( 0x808080 ) );
        prefs->setValue( "highlightColor",QColor( 0x323232 ) );
        prefs->setValue( "highlightErrorColor",QColor( 0x323232 ) );
        prefs->setValue( "highlightCaretRowColor",QColor( 0x323232 ) );
        prefs->setValue( "monkeywordsColor",QColor( 0xc8c8c8 ) );
        prefs->setValue( "userwordsColor",QColor( 0xc8c8c8 ) );
        prefs->setValue( "userwordsDeclColor",QColor( 0xFFC66D ) );
        prefs->setValue( "userwordsVarColor",QColor( 0xAE8ABE ) );
        prefs->setValue( "paramsColor",QColor( 0xcfefefe ) );
        prefs->setValue( "wordUnderCursorColor",QColor( 0x3c3c3c ) );
    }
    else if (_theme == NETBEANS) {
        prefs->setValue( "backgroundColor",QColor( 0xf5f5f5 ) );
        prefs->setValue( "defaultColor",QColor( 0x000001 ) );
        prefs->setValue( "numbersColor",QColor( 0x000001 ) );
        prefs->setValue( "stringsColor",QColor( 0xce7b00 ) );
        prefs->setValue( "identifiersColor",QColor( 0x009900 ) );
        prefs->setValue( "keywordsColor",QColor( 0x0000e6 ) );
        prefs->setValue( "constantsColor",QColor( 0x000001 ) );
        prefs->setValue( "funcDeclsColor",QColor( 0x000001 ) );
        prefs->setValue( "commentsColor",QColor( 0x606060 ) );
        prefs->setValue( "highlightColor",QColor( 0xE9EFF8 ) );
        prefs->setValue( "highlightErrorColor",QColor( 0xE9EFF8 ) );
        prefs->setValue( "highlightCaretRowColor",QColor( 0xE9EFF8 ) );
        prefs->setValue( "monkeywordsColor",QColor( 0xc000001 ) );
        prefs->setValue( "userwordsColor",QColor( 0x000001 ) );
        prefs->setValue( "userwordsDeclColor",QColor( 0x000001 ) );
        prefs->setValue( "userwordsVarColor",QColor( 0x009900 ) );
        prefs->setValue( "paramsColor",QColor( 0xc80808 ) );
        prefs->setValue( "wordUnderCursorColor",QColor( 0xECEBA3 ) );
    }
    else if (_theme == QT_CREATOR) {
        prefs->setValue( "backgroundColor",QColor( 0xfefefe ) );
        prefs->setValue( "defaultColor",QColor( 0x000001 ) );
        prefs->setValue( "numbersColor",QColor( 0x000080 ) );
        prefs->setValue( "stringsColor",QColor( 0x008000 ) );
        prefs->setValue( "identifiersColor",QColor( 0x009900 ) );
        prefs->setValue( "keywordsColor",QColor( 0x808000 ) );
        prefs->setValue( "constantsColor",QColor( 0x000001 ) );
        prefs->setValue( "funcDeclsColor",QColor( 0x800080 ) );
        prefs->setValue( "commentsColor",QColor( 0x606060 ) );
        prefs->setValue( "highlightColor",QColor( 0xE9EFF8 ) );
        prefs->setValue( "highlightErrorColor",QColor( 0xE9EFF8 ) );
        prefs->setValue( "highlightCaretRowColor",QColor( 0xF7F7F7 ) );
        prefs->setValue( "monkeywordsColor",QColor( 0xc800080 ) );
        prefs->setValue( "userwordsColor",QColor( 0x800080 ) );
        prefs->setValue( "userwordsDeclColor",QColor( 0x000001 ) );
        prefs->setValue( "userwordsVarColor",QColor( 0x800000 ) );
        prefs->setValue( "paramsColor",QColor( 0xc80808 ) );
        prefs->setValue( "wordUnderCursorColor",QColor( 0xE4E4E4 ) );
    }

    else if (_theme == DARK_SODA) {
        prefs->setValue( "backgroundColor",QColor( 0x242424 ) );
        prefs->setValue( "defaultColor",QColor( 0xDDDDDD ) );
        prefs->setValue( "numbersColor",QColor( 0xAE81FF ) );
        prefs->setValue( "stringsColor",QColor( 0xE3D874 ) );
        prefs->setValue( "identifiersColor",QColor( 0x009900 ) );
        prefs->setValue( "keywordsColor",QColor( 0xF92672 ) );
        prefs->setValue( "constantsColor",QColor( 0xA6E22E ) );
        prefs->setValue( "funcDeclsColor",QColor( 0xFD971F ) );
        prefs->setValue( "commentsColor",QColor( 0x8c8c8c ) );
        prefs->setValue( "highlightColor",QColor( 0xB5B5B5 ) );
        prefs->setValue( "highlightErrorColor",QColor( 0xe40000 ) );
        prefs->setValue( "highlightCaretRowColor",QColor( 0x3A2A21 ) );
        prefs->setValue( "monkeywordsColor",QColor( 0x2BBF1C ) );
        prefs->setValue( "userwordsColor",QColor( 0xfd971f ) );
        prefs->setValue( "userwordsDeclColor",QColor( 0xa6e22e ) );
        prefs->setValue( "userwordsVarColor",QColor( 0x9effff ) );
        prefs->setValue( "paramsColor",QColor( 0x66D9EF ) );
        prefs->setValue( "wordUnderCursorColor",QColor( 0x3c3c3c ) );
    }

    else if (_theme == LIGHT_TABLE) {
        prefs->setValue( "backgroundColor",QColor( 0x202020 ) );
        prefs->setValue( "defaultColor",QColor( 0xc6c6c6 ) );
        prefs->setValue( "numbersColor",QColor( 0xFEFEFE ) );
        prefs->setValue( "stringsColor",QColor( 0x318F8F ) );
        prefs->setValue( "identifiersColor",QColor( 0x009900 ) );
        prefs->setValue( "keywordsColor",QColor( 0x43AA79 ) );
        prefs->setValue( "constantsColor",QColor( 0xA6E22E ) );
        prefs->setValue( "funcDeclsColor",QColor( 0xAACCFF ) );
        prefs->setValue( "commentsColor",QColor( 0x6688CD ) );
        prefs->setValue( "highlightColor",QColor( 0xea1717 ) );
        prefs->setValue( "highlightErrorColor",QColor( 0x40000 ) );
        prefs->setValue( "highlightCaretRowColor",QColor( 0x423434 ) );
        prefs->setValue( "monkeywordsColor",QColor( 0xAFCDFB ) );
        prefs->setValue( "userwordsColor",QColor( 0x2886AC ) );
        prefs->setValue( "userwordsDeclColor",QColor( 0xCCAAFF ) );
        prefs->setValue( "userwordsVarColor",QColor( 0x2CA2A2 ) );
        prefs->setValue( "paramsColor",QColor( 0x66D9EF ) );
        prefs->setValue( "wordUnderCursorColor",QColor( 0x3c3c3c ) );
    }

    if (!emitSignals)
        prefs->blockEmitPrefsChanged(false, true);
}

QIcon Theme::icon(QString name) {
    QString s = (_isDark ? "dark" : "light");
    return QIcon(":/icons/"+s+"/"+name);
}

QImage Theme::imageLight(QString name) {
    return QImage(":/icons/light/"+name);
}
QImage Theme::imageDark(QString name) {
    return QImage(":/icons/dark/"+name);
}

bool Theme::isDark() {
    return _isDark;
}
