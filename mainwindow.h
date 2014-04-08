/*
Ted, a simple text editor/IDE.

Copyright 2012, Blitz Research Ltd.

See LICENSE.TXT for licensing terms.
*/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "std.h"

class CodeEditor;
class ProjectTreeModel;
class DebugTreeModel;
class FindDialog;
class Process;
class FindInFilesDialog;
class QuickHelp;


namespace Ui {
class MainWindow;
}

class Prefs;
class PrefsDialog;
class QuickHelp;
class TabWidgetDrop;


class MainWindow : public QMainWindow{
    Q_OBJECT
public:

    MainWindow( QWidget *parent=0 );
    ~MainWindow();

    void cdebug( const QString &str );
    static bool isValidMonkeyPath( const QString &path );

private:

    void parseAppArgs();
    void initKeywords();
    void loadStyles(bool load);

    bool isBuildable( CodeEditor *editor );
    QString widgetPath( QWidget *widget );
    CodeEditor *editorWithPath( const QString &path );

    QWidget *newFile( const QString &path );
    QWidget *openFile( const QString &path,bool addToRecent );
    bool saveFile( QWidget *widget,const QString &path );
    bool closeFile( QWidget *widget,bool remove=true );


    bool isValidBlitzmaxPath( const QString &path );
    QString defaultMonkeyPath();
    void enumTargets();

    void readSettings();
    void writeSettings();

    void updateWindowTitle();
    void updateTabLabel( QWidget *widget );
    void updateActions();

    void print( const QString &str );
    void runCommand( QString cmd,QWidget *fileWidget );
    void build( QString mode );

    bool confirmQuit();
    void closeEvent( QCloseEvent *event );


protected:

    void showEvent(QShowEvent * event);


public slots:

    //File menu
    void onFileNew();
    void onFileOpen();
    void onFileOpenRecent();
    void onFileClose();
    void onFileCloseAll();
    void onFileCloseOthers();
    void onFileSave();
    void onFileSaveAs();
    void onFileSaveAll();
    void onFileNext();
    void onFilePrevious();
    void onFilePrefs();
    void onFileQuit();

    //Edit menu
    void onEditUndo();
    void onEditRedo();
    void onEditCut();
    void onEditCopy();
    void onEditPaste();
    void onEditDelete();
    void onEditSelectAll();
    void onEditFind();
    void onEditFindNext();
    void onEditFindPrev();
    void onEditReplace();
    void onEditReplaceAll();
    void onFindReplace( int how );
    void onEditGoto();
    void onEditFindInFiles();

    //View menu
    void onViewToolBar();
    void onViewWindow();
    //
    void onToggleBookmark();
    void onPreviousBookmark();
    void onNextBookmark();
    void onFoldAll();
    void onUnfoldAll();
    void onGoBack();
    void onGoForward();
    //
    //Build/Debug menu
    void onBuildBuild();
    void onBuildRun();
    void onBuildCheck();
    void onBuildUpdate();
    void onDebugStep();
    void onDebugStepInto();
    void onDebugStepOut();
    void onDebugKill();
    void onBuildTarget();
    void onBuildConfig();
    void onBuildLockFile();
    void onBuildUnlockFile();
    void onBuildAddProject();

    //Window menu
    void onSwitchFullscreen();

    //Help menu
    void onHelpHome();
    void onHelpBack();
    void onHelpForward();
    void onHelpQuickHelp();
    void onHelpAbout();
    void onHelpRebuild();
    void onHelpOnlineDocs();
    void onHelpMonkeyHomepage();

    void onOpenCodeFile(const QString &file, const QString &folder, const int &lineNumber );
    void onDropFiles( const QStringList &list );

    void onCodeAnalyze();
    void onClearOutput();
    void onCloseSearch();
    void onChangeAnalyzerProperties(bool toggle=false);

    void onTabsCloseTab();
    void onTabsCloseOtherTabs();
    void onTabsCloseAllTabs();

    void onDarkTheme();
    void onLightTheme();

private slots:

    void onShowHelp( const QString &text );

    void onLinkClicked( const QUrl &url );

    void onCloseMainTab( int index );
    void onMainTabChanged( int index );

    void onDockVisibilityChanged( bool visible );

    void onTabsMenu( const QPoint &pos );
    void onProjectMenu( const QPoint &pos );
    void onFileClicked( const QModelIndex &index );

    void onTextChanged();
    void onCursorPositionChanged();
    void onShowCode(const QString &path, int line , bool error=false);
    void onShowCode( const QString &path,int pos,int len );

    void onProcStdout();
    void onProcStderr();
    void onProcLineAvailable( int channel );
    void onProcFinished();

    void onStatusBarChanged(const QString &text);
    void onAutoformatAll();

private:



    Ui::MainWindow *_ui;

    QString _defaultDir;

    QString _blitzmaxPath;
    QString _buildBmxCmd;
    QString _runBmxCmd;

    QString _monkeyPath;
    QString _buildMonkeyCmd;
    QString _runMonkeyCmd;

    QString _transVersion;

    TabWidgetDrop *_mainTabWidget;

    Process *_consoleProc;

    ProjectTreeModel *_projectTreeModel;
    DebugTreeModel *_debugTreeModel;

    CodeEditor *_codeEditor;
    CodeEditor *_lockedEditor;
    QWebView *_helpWidget;

    PrefsDialog *_prefsDialog;
    FindDialog *_findDialog;
    FindInFilesDialog *_findInFilesDialog;

    QMenu *_tabsPopupMenu;
    QMenu *_projectPopupMenu;
    QMenu *_dirPopupMenu;
    QMenu *_filePopupMenu;

    QLabel *_statusWidget;

    QComboBox *_targetsWidget;
    QComboBox *_configsWidget;
    QComboBox *_indexWidget;

    QString _lastHelpIdent;
    int _lastHelpCursorPos;


};

#endif // MAINWINDOW_H
