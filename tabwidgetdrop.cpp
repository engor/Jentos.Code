/*
Jentos IDE.

Copyright 2014, Evgeniy Goroshkin.

See LICENSE.TXT for licensing terms.
*/

#include "tabwidgetdrop.h"

TabWidgetDrop::TabWidgetDrop(QWidget *parent) :
    QTabWidget(parent)
{
}

void TabWidgetDrop::dropEvent(QDropEvent *e) {
    const QMimeData *mimeData = e->mimeData();
    if (mimeData->hasUrls()) {
        QStringList pathList;
        QList<QUrl> urlList = mimeData->urls();
        for (int i = 0; i < urlList.size() && i < 32; ++i) {
            pathList.append(urlList.at(i).toLocalFile());
            qDebug() << "file: "+urlList.at(i).toLocalFile();
        }
        e->acceptProposedAction();
        emit dropFiles(pathList);
    }
}

void TabWidgetDrop::dragEnterEvent(QDragEnterEvent* e) {
    e->acceptProposedAction();
}

void TabWidgetDrop::dragMoveEvent(QDragMoveEvent* e) {
    e->acceptProposedAction();
}

void TabWidgetDrop::dragLeaveEvent(QDragLeaveEvent* e) {
    e->accept();
}
