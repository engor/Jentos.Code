#include "theme.h"
#include "prefs.h"

QString Theme::_theme;
QString Theme::_prevTheme;
bool Theme::_isDark;

Theme::Theme(QObject *parent) : QObject(parent) {

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
    _prevTheme = "";
    QString s = p->getString("theme","netbeans");
    set(s);
}

void Theme::set(QString kind) {
    instance()->setLocal(kind);
}

void Theme::setLocal(QString kind) {
    emit beginChange();
    _prevTheme = _theme;
    _theme = kind;
    save();
    emit endChange();
}

void Theme::save() {
    _isDark = (_theme=="android");
    Prefs *prefs = Prefs::prefs();
    prefs->blockEmitPrefsChanged(true);
    prefs->setValue("theme", _theme);
    if(_theme == "android") {
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
        prefs->setValue( "highlightColorError",QColor( 0x323232 ) );
        prefs->setValue( "highlightColorCaretRow",QColor( 0x323232 ) );
        prefs->setValue( "monkeywordsColor",QColor( 0xc8c8c8 ) );
        prefs->setValue( "userwordsColor",QColor( 0xc8c8c8 ) );
        prefs->setValue( "userwordsDeclColor",QColor( 0xFFC66D ) );
        prefs->setValue( "userwordsVarColor",QColor( 0xAE8ABE ) );
        prefs->setValue( "paramsColor",QColor( 0xcfefefe ) );
    }
    else if(_theme == "netbeans") {
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
        prefs->setValue( "highlightColorError",QColor( 0xE9EFF8 ) );
        prefs->setValue( "highlightColorCaretRow",QColor( 0xE9EFF8 ) );
        prefs->setValue( "monkeywordsColor",QColor( 0xc000001 ) );
        prefs->setValue( "userwordsColor",QColor( 0x000001 ) );
        prefs->setValue( "userwordsDeclColor",QColor( 0x000001 ) );
        prefs->setValue( "userwordsVarColor",QColor( 0x009900 ) );
        prefs->setValue( "paramsColor",QColor( 0xc80808 ) );
    }
    else if(_theme == "qt") {
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
        prefs->setValue( "highlightColorError",QColor( 0xE9EFF8 ) );
        prefs->setValue( "highlightColorCaretRow",QColor( 0xF7F7F7 ) );
        prefs->setValue( "monkeywordsColor",QColor( 0xc800080 ) );
        prefs->setValue( "userwordsColor",QColor( 0x800080 ) );
        prefs->setValue( "userwordsDeclColor",QColor( 0x000001 ) );
        prefs->setValue( "userwordsVarColor",QColor( 0x800000 ) );
        prefs->setValue( "paramsColor",QColor( 0xc80808 ) );
        //cur line #c2e1ff
    }
    prefs->blockEmitPrefsChanged(false, true);
}

void Theme::load() {
    /*_theme = "android";
    Prefs *prefs = Prefs::prefs();
    QString s = prefs->getString("theme");
    if(s != "")
        _theme = s;*/
}

QIcon Theme::icon(QString name) {
    QString s = (_theme=="android" ? "dark" : "light");
    return QIcon(":/icons/"+s+"/"+name);
}

QImage Theme::image(QString name, int theme) {
    QString t = _theme;
    if(theme == 1)
        t = "android";
    else if(theme == 2)
        t = "netbeans";
    QString s = (t=="android" ? "dark" : "light");
    return QImage(":/icons/"+s+"/"+name);
}

bool Theme::isDark() {
    return _isDark;
}

QString Theme::hexColor(const QColor &color) {
    QString r = QString::number(color.red(),16);
    if(r.length() < 2)
        r = "0"+r;
    QString g = QString::number(color.green(),16);
    if(g.length() < 2)
        g = "0"+g;
    QString b = QString::number(color.blue(),16);
    if(b.length() < 2)
        b = "0"+b;
    return "#"+r+g+b;
}

QColor Theme::selWordColor() {
    static QColor c1(66,66,66);
    static QColor c2(236,235,163);//(225,225,225);
    static QColor c3(225,225,225);
    if(_theme == "android")
        return c1;
    else if(_theme == "netbeans")
        return c2;
    else
        return c3;
}
