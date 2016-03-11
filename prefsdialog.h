/*
Ted, a simple text editor/IDE.

Copyright 2012, Blitz Research Ltd.

See LICENSE.TXT for licensing terms.
*/

#ifndef PREFSDIALOG_H
#define PREFSDIALOG_H

#include "std.h"

namespace Ui {
class PrefsDialog;
}

class Prefs;
class MainWindow;

class PrefsDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit PrefsDialog( MainWindow *mainwnd, QWidget *parent=0 );
    ~PrefsDialog();

    void readSettings();
    void writeSettings();

    int exec();

public slots:
    void onHighlightLineChanged(bool value);
    void onHighlightWordChanged(bool value);
    void onFontChanged( const QFont &font );
    void onFontSizeChanged( int size );
    void onTabSizeChanged( int size );
    void onSmoothFontsChanged( bool state );
    void onColorChanged();
    void onBrowseForPath();
    void onAnalyzerChanged();
    void onCheckUpdatesChanged();
    void onThemeChanged(int index);
    void onShowHelpInDockChanged();
    void onReplaceDocsStyleChanged();

private slots:
    void on_checkBoxAutoBracket_stateChanged(int arg1);

    void on_pushButtonOpenSettingsFolder_clicked();


    void on_spinBoxTypedCharsForCompletion_valueChanged(int arg1);

private:
    MainWindow *_mainwnd;
    Ui::PrefsDialog *_ui;
    Prefs *_prefs;
    bool _used;
};

#endif // PREFSDIALOG_H
