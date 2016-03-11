/*
Jentos IDE.

Copyright 2014, Evgeniy Goroshkin.

See LICENSE.TXT for licensing terms.
*/

#ifndef LISTWIDGETCOMPLETE_H
#define LISTWIDGETCOMPLETE_H

#include "std.h"

class CodeItem;


class ListWidgetComplete : public QListWidget {
    Q_OBJECT
public:
    ListWidgetComplete(QWidget *parent = 0);
    void selectNear(int dir);
    void setIsForInheritance(bool value) { _isForInheritance = value; }
    bool isForInheritance() { return _isForInheritance; }
    //void addItem(const QString &label);
    void addWidgetForItem(QListWidgetItem *item);

signals:
    void activated(CodeItem *item);
    void focusOut();

protected:
    void keyPressEvent(QKeyEvent *event);
    void focusOutEvent(QFocusEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void show();

private:
    bool _isForInheritance;
    void updateItem(QListWidgetItem *item);

};



class ListWidgetCompleteItem : public QListWidgetItem {
    //Q_OBJECT
public:
    ListWidgetCompleteItem(const QIcon &icon, const QString &text, CodeItem *code, QListWidget *parent = 0);
    CodeItem* codeItem(){ return _codeItem; }
    void setCodeItem(CodeItem *code){ _codeItem = code; }

private:
    CodeItem *_codeItem;
};

#endif // LISTWIDGETCOMPLETE_H
