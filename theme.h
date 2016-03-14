#ifndef THEME_H
#define THEME_H

#include "std.h"


class Theme : public QObject {

    Q_OBJECT

public:

    static QString ANDROID_STUDIO, QT_CREATOR, NETBEANS, LIGHT_TABLE, DARK_SODA;

    Theme(QObject *parent=0);
    static QString theme(){ return _theme; }
    static QString prevTheme(){ return _prevTheme; }
    static bool isCurrent(QString kind) {
        return _theme == kind;
    }
    static void init();
    static void set(QString kind);
    static void save();
    static void load();
    static QIcon icon(QString name);
    static QImage imageLight(QString name);
    static QImage imageDark(QString name);
    static bool isDark();
    static QString hexColor(const QColor &color);
    static QColor selWordColor();
    static Theme* instance() {
        static Theme *t = 0;
        if(!t)
            t = new Theme;
        return t;
    }
    void setLocal(QString kind);

signals:
    void beginChange();
    void endChange();

private:

    static QString _theme, _prevTheme;
    static bool _isDark;

};

#endif // THEME_H
