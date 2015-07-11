#ifndef THEME_H
#define THEME_H

#include "std.h"


class Theme : public QObject {

    Q_OBJECT

public:

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
    static QImage image(QString name, int theme=0);
    static bool isDark();
    static bool isDark2();
    static bool isDark3();
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
    static bool _isDark2;
    static bool _isDark3;

};

#endif // THEME_H
