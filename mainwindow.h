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
    static bool isValidMonkeyPath(const QString &path , QString &trans);
    void updateTheme();

private:

    void updateCodeViews(QTreeView *tree, QListView* list);

    void parseAppArgs();
    void initKeywords();


    bool isBuildable( CodeEditor *editor );
    QString widgetPath( QWidget *widget );
    CodeEditor *editorWithPath( const QString &path );

    QWidget *newFile( const QString &path );
    QWidget *openFile( const QString &path,bool addToRecent );
    bool saveFile( QWidget *widget,const QString &path );
    bool closeFile( QWidget *widget,bool remove=true );

    void enumTargets();

    void readSettings();
    void writeSettings();

    void updateWindowTitle();
    void updateTabLabel( QWidget *widget );
    void updateActions();

    void print(const QString &str , QString kind);
    void runCommand( QString cmd,QWidget *fileWidget );
    void build( QString mode,QString pathmonkey);

    bool confirmQuit();
    void closeEvent( QCloseEvent *event );


protected:

    void showEvent(QShowEvent * event);


public slots:

    void onStyleChanged(bool b);
    void onCodeTreeViewClicked( const QModelIndex &index );
    void onSourceListViewClicked( const QModelIndex &index );

    void onLowerUpperCase();
    void onNetworkFinished(QNetworkReply *reply);
    void onCheckForUpdates();
    void onCheckForUpdatesSilent();
    void onOpenUrl();
    void onKeyEscapePressed();

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
    void onCommentUncommentBlock();
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

    void onThemeAndroidStudio();
    void onThemeNetBeans();
    void onThemeQtCreator();
    void onFindUsages();

    void onLinkClicked( const QUrl &url );
    void onDocsZoomChanged(int);

private slots:

    void onShowHelp( const QString &text );

    void onCloseUsagesTab(int index);
    void onUsagesJumpToLine(QTreeWidgetItem *item, int column);
    void onCloseMainTab( int index );
    void onMainTabChanged( int index );

    void onDockVisibilityChanged( bool visible );

    void onEditorMenu(const QPoint &pos);
    void onTabsMenu( const QPoint &pos );
    void onProjectMenu( const QPoint &pos );
    void onFileClicked( const QModelIndex &index );
    void onSourceMenu( const QPoint &pos );
    void onUsagesMenu( const QPoint &pos );

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


    void onUsagesRename();
    void onUsagesSelectAll();
    void onUsagesUnselectAll();


    void on_actionLock_Target_triggered();

    void on_actionLock_Target_toggled(bool arg1);

    void on_webView_selectionChanged();

    void on_docsDockWidget_allowedAreasChanged(const Qt::DockWidgetAreas &allowedAreas);

    void on_actionClose_all_Tabs_triggered();

    void on_actionThemeMonokaiDarkSoda_triggered();

    void on_actionThemeLightTable_triggered();

private:


    QNetworkAccessManager *_networkManager;
    bool _showMsgbox;
    Ui::MainWindow *_ui;

    QString _defaultDir;

    QString _blitzmaxPath;
    QString _buildBmxCmd;
    QString _runBmxCmd;

    static QString _monkeyPath, _transPath;
    bool _isShowHelpInDock;
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
    //FindDialog *_findDialog;
    FindInFilesDialog *_findInFilesDialog;

    QMenu *_tabsPopupMenu;
    QMenu *_projectPopupMenu;
    QMenu *_dirPopupMenu;
    QMenu *_filePopupMenu;
    QMenu *_fileImagePopupMenu;
    QMenu *_fileMonkeyPopupMenu;
    QMenu *_sourcePopupMenu;
    QMenu *_editorPopupMenu;
    QMenu *_usagesPopupMenu;

    QLabel *_statusWidget;

    QComboBox *_targetsWidget;
    QComboBox *_configsWidget;
    QComboBox *_indexWidget;

    QString _lastHelpIdent;
    int _lastHelpCursorPos;


};

#endif // MAINWINDOW_H
