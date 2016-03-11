#include "customcombobox.h"

CustomComboBox::CustomComboBox(QWidget *parent) : QComboBox(parent)
{

}

CustomComboBox::~CustomComboBox()
{

}

void CustomComboBox::wheelEvent(QWheelEvent *event)
{
    if (!hasFocus()) {
        event->ignore();
    } else {
        QComboBox::wheelEvent(event);
    }
}

