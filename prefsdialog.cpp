/*
Ted, a simple text editor/IDE.

Copyright 2012, Blitz Research Ltd.

See LICENSE.TXT for licensing terms.
*/

#include "prefsdialog.h"
#include "ui_prefsdialog.h"

#include "prefs.h"
#include "mainwindow.h"
#include "theme.h"

PrefsDialog::PrefsDialog(MainWindow *mainwnd, QWidget *parent ):QDialog( parent ),_ui( new Ui::PrefsDialog ),_prefs( Prefs::prefs() ),_used( false ){
    _ui->setupUi( this );
    _mainwnd = mainwnd;
    _isEnableThemeSignal = false;
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

void PrefsDialog::openPathSection()
{
    _ui->toolBox->setCurrentIndex(3);
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


    _ui->constantsColorWidget->setColor( _prefs->getColor( "constantsColor" ) );
    _ui->funcDeclsColorWidget->setColor( _prefs->getColor( "funcDeclsColor" ) );


    _ui->commentsColorWidget->setColor( _prefs->getColor( "commentsColor" ) );
    _ui->highlightColorWidget->setColor( _prefs->getColor( "highlightColor" ) );


    _ui->highlightColorErrorWidget->setColor( _prefs->getColor( "highlightColorError" ) );
    _ui->highlightColorCaretRowWidget->setColor( _prefs->getColor( "highlightColorCaretRow" ) );
    _ui->monkeywordsColorWidget->setColor( _prefs->getColor( "monkeywordsColor" ) );
    _ui->userwordsColorWidget->setColor( _prefs->getColor( "userwordsColor" ) );


    _ui->userwordsDeclColorWidget->setColor( _prefs->getColor( "userwordsDeclColor" ) );
    _ui->userwordsVarColorWidget->setColor( _prefs->getColor( "userwordsVarColor" ) );
    _ui->paramsColorWidget->setColor( _prefs->getColor( "paramsColor" ) );

    _ui->tabSizeWidget->setValue( _prefs->getInt( "tabSize" ) );

    _ui->monkeyPathWidget->setText( _prefs->getString( "monkeyPath" ) );

    _ui->checkBoxAutoformat->setChecked( _prefs->getBool( "codeAutoformat" ) );
    _ui->checkBoxUseSpaces->setChecked( _prefs->getBool( "codeTabUseSpaces" ) );
    _ui->checkBoxFillAucompInherit->setChecked( _prefs->getBool( "codeFillAucompInherit" ) );

    _ui->checkBoxCheckUpdates->setChecked( _prefs->getBool( "updates" ) );
    _ui->checkBoxAutoBracket->setChecked(_prefs->getBool("AutoBracket"));
    _ui->checkBoxShowHelpInDock->setChecked(_prefs->getBool("showHelpInDock"));
    _ui->checkBoxReplaceDocsStyle->setChecked(_prefs->getBool("replaceDocsStyle"));

    _ui->spinBoxTypedCharsForCompletion->setValue(_prefs->getInt("CharsForCompletion"));
    _ui->checkBoxAddVoidForMethods->setChecked(_prefs->getBool("addVoidForMethods"));

    Prefs *p = Prefs::prefs();
    _ui->labelSettingsFile->setText( p->settings()->fileName() );

    // themes
    _isEnableThemeSignal = false;
    QStringList lst = Theme::allThemes();
    if (_ui->comboBoxTheme->count() == 0) {
        _ui->comboBoxTheme->addItems(lst);
    }
    QString theme = p->getString("theme");
    int index = lst.indexOf(theme);
    _ui->comboBoxTheme->setCurrentIndex(index);
    _isEnableThemeSignal = true;

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

void PrefsDialog::onShowHelpInDockChanged() {
    _prefs->setValue("showHelpInDock", _ui->checkBoxShowHelpInDock->isChecked());
}

void PrefsDialog::onReplaceDocsStyleChanged() {
    _prefs->setValue("replaceDocsStyle", _ui->checkBoxReplaceDocsStyle->isChecked());
}

void PrefsDialog::on_checkBoxAutoBracket_stateChanged(int arg1)
{
    _prefs->setValue( "AutoBracket", _ui->checkBoxAutoBracket->isChecked() );
}

void PrefsDialog::on_pushButtonOpenSettingsFolder_clicked()
{
    QString path = extractDir(_ui->labelSettingsFile->text());
    QUrl url("file:///"+path, QUrl::TolerantMode);
    QDesktopServices::openUrl(url);
}

void PrefsDialog::on_spinBoxTypedCharsForCompletion_valueChanged(int arg1)
{
    _prefs->setValue( "CharsForCompletion", arg1 );
}

void PrefsDialog::on_checkBoxAddVoidForMethods_clicked()
{
    _prefs->setValue( "addVoidForMethods", _ui->checkBoxAddVoidForMethods->isChecked() );
}

void PrefsDialog::on_comboBoxTheme_currentIndexChanged(const QString &arg1)
{
    if (!_isEnableThemeSignal)
        return;
    _mainwnd->updateTheme(arg1);
}
