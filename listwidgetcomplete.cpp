/*
Jentos IDE.

Copyright 2014, Evgeniy Goroshkin.

See LICENSE.TXT for licensing terms.
*/

#include "listwidgetcomplete.h"

ListWidgetComplete::ListWidgetComplete(QWidget *parent) :
    QListWidget(parent)
{
    _isForInheritance = false;
}

void ListWidgetComplete::keyPressEvent(QKeyEvent *event) {
    int key = event->key();
    if( key == Qt::Key_Escape ) {
        emit focusOut();
    }
    else if( key == Qt::Key_Return || key == Qt::Key_Tab ) {
        emit itemActivated( currentItem() );
        //emit activated(_codeItem);
    }
    else {
        QListWidget::keyPressEvent(event);
    }
    event->accept();
}

void ListWidgetComplete::focusOutEvent(QFocusEvent* event) {
    emit focusOut();
}

void ListWidgetComplete::mouseDoubleClickEvent(QMouseEvent *event) {
    emit itemActivated( currentItem() );
}

void ListWidgetComplete::updateItem(QListWidgetItem *item)
{
    //qDebug()<<"updateItem";
    QLabel *widget = new QLabel(this);
    widget->setText(item->text());
    setItemWidget(item, widget);
}

void ListWidgetComplete::selectNear(int dir) {

    int cnt = count();
    if(cnt <= 1)
        return;
    int row = currentRow();
    if(dir == -1) {
        --row;
        if(row < 0)
            row = cnt-1;
        setCurrentRow(row);

    }
    else if(dir == 1) {
        ++row;
        if(row >= cnt)
            row = 0;
        setCurrentRow(row);
    }
}

/*void ListWidgetComplete::addItem(const QString &label)
{
    QListWidget::addItem(label);
    QListWidgetItem *it = item(count()-1);
    updateItem(it);
}*/

void ListWidgetComplete::addWidgetForItem(QListWidgetItem *item)
{
    updateItem(item);
}


//----------------------------------------------------------------------
ListWidgetCompleteItem::ListWidgetCompleteItem(const QIcon &icon, const QString &text, CodeItem *code, QListWidget *parent)
    : QListWidgetItem(icon,text,parent), _codeItem(code)
{

}
