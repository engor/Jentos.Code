/*
Ted, a simple text editor/IDE.

Copyright 2012, Blitz Research Ltd.

See LICENSE.TXT for licensing terms.
*/

#include "finddialog.h"
#include "ui_finddialog.h"
#include "prefs.h"

FindDialog::FindDialog( QWidget *parent ):QDialog( parent ),_ui( new Ui::FindDialog ),_used( false ){

    _ui->setupUi( this );
}

FindDialog::~FindDialog(){

    delete _ui;
}

void FindDialog::readSettings(){

    QSettings *set = Prefs::settings();

    if( set->value( "settingsVersion" ).toInt()<2 ) return;

    set->beginGroup( "findDialog" );

    _ui->findText->setText( set->value( "findText" ).toString() );
    _ui->replaceText->setText( set->value( "replaceText" ).toString() );
    _ui->caseSensitive->setChecked( set->value( "caseSensitive" ).toBool() );

    restoreGeometry( set->value( "geometry" ).toByteArray() );

    set->endGroup();
}

void FindDialog::writeSettings(){
    if( !_used ) return;

    QSettings *set = Prefs::settings();

    set->beginGroup( "findDialog" );

    set->setValue( "findText",_ui->findText->text() );
    set->setValue( "replaceText",_ui->replaceText->text() );
    set->setValue( "caseSensitive",_ui->caseSensitive->isChecked() );

    set->setValue( "geometry",saveGeometry() );

    set->endGroup();
}

int FindDialog::exec(){
    QDialog::show();

    if( !_used ){
        restoreGeometry( saveGeometry() );
        _used=true;
    }

    _ui->findText->setFocus( Qt::OtherFocusReason );
    _ui->findText->selectAll();

    return QDialog::exec();
}

QString FindDialog::findText(){
    return _ui->findText->text();
}

QString FindDialog::replaceText(){
    return _ui->replaceText->text();
}

bool FindDialog::caseSensitive(){
    return _ui->caseSensitive->isChecked();
}

void FindDialog::onFindNext(){
    emit findReplace( 0 );
}

void FindDialog::onReplace(){
    emit findReplace( 1 );
}

void FindDialog::onReplaceAll(){
    emit findReplace( 2 );
}


