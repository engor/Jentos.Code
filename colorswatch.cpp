/*
Ted, a simple text editor/IDE.

Copyright 2012, Blitz Research Ltd.

See LICENSE.TXT for licensing terms.
*/

#include "colorswatch.h"

ColorSwatch::ColorSwatch( QWidget *parent ):QLabel( parent ),_color( 0,0,0 ){

    setAutoFillBackground( true );

    setStyleSheet("background: "+_color.name()+";");
}

QColor ColorSwatch::color(){
    return _color;
}

void ColorSwatch::setColor( const QColor &color ){
    _color = color;
    setStyleSheet("background: "+_color.name()+";");
    emit colorChanged();
}

void ColorSwatch::mousePressEvent( QMouseEvent *ev ){
    setColor( QColorDialog::getColor( _color,this ) );
}
