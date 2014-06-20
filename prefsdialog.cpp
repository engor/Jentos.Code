/*
Ted, a simple text editor/IDE.

Copyright 2012, Blitz Research Ltd.

See LICENSE.TXT for licensing terms.
*/

#include "prefsdialog.h"
#include "ui_prefsdialog.h"

#include "prefs.h"
#include "mainwindow.h"

PrefsDialog::PrefsDialog( QWidget *parent ):QDialog( parent ),_ui( new Ui::PrefsDialog ),_prefs( Prefs::prefs() ),_used( false ){
    _ui->setupUi( this );
}

PrefsDialog::~PrefsDialog(){
    delete _ui;
}

void PrefsDialog::readSettings() {
    QSettings *set = Prefs::settings();
    if( set->value( "settingsVersion" ).toInt() < 2 ){
        return;
    }
    set->beginGroup( "prefsDialog" );
    restoreGeometry( set->value( "geometry" ).toByteArray() );
    set->endGroup();
}

void PrefsDialog::writeSettings(){
    QSettings *set = Prefs::settings();
    set->beginGroup( "prefsDialog" );
    if( _used )
        set->setValue( "geometry",saveGeometry() );
    set->endGroup();
}

int PrefsDialog::exec(){

    if( !_used ){
        QRect frect = frameGeometry();
        frect.moveCenter(QDesktopWidget().availableGeometry().center());
        move(frect.topLeft());
        restoreGeometry( saveGeometry() );
        _used = true;
    }

    _ui->fontComboBox->setCurrentFont( QFont( _prefs->getString( "fontFamily" ),_prefs->getInt("fontSize") ) );
    _ui->fontSizeWidget->setValue( _prefs->getInt("fontSize") );
    _ui->tabSizeWidget->setValue( _prefs->getInt( "tabSize" ) );
    _ui->chbSmoothFonts->setChecked( _prefs->getBool( "smoothFonts" ) );
    _ui->chbHighlightLine->setChecked( _prefs->getBool( "highlightLine" ) );
    _ui->chbHighlightWord->setChecked( _prefs->getBool( "highlightWord" ) );

    _ui->backgroundColorWidget->setColor( _prefs->getColor( "backgroundColor" ) );
    _ui->defaultColorWidget->setColor( _prefs->getColor( "defaultColor" ) );
    _ui->numbersColorWidget->setColor( _prefs->getColor( "numbersColor" ) );
    _ui->stringsColorWidget->setColor( _prefs->getColor( "stringsColor" ) );
    _ui->identifiersColorWidget->setColor( _prefs->getColor( "identifiersColor" ) );
    _ui->keywordsColorWidget->setColor( _prefs->getColor( "keywordsColor" ) );
    _ui->commentsColorWidget->setColor( _prefs->getColor( "commentsColor" ) );
    _ui->highlightColorWidget->setColor( _prefs->getColor( "highlightColor" ) );
    _ui->tabSizeWidget->setValue( _prefs->getInt( "tabSize" ) );

    _ui->monkeyPathWidget->setText( _prefs->getString( "monkeyPath" ) );

    _ui->checkBoxAutoformat->setChecked( _prefs->getBool( "codeAutoformat" ) );
    _ui->checkBoxUseSpaces->setChecked( _prefs->getBool( "codeTabUseSpaces" ) );
    _ui->checkBoxFillAucompInherit->setChecked( _prefs->getBool( "codeFillAucompInherit" ) );

    _ui->checkBoxCheckUpdates->setChecked( _prefs->getBool( "updates" ) );

    _ui->groupBox->setVisible(false);

    QDialog::show();
    return QDialog::exec();
}

void PrefsDialog::onCheckUpdatesChanged(){
    _prefs->setValue( "updates", _ui->checkBoxCheckUpdates->isChecked() );
}

void PrefsDialog::onFontChanged( const QFont &font ){
    _prefs->setValue( "fontFamily",font.family() );
}

void PrefsDialog::onFontSizeChanged( int size ){
    _prefs->setValue( "fontSize",size );
}

void PrefsDialog::onTabSizeChanged( int size ){
    _prefs->setValue( "tabSize",size );
}

void PrefsDialog::onSmoothFontsChanged( bool state ){
    _prefs->setValue( "smoothFonts",state );
}

void PrefsDialog::onHighlightLineChanged(bool value) {
    _prefs->setValue( "highlightLine", value );
}

void PrefsDialog::onHighlightWordChanged(bool value) {
    _prefs->setValue( "highlightWord", value );
}

void PrefsDialog::onColorChanged(){

    ColorSwatch *swatch=qobject_cast<ColorSwatch*>( sender() );
    if( !swatch ) return;

    QString name=swatch->objectName();
    int i=name.indexOf( "Widget" );
    if( i==-1 ) return;
    name=name.left( i );

    _prefs->setValue( name.toStdString().c_str(),swatch->color() );
}

void PrefsDialog::onAnalyzerChanged() {
    if(sender() == _ui->checkBoxAutoformat)
        _prefs->setValue( "codeAutoformat",_ui->checkBoxAutoformat->isChecked() );
    else if(sender() == _ui->checkBoxFillAucompInherit)
        _prefs->setValue( "codeFillAucompInherit",_ui->checkBoxFillAucompInherit->isChecked() );
    else if(sender() == _ui->checkBoxUseSpaces)
        _prefs->setValue( "codeTabUseSpaces",_ui->checkBoxUseSpaces->isChecked() );
}

void PrefsDialog::onBrowseForPath() {
    QString path = QFileDialog::getExistingDirectory( this,"Select Monkey directory","",QFileDialog::ShowDirsOnly|QFileDialog::DontResolveSymlinks );
    if( path.isEmpty() ) return;
    path = fixPath( path );
    QString trans;
    if( MainWindow::isValidMonkeyPath(path, trans) ) {
        _prefs->setValue( "monkeyPath",path );
        _prefs->setValue( "transPath",trans );
        _ui->monkeyPathWidget->setText( path );
    }
    else {
        QMessageBox::warning( this,"","Invalid Monkey Path!\nPlease, choose the root directory of Monkey." );
        _prefs->setValue( "monkeyPath","" );
    }
}

