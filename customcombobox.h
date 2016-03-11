#ifndef CUSTOMCOMBOBOX_H
#define CUSTOMCOMBOBOX_H

#include "std.h"

class CustomComboBox : public QComboBox
{
    Q_OBJECT

public:
    CustomComboBox(QWidget *parent = 0);
    ~CustomComboBox();

protected:
    void wheelEvent(QWheelEvent * event);

};

#endif // CUSTOMCOMBOBOX_H
