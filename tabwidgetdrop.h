/*
Jentos IDE.

Copyright 2014, Evgeniy Goroshkin.

See LICENSE.TXT for licensing terms.
*/

#ifndef TABWIDGETDROP_H
#define TABWIDGETDROP_H

#include "std.h"

class TabWidgetDrop : public QTabWidget
{
    Q_OBJECT
public:
    TabWidgetDrop(QWidget *parent = 0);
    
signals:
    void dropFiles( const QStringList &list );

protected:
    void dropEvent(QDropEvent *e);
    void dragEnterEvent(QDragEnterEvent* e);
    void dragMoveEvent(QDragMoveEvent* e);
    void dragLeaveEvent(QDragLeaveEvent* e);

public slots:
    
};

#endif // TABWIDGETDROP_H
