/*
Jentos IDE.

Copyright 2014, Evgeniy Goroshkin.

See LICENSE.TXT for licensing terms.
*/

#include "listwidgetcomplete.h"

ListWidgetComplete::ListWidgetComplete(QWidget *parent) :
    QListWidget(parent)
{

}

void ListWidgetComplete::keyPressEvent(QKeyEvent *event) {
    int key = event->key();
    if( key == Qt::Key_Escape) {
        emit focusOut();
    }
    else if( key == Qt::Key_Return) {
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


//----------------------------------------------------------------------
ListWidgetCompleteItem::ListWidgetCompleteItem(const QIcon &icon, const QString &text, CodeItem *code, QListWidget *parent)
    : QListWidgetItem(icon,text,parent), _codeItem(code) {

}
