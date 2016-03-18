/*
Ted, a simple text editor/IDE.

Copyright 2012, Blitz Research Ltd.

See LICENSE.TXT for licensing terms.
*/

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "codeeditor.h"
#include "prefsdialog.h"
#include "previewhtml5.h"
#include "projecttreemodel.h"
#include "debugtreemodel.h"
#include "finddialog.h"
#include "prefs.h"
#include "proc.h"
#include "findinfilesdialog.h"
#include "quickhelp.h"
#include "codeanalyzer.h"
#include "tabwidgetdrop.h"
#include "theme.h"
#include "customcombobox.h"
#include <QHostInfo>
#include "addpropertydialog.h"
#include "saveonclosedialog.h"


#define SETTINGS_VERSION 2

#ifdef Q_OS_WIN
#define HOST QString("_winnt")
#elif defined( Q_OS_MAC )
#define HOST QString("_macos")
#elif defined( Q_OS_LINUX )
#define HOST QString("_linux")
#else
#define HOST QString("")
#endif

#define _QUOTE(X) #X
#define _STRINGIZE( X ) _QUOTE(X)

#define TED_VERSION "1.17"
#define APP_VERSION "1.4"
#define APP_NAME "Jentos.Code"


QString MainWindow::_monkeyPath;
QString MainWindow::_transPath;


static MainWindow *mainWindow;

void cdebug( const QString &q ){
    if( mainWindow ) mainWindow->cdebug( q );
}


//***** MainWindow *****
//
MainWindow::MainWindow(QWidget *parent) : QMainWindow( parent ),_ui( new Ui::MainWindow ){

    mainWindow = this;

    QTextCodec::setCodecForLocale( QTextCodec::codecForName( "UTF-8" ) );

#ifdef Q_OS_MAC
    QCoreApplication::instance()->setAttribute( Qt::AA_DontShowIconsInMenus );
#endif

    //Enables pdf viewing!
    QWebSettings::globalSettings()->setAttribute( QWebSettings::PluginsEnabled,true );

    QSettings::setDefaultFormat( QSettings::IniFormat );
    QCoreApplication::setOrganizationName( "FingerDev Studio" );
    QCoreApplication::setOrganizationDomain( "fingerdev.com" );
    QCoreApplication::setApplicationName( APP_NAME );

    _ui->setupUi( this );

    CodeAnalyzer::init();
    Theme::init();

    _isUpdaterQuiet = true;
    _networkManager = 0;
    _codeEditor = 0;
    _lockedEditor = 0;
    _helpWidget = 0;

    //docking options
    setCorner( Qt::TopLeftCorner,Qt::LeftDockWidgetArea );
    setCorner( Qt::BottomLeftCorner,Qt::LeftDockWidgetArea );
    setCorner( Qt::TopRightCorner,Qt::RightDockWidgetArea );
    setCorner( Qt::BottomRightCorner,Qt::RightDockWidgetArea );

    //status bar widget
    _statusWidget = new QLabel;
    statusBar()->addPermanentWidget( _statusWidget );

    //targets combobox
    _targetsWidget = new CustomComboBox;
    _targetsWidget->setMinimumWidth(100);
    _ui->buildToolBar->addWidget( _targetsWidget );
    _targetsWidget->setFocusPolicy(Qt::NoFocus);

    QWidget *w = new QWidget;
    w->setFixedWidth(3);
    _ui->buildToolBar->addWidget(w);

    _configsWidget = new CustomComboBox;
    _configsWidget->addItem( "Debug" );
    _configsWidget->addItem( "Release" );
    _ui->buildToolBar->addWidget( _configsWidget );
    _configsWidget->setFocusPolicy(Qt::NoFocus);

    //quick help search combo
    w = new QWidget;
    w->setFixedWidth(3);
    _ui->helpToolBar->addWidget(w);
    _indexWidget = new CustomComboBox;
    _indexWidget->setFixedWidth( 180 );
    _indexWidget->setEditable( true );
    _indexWidget->setInsertPolicy( QComboBox::NoInsert );
    _ui->helpToolBar->addWidget( _indexWidget );
    _indexWidget->setFocusPolicy(Qt::ClickFocus);

    //
    /*w = new QWidget;
    w->setFixedWidth(3);
    _ui->codeToolBar->addWidget(w);
    QProgressBar *_progressBar = new QProgressBar(_ui->codeToolBar);
    _progressBar->setFixedWidth(80);
    _progressBar->setMaximum(100);
    _progressBar->setValue(40);
    _progressBar->setTextVisible(false);
    _ui->codeToolBar->addWidget(_progressBar);*/

    //init central tab widget
    _mainTabWidget = new TabWidgetDrop;
    _mainTabWidget->setMovable( true );
    _mainTabWidget->setTabsClosable( true );
    _mainTabWidget->setAcceptDrops(true);

    QGridLayout *lay = new QGridLayout(_ui->widget);
    lay->setSpacing(0);
    lay->setMargin(1);
    //_ui->widget->layout()
    _ui->widget->setLayout(lay);
    _ui->widget->layout()->addWidget(_mainTabWidget);

    _mainTabWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect( _mainTabWidget,SIGNAL(currentChanged(int)),SLOT(onMainTabChanged(int)) );
    connect( _mainTabWidget,SIGNAL(tabCloseRequested(int)),SLOT(onCloseMainTab(int)) );
    connect( _mainTabWidget,SIGNAL(dropFiles(QStringList)),SLOT(onDropFiles(QStringList)) );
    connect( _mainTabWidget,SIGNAL(customContextMenuRequested(const QPoint&)),SLOT(onTabsMenu(const QPoint&)) );

    //init console widgets
    _consoleProc=0;

    //init browser widgets
    _projectTreeModel=new ProjectTreeModel;

    _ui->projectTreeView->setModel( _projectTreeModel );
    _ui->projectTreeView->hideColumn( 1 );
    _ui->projectTreeView->hideColumn( 2 );
    _ui->projectTreeView->hideColumn( 3 );
    _ui->projectTreeView->setContextMenuPolicy( Qt::CustomContextMenu );
    connect( _ui->projectTreeView,SIGNAL(doubleClicked(const QModelIndex&)),SLOT(onFileClicked(const QModelIndex&)) );
    connect( _ui->projectTreeView,SIGNAL(customContextMenuRequested(const QPoint&)),SLOT(onProjectMenu(const QPoint&)) );

    _ui->codeTreeView->hideColumn( 1 );
    _ui->codeTreeView->hideColumn( 2 );
    _ui->codeTreeView->hideColumn( 3 );

    _debugTreeModel=0;

    connect( _ui->projectDockWidget,SIGNAL(visibilityChanged(bool)),SLOT(onDockVisibilityChanged(bool)) );
    connect( _ui->sourceDockWidget,SIGNAL(visibilityChanged(bool)),SLOT(onDockVisibilityChanged(bool)) );
    connect( _ui->codeTreeDockWidget,SIGNAL(visibilityChanged(bool)),SLOT(onDockVisibilityChanged(bool)) );
    connect( _ui->debugDockWidget,SIGNAL(visibilityChanged(bool)),SLOT(onDockVisibilityChanged(bool)) );
    connect( _ui->outputDockWidget,SIGNAL(visibilityChanged(bool)),SLOT(onDockVisibilityChanged(bool)) );
    connect( _ui->usagesDockWidget,SIGNAL(visibilityChanged(bool)),SLOT(onDockVisibilityChanged(bool)) );
    connect( _ui->docsDockWidget,SIGNAL(visibilityChanged(bool)),SLOT(onDockVisibilityChanged(bool)) );

#ifdef Q_OS_WIN
    _ui->actionFileNext->setShortcut( QKeySequence( "Ctrl+Tab" ) );
    _ui->actionFilePrevious->setShortcut( QKeySequence( "Ctrl+Shift+Tab" ) );
#else
    _ui->actionFileNext->setShortcut( QKeySequence( "Meta+Tab" ) );
    _ui->actionFilePrevious->setShortcut( QKeySequence( "Meta+Shift+Tab" ) );
#endif

    _tabsPopupMenu = new QMenu;
    _tabsPopupMenu->addAction( _ui->actionClose_Tab );
    _tabsPopupMenu->addAction( _ui->actionClose_other_Tabs );
    _tabsPopupMenu->addAction( _ui->actionClose_all_Tabs );
    _tabsPopupMenu->addSeparator();
    _tabsPopupMenu->addAction( _ui->actionLock_Build_File );
    //_tabsPopupMenu->addAction( _ui->actionUnlock_Build_File );
    //_tabsPopupMenu->addSeparator();

    _projectPopupMenu=new QMenu;
    _projectPopupMenu->addAction( _ui->actionNewFile );
    _projectPopupMenu->addAction( _ui->actionNewFolder );
    _projectPopupMenu->addSeparator();
    _projectPopupMenu->addAction( _ui->actionEditFindInFiles );
    _projectPopupMenu->addSeparator();
    _projectPopupMenu->addAction( _ui->actionOpen_on_Desktop );
    _projectPopupMenu->addSeparator();
    _projectPopupMenu->addAction( _ui->actionCloseProject );

    _filePopupMenu=new QMenu;
    _filePopupMenu->addAction( _ui->actionOpen_on_Desktop );
    _filePopupMenu->addSeparator();
    _filePopupMenu->addAction( _ui->actionRenameFile );
    _filePopupMenu->addAction( _ui->actionDeleteFile );

    _fileImagePopupMenu=new QMenu;
    _fileImagePopupMenu->addAction( _ui->actionView_Image );
    _fileImagePopupMenu->addSeparator();
    _fileImagePopupMenu->addAction( _ui->actionEdit_Image );
    _fileImagePopupMenu->addSeparator();
    _fileImagePopupMenu->addAction( _ui->actionOpen_on_Desktop );
    _fileImagePopupMenu->addSeparator();
    _fileImagePopupMenu->addAction( _ui->actionRenameFile );
    _fileImagePopupMenu->addAction( _ui->actionDeleteFile );

    _fileMonkeyPopupMenu=new QMenu;
    _fileMonkeyPopupMenu->addAction( _ui->actionMonkeyBuild_and_Run );
    _fileMonkeyPopupMenu->addSeparator();
    _fileMonkeyPopupMenu->addAction( _ui->actionMonkeyBuild );
    _fileMonkeyPopupMenu->addSeparator();
    _fileMonkeyPopupMenu->addAction( _ui->actionOpen_on_Desktop );
    _fileMonkeyPopupMenu->addSeparator();
    _fileMonkeyPopupMenu->addAction( _ui->actionRenameFile );
    _fileMonkeyPopupMenu->addAction( _ui->actionDeleteFile );

    _dirPopupMenu=new QMenu;
    _dirPopupMenu->addAction( _ui->actionNewFile );
    _dirPopupMenu->addAction( _ui->actionNewFolder );
    _dirPopupMenu->addSeparator();
    _dirPopupMenu->addAction( _ui->actionEditFindInFiles );
    _dirPopupMenu->addSeparator();
    _dirPopupMenu->addAction( _ui->actionOpen_on_Desktop );
    _dirPopupMenu->addSeparator();
    _dirPopupMenu->addAction( _ui->actionRenameFile );
    _dirPopupMenu->addAction( _ui->actionDeleteFile );

    _editorPopupMenu = new QMenu;
    //_editorPopupMenu->addAction( _ui->actionBuildRun );
    //_editorPopupMenu->addSeparator();
    //_editorPopupMenu->addAction( _ui->actionBuildBuild );
    _editorPopupMenu->addAction( _ui->actionAddProperty );
    _editorPopupMenu->addSeparator();
    _editorPopupMenu->addAction( _ui->actionFind_Usages );
    _editorPopupMenu->addSeparator();
    QMenu *bm = new QMenu("Bookmarks",_editorPopupMenu);
    bm->addAction( _ui->actionToggleBookmark );
    bm->addAction( _ui->actionPreviousBookmark );
    bm->addAction( _ui->actionNextBookmark );
    _editorPopupMenu->addMenu(bm);
    _editorPopupMenu->addSeparator();
    _editorPopupMenu->addAction( _ui->actionEditCut );
    _editorPopupMenu->addAction( _ui->actionEditCopy );
    _editorPopupMenu->addAction( _ui->actionEditPaste );

    _usagesPopupMenu = new QMenu;
    _usagesPopupMenu->addAction(_ui->actionUsagesSelectAll);
    _usagesPopupMenu->addAction(_ui->actionUsagesUnselectAll);

    connect( _ui->actionFileQuit,SIGNAL(triggered()),SLOT(onFileQuit()) );

    readSettings();

    _prefsDialog = new PrefsDialog( this, this );
    _prefsDialog->readSettings();

    _findInFilesDialog=new FindInFilesDialog( 0 );
    _findInFilesDialog->readSettings();
    connect( _findInFilesDialog,SIGNAL(showCode(QString,int,int)),SLOT(onShowCode(QString,int,int)) );

    parseAppArgs();

    updateWindowTitle();

    updateActions();

    statusBar()->showMessage( "Ready." );

    _ui->frameSearch->setVisible(false);

    connect( Prefs::prefs(), SIGNAL(prefsChanged(const QString&)), CodeAnalyzer::instance(), SLOT(onPrefsChanged(const QString&)) );

    _ui->toolButtonShowVariables->setChecked(CodeAnalyzer::isShowVariables());
    _ui->toolButtonShowInherited->setChecked(CodeAnalyzer::isShowInherited());
    _ui->toolButtonSortByName->setChecked(CodeAnalyzer::isSortByName());
    onChangeAnalyzerProperties(false);

    updateCodeViews(_ui->codeTreeView, _ui->sourceListView);

    _ui->usagesTabWidget->setContextMenuPolicy( Qt::CustomContextMenu );
    connect( _ui->usagesTabWidget,SIGNAL(customContextMenuRequested(const QPoint&)),SLOT(onUsagesMenu(const QPoint&)) );
    connect( _ui->usagesTabWidget,SIGNAL(tabCloseRequested(int)),SLOT(onCloseUsagesTab(int)) );
    _ui->frameUsagesRename->hide();

    // some checkers here
    QString trans;
    bool isValid = isValidMonkeyPath(_monkeyPath, trans);

    if (isValid) {
        initKeywords();
    }

    QSettings *set = Prefs::settings();
    int n = set->beginReadArray( "openDocuments" );
    QStringList list;
    for (int i = 0; i < n; ++i ) {
        set->setArrayIndex(i);
        QString path = fixPath( set->value( "path" ).toString() );
        list.append(path);
    }
    set->endArray();

    foreach (QString s, list) {
        if (isUrl(s)) {
            openFile( s,false );
        } else if (QFile::exists(s)) {
            CodeAnalyzer::flushFileModified(s);
            openFile( s,false );
        }
    }

    Prefs *p = Prefs::prefs();
    if (p->getBool("updates")) {
        QTimer::singleShot(5000, this, SLOT(onCheckForUpdatesQuiet()));
    }
    p->isValidMonkeyPath = isValid;

}

MainWindow::~MainWindow(){

    delete _ui;
    delete _tabsPopupMenu;
    delete _projectPopupMenu;
    delete _filePopupMenu;
    delete _fileImagePopupMenu;
    delete _fileMonkeyPopupMenu;
    delete _dirPopupMenu;
    delete _editorPopupMenu;
    delete _usagesPopupMenu;

    CodeAnalyzer::finalize();

}

void MainWindow::onStyleChanged(bool b) {
    /*qDebug()<<"style changed";
    QList<QAction*> list = _ui->menuStyles->actions();
    QString style = sender()->objectName();
    Prefs *p = Prefs::prefs();
    p->setValue("style", style);
    foreach (QAction *a, list) {
        a->setChecked( a->text() == style );
    }
    qApp->setStyle(QStyleFactory::create(style));*/
}

void MainWindow::showEvent(QShowEvent *event) {
    QMainWindow::showEvent(event);
}

//***** private methods *****
void MainWindow::updateTheme(const QString &name) {

    if (!name.isEmpty()) {
        Theme::set(name);
    }

    QString css = "";
    if (Theme::isDark()) {
        QFile f(":/txt/android.css");
        if(f.open(QFile::ReadOnly)) {
            css = f.readAll();
        }
        f.close();
    }

    css += "QDockWidget::title{text-align:center;}";
    qApp->setStyleSheet(css);
    QApplication::processEvents();
    //
    _ui->actionThemeAndroidStudio->setChecked( Theme::isCurrent(Theme::ANDROID_STUDIO) );
    _ui->actionThemeNetBeans->setChecked( Theme::isCurrent(Theme::NETBEANS) );
    _ui->actionThemeQt->setChecked( Theme::isCurrent(Theme::QT_CREATOR) );
    _ui->actionThemeMonokaiDarkSoda->setChecked( Theme::isCurrent(Theme::DARK_SODA) );
    _ui->actionThemeLightTable->setChecked( Theme::isCurrent(Theme::LIGHT_TABLE) );


    //update all icons
    //
    _ui->actionNew->setIcon(Theme::icon("New.png"));

    _ui->actionOpen->setIcon(Theme::icon("Open.png"));
    _ui->actionOpenProject->setIcon(Theme::icon("Project.png"));
    _ui->actionSave->setIcon(Theme::icon("Save.png"));
    _ui->actionClose->setIcon(Theme::icon("Close.png"));
    _ui->actionPrefs->setIcon(Theme::icon("Options.png"));
    //
    _ui->actionBuildBuild->setIcon(Theme::icon("Build.png"));
    _ui->actionBuildRun->setIcon(Theme::icon("Build-Run.png"));
    //
    _ui->actionStep->setIcon(Theme::icon("Step.png"));
    _ui->actionStep_In->setIcon(Theme::icon("Step-In.png"));
    _ui->actionStep_Out->setIcon(Theme::icon("Step-Out.png"));
    _ui->actionKill->setIcon(Theme::icon("Stop.png"));
    //
    _ui->actionEditFind->setIcon(Theme::icon("Find.png"));
    _ui->actionEditCut->setIcon(Theme::icon("Cut.png"));
    _ui->actionEditCopy->setIcon(Theme::icon("Copy.png"));
    _ui->actionEditPaste->setIcon(Theme::icon("Paste.png"));
    _ui->actionHelpHome->setIcon(Theme::icon("Home.png"));
    _ui->actionGoBack->setIcon(Theme::icon("Back.png"));
    _ui->actionGoForward->setIcon(Theme::icon("Forward.png"));
    //
    _ui->actionToggleBookmark->setIcon(Theme::icon("Bookmark.png"));
    _ui->actionFoldAll->setIcon(Theme::icon("Fold.png"));
    _ui->actionUnfoldAll->setIcon(Theme::icon("Unfold.png"));
    //
    //_ui->toolButtonShowVariables->setIcon(Theme::icon("ShowVars.png"));
    //_ui->toolButtonSortByName->setIcon(Theme::icon("Sort.png"));
    //_ui->toolButtonClearOutput->setIcon(Theme::icon("Clear.png"));
    _ui->toolButtonCloseSearch->setIcon(Theme::icon("CloseSearch.png"));
    //
    //_ui->actionCodeAnalyze->setIcon(Theme::icon("Options.png"));

    /*_ui->action->setIcon(QIcon(":/icons/"+theme+"/.png"));
    _ui->action->setIcon(QIcon(":/icons/"+theme+"/.png"));
    _ui->action->setIcon(QIcon(":/icons/"+theme+"/.png"));*/

    //qDebug() << "update theme: "+Theme::theme();
    Prefs *p = Prefs::prefs();
    if( !_monkeyPath.isEmpty() && p->getBool("replaceDocsStyle") ) {
        //qDebug()<<"replace docs style";
        QString jent = QApplication::applicationDirPath() + "/pagestyle.css";
        QString monk = _monkeyPath+"/docs/html/pagestyle.css";
        if(!QFile::exists(jent)) {
            QFile::copy(monk,jent);
        }

        jent = QApplication::applicationDirPath() + (Theme::isDark() ? "/help_dark.css" : "/pagestyle.css");
        if(QFile::exists(jent)) {
            bool b = QFile::remove(monk);
            //qDebug()<<"remove:"<<b<<monk;
            b = QFile::copy(jent,monk);
            //qDebug()<<"copy dark help:"<<b<<jent;
        }
    }
}

void MainWindow::initKeywords(){
    //qDebug() << "initKeywords()";

    CodeAnalyzer::loadTemplates(QApplication::applicationDirPath()+"/templates.txt");
    CodeAnalyzer::loadKeywords(QApplication::applicationDirPath()+"/keywords.txt");
    CodeAnalyzer::clearMonkey();
    QStringList exclude;
    exclude << "trans";
    CodeAnalyzer::analyzeDir(_monkeyPath+"/modules/", exclude);

    //CodeAnalyzer::print();

    QuickHelp::init(_monkeyPath);//load keywords

    _indexWidget->disconnect();
    _indexWidget->clear();

    QMap<QString, QuickHelp*>::iterator i;
    for( i = QuickHelp::map()->begin() ; i != QuickHelp::map()->end() ; ++i ){
        if( i.value()->isGlobal || i.value()->isKeyword ) {
            _indexWidget->addItem( i.value()->topic );
            //Highlighter::keywordsAdd( i.value()->topic );
        }
    }

    connect( _indexWidget,SIGNAL(currentIndexChanged(QString)),SLOT(onShowHelp(QString)) );

}

void MainWindow::parseAppArgs(){
    QStringList args=QApplication::arguments();
    for( int i=1;i<args.size();++i ){
        QString arg=fixPath( args.at(i) );
        if( QFile::exists( arg ) ){
            openFile( arg,true );
        }
    }
}

bool MainWindow::isBuildable( CodeEditor *editor ){
    if( !editor ) return false;
    if( editor->fileType()=="monkey" ) return !_monkeyPath.isEmpty();
    //if( editor->fileType()=="bmx" ) return !_blitzmaxPath.isEmpty();
    return false;
}

QString MainWindow::widgetPath( QWidget *widget ){
    if( CodeEditor *editor=qobject_cast<CodeEditor*>( widget ) ){
       return editor->path();
    }else if( QWebView *webView=qobject_cast<QWebView*>( widget ) ){
        return webView->url().toString();
    }
    return "";
}

CodeEditor *MainWindow::editorWithPath( const QString &path ){
    for( int i=0;i<_mainTabWidget->count();++i ){
       if( CodeEditor *editor=qobject_cast<CodeEditor*>( _mainTabWidget->widget( i ) ) ){

            if( editor->path()==path ) return editor;
        }
    }
    return 0;
}

QWidget *MainWindow::newFile( const QString &cpath ){

    QString path=cpath;

    if( path.isEmpty() ){

        path=fixPath( QFileDialog::getSaveFileName( this,"New File",_defaultDir,"Source Files (*.monkey)" ) );
        if( path.isEmpty() ) return 0;
    }

    QFile file( path );
    if( !file.open( QIODevice::WriteOnly | QIODevice::Truncate ) ){
        QMessageBox::warning( this,"New file","Failed to create new file: "+path );
        return 0;
    }
    file.close();

    return openFile( path,true );
}

void MainWindow::onOpenCodeFile(const QString &file, const QString &folder , const int &lineNumber) {
    QWidget *w = openFile(file, false);
    if( CodeEditor *editor = qobject_cast<CodeEditor*>(w) ) {
        editor->highlightLine(lineNumber);
    }
}

void MainWindow::onDropFiles( const QStringList &list ) {
    foreach (QString file, list) {
        openFile(file, false);
    }
}

void MainWindow::onCodeAnalyze() {
    if(_codeEditor)
        _codeEditor->analyzeCode();
}

void MainWindow::onClearOutput() {
    _ui->outputTextEdit->clear();
}

void MainWindow::onCloseSearch() {
    _ui->frameSearch->setVisible(false);
}

void MainWindow::onFindUsages() {
    //qDebug()<<"find usages";
    if(!_codeEditor || !_codeEditor->canFindUsages())
        return;
    QWidget *w = new QWidget(_ui->usagesTabWidget);
    QTreeWidget *tree = new QTreeWidget(w);
    tree->setHeaderHidden(true);

    QString s = _codeEditor->findUsages(tree);

    QHBoxLayout *lay = new QHBoxLayout(w);
    lay->setSpacing(0);
    lay->setMargin(0);
    lay->addWidget(tree);
    w->setLayout(lay);

    _ui->usagesTabWidget->addTab(w,s);
    _ui->usagesTabWidget->setCurrentWidget(w);
    tree->expandAll();
    _ui->usagesDockWidget->setVisible(true);
    connect(tree,SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)),SLOT(onUsagesJumpToLine(QTreeWidgetItem*,int)));
    _ui->frameUsagesRename->show();
}

void MainWindow::onCloseUsagesTab(int index) {
    //qDebug()<<"close tab:"<<index;
    QWidget *w = _ui->usagesTabWidget->widget(index);
    _ui->usagesTabWidget->removeTab(index);
    delete w;
    if(_ui->usagesTabWidget->count() == 0) {
        _ui->frameUsagesRename->hide();
        UsagesResult::clear();
    }
}

void MainWindow::onUsagesJumpToLine(QTreeWidgetItem *item, int column) {
    UsagesResult *u = UsagesResult::item(item);
    if(u) {
        onShowCode(u->path, u->blockNumber);
        if(_codeEditor) {
            QTextCursor c = _codeEditor->textCursor();
            c.setPosition(u->positionStart);
            c.setPosition(u->positionEnd, QTextCursor::KeepAnchor);
            _codeEditor->setTextCursor(c);
        }
    }
}

void MainWindow::onTabsCloseTab() {
    _mainTabWidget->tabCloseRequested(_mainTabWidget->currentIndex());
    //qDebug()<<"close tab";
}

void MainWindow::onTabsCloseOtherTabs() {
    QWidget *cur = _mainTabWidget->currentWidget();
    for(int k = 0; k < _mainTabWidget->count(); ++k) {
        if(_mainTabWidget->widget(k) != cur) {
            _mainTabWidget->tabCloseRequested(k);
            --k;
        }
    }
}

void MainWindow::onTabsCloseAllTabs() {
    for(int k = 0; k < _mainTabWidget->count(); ++k) {
        _mainTabWidget->tabCloseRequested(k);
    }
}

QWidget *MainWindow::openFile( const QString &cpath,bool addToRecent ){

    if( isUrl( cpath ) ){
        if(_isShowHelpInDock) {
            _ui->webView->page()->setLinkDelegationPolicy( QWebPage::DelegateAllLinks );
            _ui->webView->setUrl( cpath );
            _ui->docsDockWidget->setVisible(true);
            return 0;
        }
        else {
            QWebView *webView = 0;
            for( int i = 0; i < _mainTabWidget->count(); ++i ){
                if( webView = qobject_cast<QWebView*>( _mainTabWidget->widget( i ) ) ) break;
            }
            if( !webView ){
                webView = new QWebView;
                webView->page()->setLinkDelegationPolicy( QWebPage::DelegateAllLinks );
                connect( webView,SIGNAL(linkClicked(QUrl)),SLOT(onLinkClicked(QUrl)) );
                _mainTabWidget->addTab( webView,"Help" );
            }

            webView->setUrl( cpath );

            if( webView != _mainTabWidget->currentWidget() ){
                _mainTabWidget->setCurrentWidget( webView );
            }else{
                updateWindowTitle();
            }
            return webView;
        }
    }

    QString path=cpath;

    if( path.isEmpty() ){

        path=fixPath( QFileDialog::getOpenFileName( this,"Open File",_defaultDir,"Source Files (*.monkey *.cpp *.cs *.js *.as *.java);;Image Files(*.jpg *.png *.bmp);;All Files(*.*)" ) );
        if( path.isEmpty() ) return 0;

        _defaultDir=extractDir( path );
    }

    CodeEditor *editor=editorWithPath( path );
    if( editor ){
        //if file is already opened - not need to analyze
        CodeAnalyzer::disable();
        _mainTabWidget->setCurrentWidget( editor );
        CodeAnalyzer::enable();
        return editor;
    }
    editor = new CodeEditor;
    if( !editor->open( path ) ){
        delete editor;
        QMessageBox::warning( this,"Open File Error","Error opening file: "+path );
        return 0;
    }

    //qDebug()<<"work1";

    connect( editor,SIGNAL(keyEscapePressed()),SLOT(onKeyEscapePressed()) );
    connect( editor,SIGNAL(textChanged()),SLOT(onTextChanged()) );
    connect( editor,SIGNAL(cursorPositionChanged()),SLOT(onCursorPositionChanged()) );
    connect( editor,SIGNAL(statusBarChanged(const QString&)),SLOT(onStatusBarChanged(const QString&)) );
    connect( editor,SIGNAL(openCodeFile(const QString&,const QString&,const int&)),SLOT(onOpenCodeFile(const QString&,const QString&,const int&)));

    editor->setContextMenuPolicy(Qt::CustomContextMenu);
    connect( editor,SIGNAL(customContextMenuRequested(const QPoint&)),SLOT(onEditorMenu(const QPoint&)) );
    QStringList filemonkeynamelist = path.split('/');
    QString filemonkeyname = filemonkeynamelist.value(filemonkeynamelist.length()-1);
    QString filemonkeytype = ( filemonkeyname.right(3) ).toLower();

    bool fileinvalidbool = false;
    /*int fileinvalidsnum = 4;
    QString fileinvalids[fileinvalidsnum] = {"png","ico","jpg","gif"};
    for(int a=0; a<fileinvalidsnum;a++){

        if( filemonkeytype.endsWith(fileinvalids[a])){
              fileinvalidbool = true;
             //qDebug() << "archivo no valido";
              a=fileinvalidsnum;
        }
    }*/
    if(!fileinvalidbool){
        _mainTabWidget->addTab( editor,stripDir( path ) );
        _mainTabWidget->setCurrentWidget( editor );
    }

    //qDebug()<<"work3";

    if( addToRecent ){
        QMenu *menu=_ui->menuRecent_Files;
        QList<QAction*> actions=menu->actions();
        bool found=false;
        for( int i=0;i<actions.size();++i ){
            if( actions[i]->text()==path ){
                found=true;
                break;
            }
        }
        if( !found ){
            for( int i=19;i<actions.size();++i ){
                menu->removeAction( actions[i] );
            }
            QAction *action=new QAction( path,menu );
            if( actions.size() ){
                menu->insertAction( actions[0],action );
            }else{
                menu->addAction( action );
            }
            connect( action,SIGNAL(triggered()),this,SLOT(onFileOpenRecent()) );
        }
    }

    onCodeAnalyze();

    return editor;
}

bool MainWindow::saveFile( QWidget *widget,const QString &cpath ){

    QString path=cpath;

    CodeEditor *editor=qobject_cast<CodeEditor*>( widget );
    if( !editor ) return true;

    if( path.isEmpty() ){

        _mainTabWidget->setCurrentWidget( editor );

        path=fixPath( QFileDialog::getSaveFileName( this,"Save File As",editor->path(),"Source Files (*.monkey *.cpp *.cs *.js *.as *.java)" ) );
        if( path.isEmpty() ) return false;

    }else if( !editor->modified() ){
        return true;
    }

    bool done = editor->save( path );
    if( !done ){
        QMessageBox::warning( this,"Save File Error","Error saving file: "+path );
        return false;
    }

    updateTabLabel( editor );

    updateWindowTitle();

    updateActions();

    editor->analyzeCode();

    return true;
}

bool MainWindow::closeFile( QWidget *widget,bool really ){
    //qDebug()<<"close file";
    if( !widget ) return true;

    CodeEditor *editor = qobject_cast<CodeEditor*>( widget );

    if( editor && editor->modified() ){

        _mainTabWidget->setCurrentWidget( editor );

        QMessageBox msgBox;
        msgBox.setText( editor->path()+" has been modified." );
        msgBox.setInformativeText( "Do you want to save your changes?" );
        msgBox.setStandardButtons( QMessageBox::Save|QMessageBox::Discard|QMessageBox::Cancel );
        msgBox.setDefaultButton( QMessageBox::Save );

        int ret=msgBox.exec();

        if( ret==QMessageBox::Save ){
            if( !saveFile( editor,editor->path() ) ) return false;
        }else if( ret==QMessageBox::Cancel ){
            return false;
        }else if( ret==QMessageBox::Discard ){
        }
    }

    if( !really ) return true;

    if( widget==_codeEditor ){
        _codeEditor=0;
    }else if( widget==_helpWidget ){
        _helpWidget=0;
    }
    if( widget==_lockedEditor ){
        _lockedEditor=0;
    }

    QString path = "";
    if(editor) {
        path = editor->path();
    }

    _mainTabWidget->removeTab( _mainTabWidget->indexOf( widget ) );

    delete widget;

    if(!path.isEmpty()) {
        CodeAnalyzer::removeUserFile(path);

        /*if(_codeEditor) {
            //qDebug()<<"fill tree here:"<<_codeEditor->fileName();
            _codeEditor->fillCodeTree();
        }*/
    }

    return true;
}

bool MainWindow::confirmQuit(){

    writeSettings();

    _prefsDialog->writeSettings();

    _findInFilesDialog->writeSettings();

    CodeAnalyzer::disable();//skip analyzer while saving files on exit

    // grab all modified files into list
    QStringList files;
    QList<CodeEditor*> editors;
    for (int i = 0; i < _mainTabWidget->count(); ++i ){
        CodeEditor *editor = qobject_cast<CodeEditor*>( _mainTabWidget->widget( i ) );
        if (editor && editor->modified()) {
            files.append(editor->path());
            editors.append(editor);
        }
    }

    bool ok = true;
    if (!files.isEmpty()) {
        SaveOnCloseDialog *dialog = new SaveOnCloseDialog(this);
        dialog->fillList(files);
        int retval = dialog->exec();
        if (retval == -1) {// discard

        } else if (retval == 0) {// cancel
            ok = false;
        } else { // save
            QStringList filesToSave = dialog->filesToSave();
            foreach (CodeEditor *editor, editors) {
                QString path = editor->path();
                if (filesToSave.contains(path)) {// check if this file is checked in save-dialog
                    saveFile( editor,path );
                }
            }
        }
        delete dialog;
    }

    CodeAnalyzer::enable();//if user select 'cancel' option
    return ok;
}

void MainWindow::closeEvent( QCloseEvent *event ){
    //qDebug()<<"close app";
    if( confirmQuit() ){
        _findInFilesDialog->close();
        event->accept();
        //restore help css
        Prefs *p = Prefs::prefs();
        if( !_monkeyPath.isEmpty() && p->getBool("replaceDocsStyle") ) {
            QString jent = QApplication::applicationDirPath() + "/pagestyle.css";
            QString monk = _monkeyPath+"/docs/html/pagestyle.css";
            if(QFile::exists(jent)) {
                QFile::remove(monk);
                QFile::copy(jent,monk);
                //qDebug()<<"restore help css";
            }
        }

    } else{
        event->ignore();
    }
}

//Settings...
//
bool MainWindow::isValidMonkeyPath( const QString &path, QString &trans ){
    trans = "transcc"+HOST;
#ifdef Q_OS_WIN
    trans += ".exe";
#endif
    QString s = path+"/bin/"+trans;
    bool r = QFile::exists(s);
    if(r) {
        _monkeyPath = path;
        _transPath = trans;
        return true;
    }
    trans = "trans"+HOST;
#ifdef Q_OS_WIN
    trans += ".exe";
#endif
    s = path+"/bin/"+trans;
    r = QFile::exists(s);
    if(r) {
        _monkeyPath = path;
        _transPath = trans;
        return true;
    }
    return false;
}

void MainWindow::enumTargets(){
    if( _monkeyPath.isEmpty() ) return;

    QString cmd="\""+_monkeyPath+"/bin/"+_transPath+"\"";

    Process proc;
    if( !proc.start( cmd ) ) return;

    QString target = _targetsWidget->currentText();
    _targetsWidget->clear();

    QString sol = "Valid targets: ";
    QString ver = "TRANS monkey compiler V";
    int index = -1;
    while( proc.waitLineAvailable( 0 ) ){
        QString line=proc.readLine( 0 );
        if( line.startsWith( ver ) ){
            _transVersion = line.mid( ver.length() );
        }else if( line.startsWith( sol ) ){
            line=line.mid( sol.length() );
            QStringList bits=line.split( ' ' );
            for( int i=0;i<bits.count();++i ){
                QString bit=bits[i];
                if( !bit.isEmpty() ) {
                    QString t = bit.replace( '_',' ' );
                    _targetsWidget->addItem( t );
                    if(t == target)
                        index = _targetsWidget->count()-1;
                }
            }
        }
    }
    if(index >= 0)
        _targetsWidget->setCurrentIndex(index);
}

void MainWindow::readSettings(){

    QSettings *set = Prefs::settings();
    Prefs *prefs = Prefs::prefs();

    _isShowHelpInDock = prefs->getBool("showHelpInDock");

    _monkeyPath = prefs->getString( "monkeyPath" );
    _transPath = prefs->getString( "transPath" );

    onHelpHome();
    enumTargets();

    // hide some panels by default
    _ui->usagesDockWidget->hide();
    _ui->debugDockWidget->hide();
    _ui->docsDockWidget->hide();

    set->beginGroup( "mainWindow" );
    restoreGeometry( set->value( "geometry" ).toByteArray() );
    restoreState( set->value( "state" ).toByteArray() );
    set->endGroup();

    int n = set->beginReadArray( "openProjects" );
    for( int i=0;i<n;++i ){
        set->setArrayIndex( i );
        QString path = fixPath( set->value( "path" ).toString() );
        if( QFile::exists( path ) ) _projectTreeModel->addProject( path );
    }
    set->endArray();

    n = set->beginReadArray( "recentFiles" );
    for( int i=0;i<n;++i ){
        set->setArrayIndex( i );
        QString path = fixPath( set->value( "path" ).toString() );
        if( QFile::exists( path ) )
            _ui->menuRecent_Files->addAction( path,this,SLOT(onFileOpenRecent()) );
    }
    set->endArray();

    set->beginGroup( "buildSettings" );
    QString target = set->value( "target" ).toString();
    if( !target.isEmpty() ){
        for( int i=0;i<_targetsWidget->count();++i ){
            if( _targetsWidget->itemText(i) == target ){
                _targetsWidget->setCurrentIndex( i );
                break;
            }
        }
    }
    QString config = set->value( "config" ).toString();
    if( !config.isEmpty() ){
        for( int i=0;i<_configsWidget->count();++i ){
            if( _configsWidget->itemText(i) == config ){
                _configsWidget->setCurrentIndex( i );
                break;
            }
        }
    }
    QString locked = set->value( "locked" ).toString();
    if( !locked.isEmpty() ){
        if( CodeEditor *editor = editorWithPath( locked ) ){
            _lockedEditor = editor;
            updateTabLabel( editor );
        }
    }
    set->endGroup();

    _defaultDir = fixPath( set->value("defaultDir").toString() );

    updateTheme();
}

void MainWindow::writeSettings(){
    QSettings *set = Prefs::settings();

    set->setValue( "settingsVersion",SETTINGS_VERSION );

    set->beginGroup( "mainWindow" );
    set->setValue( "geometry",saveGeometry() );
    set->setValue( "state",saveState() );
    set->endGroup();

    set->beginWriteArray( "openProjects" );
    QVector<QString> projs = _projectTreeModel->projects();
    for( int i=0;i<projs.size();++i ){
        set->setArrayIndex(i);
        set->setValue( "path",projs[i] );
    }
    set->endArray();

    set->beginWriteArray( "openDocuments" );
    int n=0;
    for( int i=0;i<_mainTabWidget->count();++i ){
        QString path = widgetPath( _mainTabWidget->widget( i ) );
        if( path.isEmpty() ) continue;
        set->setArrayIndex( n++ );
        set->setValue( "path",path );
    }
    set->endArray();

    set->beginWriteArray( "recentFiles" );
    QList<QAction*> rfiles = _ui->menuRecent_Files->actions();
    for( int i=0;i<rfiles.size();++i ){
        set->setArrayIndex( i );
        set->setValue( "path",rfiles[i]->text() );
    }
    set->endArray();

    set->beginGroup( "buildSettings" );
    set->setValue( "target",_targetsWidget->currentText() );
    set->setValue( "config",_configsWidget->currentText() );
    set->setValue( "locked",_lockedEditor ? _lockedEditor->path() : "" );
    set->endGroup();

    set->setValue( "defaultDir",_defaultDir );
}

//Actions...
//
void MainWindow::updateActions(){

    bool ed=_codeEditor!=0;
    bool db=_debugTreeModel!=0;
    bool wr=ed && !_codeEditor->isReadOnly();
    bool sel=ed && _codeEditor->textCursor().hasSelection();

    bool saveAll=false;
    for( int i=0;i<_mainTabWidget->count();++i ){
        if( CodeEditor *editor=qobject_cast<CodeEditor*>( _mainTabWidget->widget( i ) ) ){
            if( editor->modified() ) {
                saveAll=true;
                break;
            }
        }
    }

    //file menu
    _ui->actionClose->setEnabled( ed || _helpWidget );
    _ui->actionClose_All->setEnabled( _mainTabWidget->count()>1 || (_mainTabWidget->count()==1 && !_helpWidget) );
    _ui->actionClose_Others->setEnabled( _mainTabWidget->count()>1 );
    _ui->actionSave->setEnabled( ed && _codeEditor->modified() );
    _ui->actionSave_As->setEnabled( ed );
    _ui->actionSave_All->setEnabled( saveAll );
    _ui->actionFileNext->setEnabled( _mainTabWidget->count()>1 );
    _ui->actionFilePrevious->setEnabled( _mainTabWidget->count()>1 );

    //edit menu
    _ui->actionEditUndo->setEnabled( wr && _codeEditor->document()->isUndoAvailable() );
    _ui->actionEditRedo->setEnabled( wr && _codeEditor->document()->isRedoAvailable() );

    //_ui->actionGoBack->setEnabled( wr && _codeEditor->document()->isUndoAvailable() );
    //_ui->actionGoForward->setEnabled( wr && _codeEditor->document()->isRedoAvailable() );

    _ui->actionEditCut->setEnabled( wr && sel );
    _ui->actionEditCopy->setEnabled( sel );
    _ui->actionEditPaste->setEnabled( wr );
    _ui->actionEditDelete->setEnabled( sel );
    _ui->actionEditSelectAll->setEnabled( ed );
    _ui->actionEditFind->setEnabled( ed );
    _ui->actionEditFindNext->setEnabled( ed );
    _ui->actionEditFindPrevious->setEnabled( ed );
    _ui->actionEditGoto->setEnabled( ed );
    bool usages = ed && _codeEditor->canFindUsages();
    _ui->actionFind_Usages->setEnabled( usages );

    //view menu - not totally sure why !isHidden works but isVisible doesn't...
    _ui->actionViewFile->setChecked( !_ui->fileToolBar->isHidden() );
    _ui->actionViewEdit->setChecked( !_ui->editToolBar->isHidden() );
    _ui->actionViewBuild->setChecked( !_ui->buildToolBar->isHidden() );
    _ui->actionViewHelp->setChecked( !_ui->helpToolBar->isHidden() );
    _ui->actionViewOutput->setChecked( !_ui->outputDockWidget->isHidden() );
    _ui->actionViewUsages->setChecked( !_ui->usagesDockWidget->isHidden() );
    _ui->actionViewProject->setChecked( !_ui->projectDockWidget->isHidden() );
    _ui->actionViewSource->setChecked( !_ui->sourceDockWidget->isHidden() );
    _ui->actionViewCodeTree->setChecked( !_ui->codeTreeDockWidget->isHidden() );
    _ui->actionViewDebug->setChecked( !_ui->debugDockWidget->isHidden() );
    _ui->actionViewDocs->setChecked( !_ui->docsDockWidget->isHidden() );

    //build menu
    CodeEditor *buildEditor=_lockedEditor ? _lockedEditor : _codeEditor;
    bool canBuild=!_consoleProc && isBuildable( buildEditor );
    bool canTrans=canBuild && buildEditor->fileType()=="monkey";
    _ui->actionBuildBuild->setEnabled( canBuild );
    _ui->actionBuildRun->setEnabled( canBuild || db );
    _ui->actionBuildCheck->setEnabled( canTrans );
    _ui->actionBuildUpdate->setEnabled( canTrans );
    _ui->actionStep->setEnabled( db );
    _ui->actionStep_In->setEnabled( db );
    _ui->actionStep_Out->setEnabled( db );
    _ui->actionKill->setEnabled( _consoleProc!=0 );
    _ui->actionLock_Build_File->setEnabled( /*_codeEditor!=_lockedEditor &&*/ isBuildable( _codeEditor ) );
    //_ui->actionUnlock_Build_File->setEnabled( _lockedEditor!=0 );

    //help menu
    _ui->actionHelpBack->setEnabled( _helpWidget!=0 );
    _ui->actionHelpForward->setEnabled( _helpWidget!=0 );
    _ui->actionHelpQuickHelp->setEnabled( _codeEditor!=0 );
    _ui->actionHelpRebuild->setEnabled( _consoleProc==0 );

}

void MainWindow::onChangeAnalyzerProperties(bool) {
    Prefs *p = Prefs::prefs();
    bool fill = true;
    if(sender() == _ui->toolButtonShowVariables)
        p->setValue("codeShowVariables", _ui->toolButtonShowVariables->isChecked());
    else if(sender() == _ui->toolButtonShowInherited)
        p->setValue("codeShowInherited", _ui->toolButtonShowInherited->isChecked());
    else if(sender() == _ui->toolButtonSortByName)
        p->setValue("codeSort", _ui->toolButtonSortByName->isChecked());
    else {
        fill = false;
    }
    if(_codeEditor && fill)
        _codeEditor->fillCodeTree();
    bool b = _ui->toolButtonShowVariables->isChecked();
    QString s = (b?"on":"off");
    _ui->toolButtonShowVariables->setToolTip("Show Variables: "+s);
    b = _ui->toolButtonShowInherited->isChecked();
    s = (b?"on":"off");
    _ui->toolButtonShowInherited->setToolTip("Show Inherited Members: "+s);
    b = _ui->toolButtonSortByName->isChecked();
    s = (b?"on":"off");
    _ui->toolButtonSortByName->setToolTip("Sorting: "+s);
}

void MainWindow::updateWindowTitle(){
    QWidget *widget = _mainTabWidget->currentWidget();
    QString title = "";
    if( CodeEditor *editor = qobject_cast<CodeEditor*>( widget ) ){
        title = stripDir(editor->path());
        if (editor->modified())
            title += "*";
    } else if( QWebView *webView = qobject_cast<QWebView*>( widget ) ){
        QString s = webView->url().toString();
        int i = s.lastIndexOf("/"), i2;
        if (i > 0) {
            title = s.mid(i+1);
            title = title.replace(".html","");
            i = title.lastIndexOf("#");
            if (i > 0)
                title = title.mid(i+1);
            i = title.lastIndexOf("_");
            i2 = title.lastIndexOf(".");
            if (i > i2)
                title = title.mid(i+1);
            else if (i2 > 0)
                title = title.mid(i2+1);

            title[0] = title[0].toUpper();
            title += " - Help";
        } else {
            title = "Help";
        }

    }
    if (!title.isEmpty())
        title += " - ";
    title += APP_NAME" v"APP_VERSION;
    setWindowTitle(title);
}

//Main tab widget...
//
void MainWindow::updateTabLabel( QWidget *widget ){
    if( CodeEditor *editor=qobject_cast<CodeEditor*>( widget ) ){
        QString text = editor->fileName();
        if( editor->modified() ) text=text+"*";
        if( editor==_lockedEditor ) text="¬ "+text;
        _mainTabWidget->setTabText( _mainTabWidget->indexOf( widget ),text );
    }
}

void MainWindow::onCloseMainTab( int index ){
    closeFile( _mainTabWidget->widget( index ) );
}

void MainWindow::onMainTabChanged( int index ){

    //qDebug()<<"onMainTabChanged";
    CodeEditor *_oldEditor=_codeEditor;

    QWidget *widget = _mainTabWidget->widget( index );

    _codeEditor = qobject_cast<CodeEditor*>( widget );

    _helpWidget = qobject_cast<QWebView*>( widget );

    if(_oldEditor) {
        connect( _oldEditor,SIGNAL(showCode(QString,int)),SLOT(onShowCode(QString,int)) );
    }
    if( _codeEditor ){

        connect( _codeEditor,SIGNAL(showCode(QString,int)),SLOT(onShowCode(QString,int)) );
        CodeAnalyzer::setCurFilePath(_codeEditor->path());

        _codeEditor->setFocus( Qt::OtherFocusReason );

        //CodeAnalyzer::disable();
        //onCodeAnalyze();
        //CodeAnalyzer::enable();
    }

    bool enable = (_codeEditor != 0);
    _ui->sourceDockWidget->widget()->setEnabled(enable);
    _ui->codeTreeView->setEnabled(enable);

    updateWindowTitle();

    updateActions();
}

void MainWindow::onDockVisibilityChanged( bool visible ){

    updateActions();
}

void MainWindow::onTabsMenu( const QPoint &pos ){
    if(pos.y() > 25)
        return;
    _tabsPopupMenu->exec( _mainTabWidget->mapToGlobal( pos ) );
}

void MainWindow::onEditorMenu(const QPoint &pos) {
    _editorPopupMenu->exec( _codeEditor->mapToGlobal( pos ) );
}

//Project browser...
//
void MainWindow::onProjectMenu( const QPoint &pos ){

    QModelIndex index = _ui->projectTreeView->indexAt( pos );
    if( !index.isValid() ) return;

    QFileInfo info=_projectTreeModel->fileInfo( index );

    QMenu *menu=0;
    QString typefile = (info.fileName().right( (info.fileName().length()-info.baseName().length())-1 ) ).toLower();


    bool typeimagefile = false;
    QStringList fileinvalidimg;
    fileinvalidimg << "png"<<"jpg"<<"gif"<<"ico";

    /*

    foreach (QString ext, fileinvalidimg) {
        if ( typefile.endsWith(fileinvalidimg[a])){
              typeimagefile = true;
              a=fileinvalidimgsnum;
        }
    }
    */

    if( _projectTreeModel->isProject( index ) ){
        menu=_projectPopupMenu;
    }else if( info.isFile()){
        if(!typeimagefile){

            if(typefile.endsWith("monkey")){
                menu=_fileMonkeyPopupMenu;
            }else{
                menu=_filePopupMenu;
            }
        }else if(typeimagefile){
          menu=_fileImagePopupMenu;
        }

    }else{
        menu=_dirPopupMenu;
    }

    if( !menu ) return;

    QAction *action = menu->exec( _ui->projectTreeView->mapToGlobal( pos ) );
    if( !action ) return;

    if( action==_ui->actionNewFile ){

        bool ok=false;
        QString name=QInputDialog::getText( this,"Create File","File name: "+info.filePath()+"/",QLineEdit::Normal,"",&ok );
        if( ok && !name.isEmpty() ){
            if( extractExt( name ).isEmpty() ) name+=".monkey";
            QString path=info.filePath()+"/"+name;
            newFile( path );
        }

    }else if( action==_ui->actionNewFolder ){

        bool ok=false;
        QString name=QInputDialog::getText( this,"Create Folder","Folder name: "+info.filePath()+"/",QLineEdit::Normal,"",&ok );
        if( ok && !name.isEmpty() ){
            if( !QDir( info.filePath() ).mkdir( name ) ){
                QMessageBox::warning( this,"Create Folder","Create folder failed" );
            }
        }

    }else if( action==_ui->actionRenameFile ){

        bool ok=false;
        QString newName=QInputDialog::getText( this,"Rename file","New name:",QLineEdit::Normal,info.fileName(),&ok );
        if( ok ){
            QString oldPath=info.filePath();
            QString newPath=info.path()+"/"+newName;
            if( QFile::rename( oldPath,newPath ) ){
                for( int i=0;i<_mainTabWidget->count();++i ){
                    if( CodeEditor *editor=qobject_cast<CodeEditor*>( _mainTabWidget->widget( i ) ) ){
                        if( editor->path()==oldPath ){
                            editor->rename( newPath );
                            updateTabLabel( editor );
                        }
                    }
                }
            }else{
                QMessageBox::warning( this,"Rename Error","Error renaming file: "+oldPath );
            }
        }
    }else if( action==_ui->actionOpen_on_Desktop ){

        QString path=info.filePath();
        int lenpath = info.filePath().length();
        int lenfile = info.fileName().length();
        if (info.isFile()){
                   path = path.left((lenpath-lenfile)-1);
        }
        QDesktopServices::openUrl(path);

    }else if( action==_ui->actionView_Image ){

        QLabel* label = new QLabel();
        label->setStyleSheet("QLabel {background-color : #989898;}");
        QPixmap pix(info.filePath());
        label->setPixmap(pix);
        label->setAlignment(Qt::AlignCenter);
        int imagewidth = pix.toImage().width();
        int imageheight = pix.toImage().height();
        QString sizeimg = QString::number(imagewidth)+"x"+ QString::number(imageheight);
        label->setMinimumWidth(150);
        label->setMinimumHeight(125);
        label->setWindowTitle(sizeimg+": "+info.fileName());
        label->show();

    }else if( action==_ui->actionEdit_Image ){
        QString path=info.filePath();
        QDesktopServices::openUrl(path);
    }else if( action==_ui->actionMonkeyBuild ){
        build( "build", info.filePath() );
    }else if( action==_ui->actionMonkeyBuild_and_Run ){
        build( "run", info.filePath() );
    }else if( action==_ui->actionDeleteFile ){

        QString path=info.filePath();

        if( info.isDir() ){
            if( QMessageBox::question( this,"Delete file","Okay to delete directory: "+path+" ?\n\n*** WARNING *** all subdirectories will also be deleted!",QMessageBox::Ok|QMessageBox::Cancel,QMessageBox::Cancel )==QMessageBox::Ok ){
                if( !removeDir( path ) ){
                    QMessageBox::warning( this,"Delete Error","Error deleting directory: "+info.filePath() );
                }
            }
        }else{
            if( QMessageBox::question( this,"Delete file","Okay to delete file: "+path+" ?",QMessageBox::Ok|QMessageBox::Cancel,QMessageBox::Cancel )==QMessageBox::Ok ){
                if( QFile::remove( path ) ){
                    for( int i=0;i<_mainTabWidget->count();++i ){
                        if( CodeEditor *editor=qobject_cast<CodeEditor*>( _mainTabWidget->widget( i ) ) ){
                            if( editor->path()==path ){
                                closeFile( editor );
                                i=-1;
                            }
                        }
                    }
                }else{
                    QMessageBox::warning( this,"Delete Error","Error deleting file: "+info.filePath() );
                }
            }
        }
    }else if( action==_ui->actionCloseProject ){

        _projectTreeModel->removeProject( info.filePath() );

    }else if( action==_ui->actionEditFindInFiles ){

        _findInFilesDialog->show( info.filePath() );

        _findInFilesDialog->raise();

    }
}

void MainWindow::onFileClicked( const QModelIndex &index ){

    if( !_projectTreeModel->isDir( index ) ) openFile( _projectTreeModel->filePath( index ),true );
}

void MainWindow::onUsagesMenu( const QPoint &pos ) {

    QAction *action = _usagesPopupMenu->exec( _ui->usagesTabWidget->mapToGlobal( pos ) );
    if( !action ) return;

    if( action == _ui->actionUsagesSelectAll ){
        onUsagesSelectAll();
    }
    else if( action == _ui->actionUsagesUnselectAll ){
        onUsagesUnselectAll();
    }
}

//Editor...
//
void MainWindow::onTextChanged(){
    if( CodeEditor *editor = qobject_cast<CodeEditor*>( sender() ) ){
        if( editor->modified()<2 ){
            updateTabLabel( editor );
            updateWindowTitle();
        }
    }
    updateActions();
}

void MainWindow::onCursorPositionChanged(){
    if( sender() == _codeEditor ) {
        QString s = _codeEditor->cursorRowCol();
        _statusWidget->setText( s );
    }
    updateActions();
    //qDebug()<<"onCursorPositionChanged";
}

void MainWindow::onShowCode( const QString &path, int line, bool error ){
    //qDebug()<<"onShowCode";
    CodeEditor *editor = qobject_cast<CodeEditor*>( openFile( path,true ) );
    if (editor){
        editor->gotoLine( line );
        editor->highlightLine( line, (error ? HlError : HlCommon) );
        if (editor == _codeEditor)
            editor->setFocus( Qt::OtherFocusReason );
    }
}

void MainWindow::onShowCode( const QString &path,int pos,int len ){
    //qDebug()<<"main.showcode:"<<path;
    if( CodeEditor *editor=qobject_cast<CodeEditor*>( openFile( path,true ) ) ){
        if(pos >= 0) {
            QTextCursor cursor( editor->document() );
            cursor.setPosition( pos );
            cursor.setPosition( pos+len,QTextCursor::KeepAnchor );
            editor->setTextCursor( cursor );
        }
        if( editor==_codeEditor ) editor->setFocus( Qt::OtherFocusReason );
    }
}

void MainWindow::updateCodeViews(QTreeView *tree, QListView* list) {
    CodeAnalyzer::setViews(tree, list);
    connect( tree,SIGNAL(clicked(QModelIndex)),SLOT(onCodeTreeViewClicked(QModelIndex)) );
    connect( list,SIGNAL(clicked(QModelIndex)),SLOT(onSourceListViewClicked(QModelIndex)) );

}

void MainWindow::onCodeTreeViewClicked( const QModelIndex &index ) {
    QStandardItem *i = CodeAnalyzer::treeItemModel()->itemFromIndex( index );
    if(i && i->parent() != 0) {
        i = i->parent();
        //qDebug()<<"parent1:"<<i->text();
        if(i && i->parent() != 0) {
            i = i->parent();
           //qDebug()<<"parent2:"<<i->text();
        }
    }
    if(i) {
        QString path = i->toolTip();
        int index = -1;
        for( int i = 0; i < _mainTabWidget->count(); ++i ){
            if( CodeEditor *editor = qobject_cast<CodeEditor*>( _mainTabWidget->widget( i ) ) ){

                if( editor->path() == path ) {
                    index = i;
                    break;
                }
            }
        }
        if(index != -1) {
            //qDebug()<<"show tab with:"<<path;
            _mainTabWidget->setCurrentIndex(index);
        }
        else {
            //qDebug()<<"show code for:"<<path;
            CodeAnalyzer::flushFileModified(path);
            onShowCode(path, 0, 0);
        }
    }
    if(_codeEditor) {
        //onCodeAnalyze();
        _codeEditor->onCodeTreeViewClicked(index);
    }
}

void MainWindow::onSourceListViewClicked( const QModelIndex &index ) {
    if(_codeEditor)
        _codeEditor->onSourceListViewClicked(index);
    qDebug()<<"onSourceListViewClicked";
}

//Console...
//
void MainWindow::print(const QString &str, QString kind ){

    if(kind == "") {
        _ui->outputTextEdit->setTextColor( Theme::isDark() ? QColor(0xc8c8c8) : QColor(0x050505) );
    }
    else if(kind == "error") {
        _ui->outputTextEdit->setTextColor( Theme::isDark() ? QColor(0xE07464) : QColor(0x800000) );
        _ui->outputDockWidget->setVisible(true);
    }
    else if(kind == "finish") {
        _ui->outputTextEdit->setTextColor( Theme::isDark() ? QColor(0x69C1F1) : QColor(0x000080) );
    }
    else if(kind == "debug") {
        _ui->outputTextEdit->setTextColor( Theme::isDark() ? QColor(128,0,128) : QColor(0,128,0) );
    }
    QTextCursor cursor=_ui->outputTextEdit->textCursor();
    cursor.insertText( str );
    cursor.insertBlock();
    cursor.movePosition( QTextCursor::End,QTextCursor::MoveAnchor );
    _ui->outputTextEdit->setTextCursor( cursor );
}

void MainWindow::cdebug( const QString &str ){
    print( str, "debug" );
}

void MainWindow::runCommand( QString cmd, QWidget *fileWidget ){

    QString targetStr = _targetsWidget->currentText().replace( ' ','_' );
    //qDebug() << "targetStr"<<targetStr;
    cmd=cmd.replace( "${TARGET}",targetStr );
    cmd=cmd.replace( "${CONFIG}",_configsWidget->currentText() );
    cmd=cmd.replace( "${MONKEYPATH}",_monkeyPath );
    //cmd=cmd.replace( "${BLITZMAXPATH}",_blitzmaxPath );
    if( fileWidget ) cmd=cmd.replace( "${FILEPATH}",widgetPath( fileWidget ) );

    _consoleProc = new Process;

    connect( _consoleProc,SIGNAL(lineAvailable(int)),SLOT(onProcLineAvailable(int)) );
    connect( _consoleProc,SIGNAL(finished()),SLOT(onProcFinished()) );

    _ui->outputTextEdit->clear();

    print( cmd, "" );

    if( !_consoleProc->start( cmd ) ){
        delete _consoleProc;
        _consoleProc=0;
        QMessageBox::warning( this,"Process Error","Failed to start process: "+cmd );
        return;
    }

    updateActions();

    /*if (targetStr == "Html5_Game") {
        if (_previewHtml5Dialog == 0)
            _previewHtml5Dialog = new PreviewHtml5(this);
        _previewHtml5Dialog->showMe();
    }*/

}

void MainWindow::onProcStdout(){

    static QString comerr = " : Error : ";
    static QString runerr = "Monkey Runtime Error : ";

    QString text = _consoleProc->readLine( 0 );

    bool iscomerr = text.contains( comerr );
    bool isrunerr = text.startsWith( runerr );

    print( text, (iscomerr||isrunerr ? "error" : "") );

    if(iscomerr){
        int i0=text.indexOf( comerr );
        QString info=text.left( i0 );
        int i=info.lastIndexOf( '<' );
        if( i!=-1 && info.endsWith( '>' ) ){
            QString path=info.left( i );
            int line=info.mid( i+1,info.length()-i-2 ).toInt()-1;
            QString err=text.mid( i0+comerr.length() );

            onShowCode( path,line );

            QMessageBox::warning( this,"Compile Error",err );
        }
    }else if( isrunerr ){
        QString err=text.mid( runerr.length() );

        //not sure what this voodoo is for...!
        showNormal();
        raise();
        activateWindow();
        QMessageBox::warning( this,"Monkey Runtime Error",err );
    }
}

void MainWindow::onProcStderr(){

    if( _debugTreeModel && _debugTreeModel->stopped() ) return;

    QString text=_consoleProc->readLine( 1 );

    if( text.startsWith( "{{~~" ) && text.endsWith( "~~}}" ) ){

        QString info=text.mid( 4,text.length()-8 );

        int i=info.lastIndexOf( '<' );
        if( i!=-1 && info.endsWith( '>' ) ){
            QString path=info.left( i );
            int line=info.mid( i+1,info.length()-i-2 ).toInt()-1;
            onShowCode( path,line );
        }else{
            print( info, "error" );
        }

        if( !_debugTreeModel ){

            raise();

            _debugTreeModel=new DebugTreeModel( _consoleProc );
            connect( _debugTreeModel,SIGNAL(showCode(QString,int)),SLOT(onShowCode(QString,int)) );

            _ui->debugTreeView->setModel( _debugTreeModel );
            connect( _ui->debugTreeView,SIGNAL(clicked(const QModelIndex&)),_debugTreeModel,SLOT(onClicked(const QModelIndex&)) );

            _ui->debugDockWidget->setVisible(true);

            print( "STOPPED", "error" );
        }

        _debugTreeModel->stop();

        updateActions();

        return;
    }

    print( text, "error" );
}

void MainWindow::onProcLineAvailable( int channel ){
    while( _consoleProc ){
        if( _consoleProc->isLineAvailable( 0 ) ){
            onProcStdout();
        }else if( _consoleProc->isLineAvailable( 1 ) ){
            onProcStderr();
        }else{
            return;
        }
    }
}

void MainWindow::onProcFinished(){

    while( _consoleProc->waitLineAvailable( 0,100 ) ){
        onProcLineAvailable( 0 );
    }

    print( "Done.","finish" );

    if( _debugTreeModel ){
        _ui->debugTreeView->setModel( 0 );
        delete _debugTreeModel;
        _debugTreeModel=0;
    }

    if( _consoleProc ){
        delete _consoleProc;
        _consoleProc=0;
    }

    updateActions();

    statusBar()->showMessage( "Ready." );
}

void MainWindow::build( QString mode, QString pathmonkey){

    CodeEditor *editor = (_lockedEditor ? _lockedEditor : _codeEditor);
    if( !isBuildable( editor ) )
        return;

    QString filePath = editor->path();
    if( filePath.isEmpty() )
        return;

    QString cmd, msg = "Buillding: "+filePath+"...";


    if( editor->fileType()=="monkey" ){
        if( mode=="run" ){
            if(pathmonkey.endsWith("run")){

                cmd="\"${MONKEYPATH}/bin/"+_transPath+"\" -target=${TARGET} -config=${CONFIG} -run \"${FILEPATH}\"";

            }else {

                cmd="\"${MONKEYPATH}/bin/"+_transPath+"\" -target=${TARGET} -config=${CONFIG} -run "+pathmonkey;
            }
        }else if( mode=="build" ){

            if(pathmonkey.endsWith("run")){
                 cmd="\"${MONKEYPATH}/bin/"+_transPath+"\" -target=${TARGET} -config=${CONFIG} \"${FILEPATH}\"";

            }else {
                 cmd="\"${MONKEYPATH}/bin/"+_transPath+"\" -target=${TARGET} -config=${CONFIG} "+pathmonkey;
            }

        }else if( mode=="update" ){
            cmd="\"${MONKEYPATH}/bin/"+_transPath+"\" -target=${TARGET} -config=${CONFIG} -update \"${FILEPATH}\"";
            msg="Updating: "+filePath+"...";
        }else if( mode=="check" ){
            cmd="\"${MONKEYPATH}/bin/"+_transPath+"\" -target=${TARGET} -config=${CONFIG} -check \"${FILEPATH}\"";
            msg="Checking: "+filePath+"...";
        }
    }

    if( !cmd.length() ) return;

    onFileSaveAll();

    statusBar()->showMessage( msg );

    runCommand( cmd,editor );
}

//***** File menu *****

void MainWindow::onFileNew(){
    if(sender() == _ui->actionNew) {
        newFile( "" );
    }
    else {
        QString s = QApplication::applicationDirPath()+"/projects/";
        QDir dir(s);
        bool b = dir.mkpath(s);
        //if(!b) {
        //    QMessageBox::information(this,"New File","Can't create dir");
        //}
        QDate dd = QDate::currentDate();
        QString y = QString::number(dd.year());
        QString m = QString::number(dd.month());
        QString d = QString::number(dd.day());
        QTime tt = QTime::currentTime();
        QString t = QString::number(tt.hour())+"-"+QString::number(tt.minute())+"-"+QString::number(tt.second());

        s += y+"-"+m+"-"+d+"_"+t+".monkey";
        //QMessageBox::information(this,"",s);
        newFile(s);
    }
}

void MainWindow::onFileOpen(){
    openFile( "",true );
}

void MainWindow::onFileOpenRecent(){
    if( QAction *action=qobject_cast<QAction*>( sender() ) ){
        openFile( action->text(),false );
    }

}

void MainWindow::onFileClose(){
    closeFile( _mainTabWidget->currentWidget() );
}

void MainWindow::onFileCloseAll(){
    for(;;){
        int i;
        CodeEditor *editor=0;
        for( i=0;i<_mainTabWidget->count();++i ){
            if( editor=qobject_cast<CodeEditor*>( _mainTabWidget->widget( i ) ) ) break;
        }
        if( !editor ) return;

        if( !closeFile( editor ) ) return;
    }
}

void MainWindow::onFileCloseOthers(){
    if( _helpWidget ) return onFileCloseAll();
    if( !_codeEditor ) return;

    for(;;){
        int i;
        QWidget *widget=0;
        CodeEditor *editor=0;
        for( i=0;i<_mainTabWidget->count();++i ){
            editor=qobject_cast<CodeEditor*>( _mainTabWidget->widget( i ) );
            if( editor && editor!=_codeEditor ) break;
            editor=0;
        }
        if( !editor ) return;

        if( !closeFile( editor ) ) return;
    }
}

void MainWindow::onFileSave(){
    if( !_codeEditor ) return;

    saveFile( _codeEditor,_codeEditor->path() );
}

void MainWindow::onFileSaveAs(){
    if( !_codeEditor ) return;

    saveFile( _codeEditor,"" );
}

void MainWindow::onFileSaveAll(){
    for( int i = 0; i < _mainTabWidget->count(); ++i ){
        CodeEditor *editor = qobject_cast<CodeEditor*>( _mainTabWidget->widget( i ) );
        if( editor && !saveFile( editor,editor->path() ) ) return;
    }
}

void MainWindow::onFileNext(){
    if( _mainTabWidget->count()<2 ) return;

    int i=_mainTabWidget->currentIndex()+1;
    if( i>=_mainTabWidget->count() ) i=0;

    _mainTabWidget->setCurrentIndex( i );
}

void MainWindow::onFilePrevious(){
    if( _mainTabWidget->count()<2 ) return;

    int i=_mainTabWidget->currentIndex()-1;
    if( i<0 ) i=_mainTabWidget->count()-1;

    _mainTabWidget->setCurrentIndex( i );
}

void MainWindow::onFilePrefs(bool openPathSection){

    QString mpath = _monkeyPath;

    _prefsDialog->setModal( true );

    if (openPathSection)
        _prefsDialog->openPathSection();

    _prefsDialog->exec();

    if(!_monkeyPath.isEmpty() && _monkeyPath != mpath){
        enumTargets();
        initKeywords();

        if(!_codeEditor)
            onHelpHome();
    }

    _isShowHelpInDock = Prefs::prefs()->getBool("showHelpInDock");

    updateActions();
}

void MainWindow::onFileQuit(){
    if( confirmQuit() ) QApplication::quit();
}

//***** Edit menu *****

void MainWindow::onEditUndo(){
    if( !_codeEditor ) return;

    _codeEditor->undo();
}

void MainWindow::onEditRedo(){
    if( !_codeEditor ) return;

    _codeEditor->redo();
}

void MainWindow::onEditCut(){
    if( !_codeEditor ) return;

    _codeEditor->cut();
}

void MainWindow::onEditCopy(){
    if( !_codeEditor ) return;

    _codeEditor->copy();
}

void MainWindow::onEditPaste(){
    if( !_codeEditor ) return;

    _codeEditor->onPaste(QApplication::clipboard()->text());
}

void MainWindow::onEditDelete(){
    if( !_codeEditor ) return;

    _codeEditor->textCursor().removeSelectedText();
}

void MainWindow::onEditSelectAll(){
    if( !_codeEditor ) return;

    _codeEditor->selectAll();

    updateActions();
}

void MainWindow::onEditFind(){
    if( !_codeEditor ) return;

    _ui->frameSearch->setVisible(true);
    _ui->editFind->setFocus();
    QString s = _codeEditor->textCursor().selectedText();
    if(!s.isEmpty())
        _ui->editFind->setText(s);
    _ui->editFind->selectAll();
}

void MainWindow::onEditFindNext(){
    if( _codeEditor ) {
        onFindReplace( 0 );
        //if(_ui->frameSearch->isVisible())
        //    _ui->editFind->setFocus();
    }
}
void MainWindow::onEditFindPrev(){
    if( _codeEditor )
        onFindReplace( 3 );
}
void MainWindow::onEditReplace(){
    if( _codeEditor )
        onFindReplace( 1 );
}
void MainWindow::onEditReplaceAll(){
    if( _codeEditor )
        onFindReplace( 2 );
}
void MainWindow::onFindReplace( int how ){
    if( !_codeEditor ) return;

    QString findText = _ui->editFind->text();
    if( findText.isEmpty() ) return;

    QString replaceText = _ui->editReplace->text();

    bool cased = _ui->chbCase->isChecked();

    bool wrap = _ui->chbWrap->isChecked();;

    bool found = true;
    if( how==0 || how==3){
        found = _codeEditor->findNext( findText,cased,wrap,(how==3) );
    }
    else if( how==1 ){
        if( _codeEditor->replace( findText,replaceText,cased ) ){
            found = _codeEditor->findNext( findText,cased,wrap );
        }
    }
    else if( how==2 ){
        int n = _codeEditor->replaceAll( findText,replaceText,cased,wrap );
        QMessageBox::information( this,"Replace All",QString::number(n)+" occurences replaced" );
    }
    if( !found ){
        QApplication::beep();
        _ui->editFind->setFocus();
    }
}

void MainWindow::onEditGoto(){
    if( !_codeEditor )
        return;

    bool ok =false;
    int line = QInputDialog::getInt( this,"Go to Line","Line number:",1,1,_codeEditor->document()->blockCount(),1,&ok );
    if ( ok ){
        _codeEditor->gotoLine( line-1 );
        _codeEditor->highlightLine( line-1 );
    }
}

void MainWindow::onEditFindInFiles(){

    _findInFilesDialog->show();

    _findInFilesDialog->raise();
}

//***** View menu *****

void MainWindow::onViewToolBar(){
    if( sender()==_ui->actionViewFile ){
        _ui->fileToolBar->setVisible( _ui->actionViewFile->isChecked() );
    }else if( sender()==_ui->actionViewEdit ){
        _ui->editToolBar->setVisible( _ui->actionViewEdit->isChecked() );
    }else if( sender()==_ui->actionViewBuild ){
        _ui->buildToolBar->setVisible( _ui->actionViewBuild->isChecked() );
    }else if( sender()==_ui->actionViewHelp ){
        _ui->helpToolBar->setVisible( _ui->actionViewHelp->isChecked() );
    }
}

void MainWindow::onViewWindow(){
    if( sender() == _ui->actionViewProject ){
        _ui->projectDockWidget->setVisible( _ui->actionViewProject->isChecked() );
    }
    else if( sender() == _ui->actionViewSource ){
        _ui->sourceDockWidget->setVisible( _ui->actionViewSource->isChecked() );
    }
    else if( sender() == _ui->actionViewOutput ){
        _ui->outputDockWidget->setVisible( _ui->actionViewOutput->isChecked() );
    }
    else if( sender() == _ui->actionViewUsages ){
        _ui->usagesDockWidget->setVisible( _ui->actionViewUsages->isChecked() );
    }
    else if( sender() == _ui->actionViewCodeTree ){
        _ui->codeTreeDockWidget->setVisible( _ui->actionViewCodeTree->isChecked() );
    }
    else if( sender() == _ui->actionViewDebug ){
        _ui->debugDockWidget->setVisible( _ui->actionViewDebug->isChecked() );
    }
    else if( sender() == _ui->actionViewDocs ){
        _ui->docsDockWidget->setVisible( _ui->actionViewDocs->isChecked() );
    }
    else if( sender() == _ui->actionViewDockShowAll ){
        bool vis = true;
        _ui->codeTreeDockWidget->setVisible( vis );
        _ui->sourceDockWidget->setVisible( vis );
        _ui->projectDockWidget->setVisible( vis );
        _ui->debugDockWidget->setVisible( vis );
        _ui->outputDockWidget->setVisible( vis );
        _ui->usagesDockWidget->setVisible( vis );
        _ui->docsDockWidget->setVisible( vis );
    }
    else if( sender() == _ui->actionViewDocksHideAll ){
        bool vis = false;
        _ui->codeTreeDockWidget->setVisible( vis );
        _ui->sourceDockWidget->setVisible( vis );
        _ui->projectDockWidget->setVisible( vis );
        _ui->debugDockWidget->setVisible( vis );
        _ui->outputDockWidget->setVisible( vis );
        _ui->usagesDockWidget->setVisible( vis );
        _ui->docsDockWidget->setVisible( vis );
    }
}

//***** Build menu *****

void MainWindow::onBuildBuild(){
    build( "build","run" );
}

void MainWindow::onBuildRun(){
    if( _debugTreeModel ){
        _debugTreeModel->run();
    }else{
        build( "run","run" );
    }
}

void MainWindow::onBuildCheck(){
    build( "check" ,"run");
}

void MainWindow::onBuildUpdate(){
    build( "update" ,"run");
}

void MainWindow::onDebugStep(){
    if( !_debugTreeModel ) return;
    _debugTreeModel->step();
}

void MainWindow::onDebugStepInto(){
    if( !_debugTreeModel ) return;
    _debugTreeModel->stepInto();
}

void MainWindow::onDebugStepOut(){
    if( !_debugTreeModel ) return;
    _debugTreeModel->stepOut();
}

void MainWindow::onDebugKill(){
    if( !_consoleProc ) return;

    print( "Killing process...", "finish" );

    _consoleProc->kill();
}

void MainWindow::onBuildTarget(){

    QStringList items;
    for( int i=0;i<_targetsWidget->count();++i ){
        items.push_back( _targetsWidget->itemText( i ) );
    }

    bool ok=false;
    QString item=QInputDialog::getItem( this,"Select build target","Build target:",items,_targetsWidget->currentIndex(),false,&ok );
    if( ok ){
        int index=items.indexOf( item );
        if( index!=-1 ) _targetsWidget->setCurrentIndex( index );
    }
}

void MainWindow::onBuildConfig(){

    QStringList items;
    for( int i=0;i<_configsWidget->count();++i ){
        items.push_back( _configsWidget->itemText( i ) );
    }

    bool ok=false;
    QString item=QInputDialog::getItem( this,"Select build config","Build config:",items,_configsWidget->currentIndex(),false,&ok );
    if( ok ){
        int index=items.indexOf( item );
        if( index!=-1 ) _configsWidget->setCurrentIndex( index );
    }
}

void MainWindow::onBuildLockFile(){
    CodeEditor *wasLocked = _lockedEditor;
    if( wasLocked ){
        _lockedEditor = 0;
        updateTabLabel( wasLocked );
    }
    if( _codeEditor && _codeEditor != wasLocked ){
        _lockedEditor = _codeEditor;
        updateTabLabel( _lockedEditor );
    }
    updateActions();
}

void MainWindow::onBuildUnlockFile(){
    if( CodeEditor *wasLocked=_lockedEditor ){
        _lockedEditor=0;
        updateTabLabel( wasLocked );
    }
    updateActions();
}

void MainWindow::onBuildAddProject(){

    QString dir=fixPath( QFileDialog::getExistingDirectory( this,"Select project directory",_defaultDir,QFileDialog::ShowDirsOnly|QFileDialog::DontResolveSymlinks ) );
    if( dir.isEmpty() ) return;

    if( !_projectTreeModel->addProject( dir ) ){
        QMessageBox::warning( this,"Add Project Error","Error adding project: "+dir );
    }
}

//***** Window menu *****

void MainWindow::onSwitchFullscreen() {
    if(windowState() != Qt::WindowFullScreen)
        this->setWindowState(Qt::WindowFullScreen);
    else
        this->setWindowState(Qt::WindowActive);
}


//***** Help menu *****

void MainWindow::onHelpHome(){

    QString htmlDocs=_monkeyPath+"/docs/html/Home.html";

    if( QFile::exists( htmlDocs ) ){
        openFile( "file:///"+htmlDocs,false );
    }else {
        htmlDocs = "qrc:/txt/home.html";
        openFile( htmlDocs,false );
    }
}

void MainWindow::onHelpBack(){
    if( !_helpWidget ) return;

    _helpWidget->back();
    updateWindowTitle();
}

void MainWindow::onHelpForward(){
    if( !_helpWidget ) return;

    _helpWidget->forward();
    updateWindowTitle();
}

void MainWindow::onHelpQuickHelp(){
    if( !_codeEditor ) return;

    QString ident = _codeEditor->identAtCursor();
    if( ident.isEmpty() ) return;

    QuickHelp *h = QuickHelp::help( ident );
    if( !h )
        return;
    ident = h->topic;
    QString url = h->url;
    QTextCursor cursor( _codeEditor->document() );
    int pos = cursor.position();
    if( ident == _lastHelpIdent && pos == _lastHelpCursorPos ) {
        openFile(url,false);
    }
    else {
        statusBar()->showMessage("Help: "+h->quick());// code->descrAsItem());
    }
    _lastHelpIdent = ident;
    _lastHelpCursorPos = pos;
}

void MainWindow::onHelpAbout(){
    QString hrefGithub = "https://github.com/engor/Jentos_IDE";
    QString hrefSite = "http://fingerdev.com/jentos";
    QString hrefPaypal = "https://www.paypal.me/engor";//"https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=RGCTKTP8H3CNE";
    QString hrefIcons = "https://icons8.com";
    QString APP_ABOUT = "<html><head><style>a{color:#CC8030;}</style></head><body bgcolor2='#ff3355'><b>"APP_NAME"</b> is a powefull code editor for the Monkey programming language.<br>"
            "Based on Ted V"TED_VERSION".<br><br>"
            "Visit <a href='"+hrefSite+"'>"+hrefSite+"</a> for more information.<br><br>"
            "Latest sources: <a href='"+hrefGithub+"'>"+hrefGithub+"</a><br><br>"
            "Almost all icons taken from <a href='"+hrefIcons+"'>"+hrefIcons+"</a><br><br>"
            "Version: "APP_VERSION+"<br>Trans: "+_transVersion+"<br>Qt: "_STRINGIZE(QT_VERSION)+"<br><br>"
            "Jentos is free and always be free.<br>But you may support the author (Evgeniy Goroshkin)<br>"
            "via <a href=\""+hrefPaypal+"\">PayPal Donation</a>.<br>"
            "</body></html>";
    QMessageBox::information( this, "About", APP_ABOUT );
}

void MainWindow::onShowHelp( const QString &topic ) {
    QuickHelp *h = QuickHelp::help( topic );
    if( h != 0 )
        openFile( h->url, false );
}

void MainWindow::onHelpOnlineDocs() {
    openFile( "http://monkey-x.com/docs/html/Home.html", false );
}

void MainWindow::onHelpMonkeyHomepage() {
    QString s = "http://monkey-x.com";
    QDesktopServices::openUrl( s );
}

void MainWindow::onLinkClicked( const QUrl &url ){

    QString str=url.toString();
    QString lstr=str.toLower();

    if( lstr.startsWith( "file:///" ) ){
        QString ext=";"+extractExt(lstr)+";";
        if( textFileTypes.contains( ext ) || codeFileTypes.contains( ext ) ){
            openFile( str.mid( 8 ),false );
            return;
        }
        openFile( str,false );
        return;
    }
    if( lstr.startsWith( "http://" ) ){
        openFile( str,false );
        return;
    }
    if( lstr.startsWith( "prog://" ) ){
        if(lstr == "prog://settings")
            onFilePrefs(true);
        return;
    }
    QDesktopServices::openUrl( str );
}

void MainWindow::onHelpRebuild(){
    if( _consoleProc || _monkeyPath.isEmpty() ) return;

    onFileSaveAll();

    QString cmd="\"${MONKEYPATH}/bin/makedocs"+HOST+"\"";

    runCommand( cmd,0 );

    initKeywords();

    for( int i=0;i<_mainTabWidget->count();++i ){
        QWebView *webView=qobject_cast<QWebView*>( _mainTabWidget->widget( i ) );

        if( webView ) webView->triggerPageAction( QWebPage::ReloadAndBypassCache );
    }
}

void MainWindow::onStatusBarChanged(const QString &text) {
    statusBar()->showMessage( text );
}

void MainWindow::onCommentUncommentBlock() {
    if( _codeEditor )
        _codeEditor->commentUncommentBlock();
}

void MainWindow::onToggleBookmark() {
    if( _codeEditor )
        _codeEditor->bookmarkToggle();
}

void MainWindow::onPreviousBookmark() {
    if( _codeEditor )
        _codeEditor->bookmarkPrev();
}

void MainWindow::onNextBookmark() {
    if( _codeEditor )
        _codeEditor->bookmarkNext();
}

void MainWindow::onFoldAll() {
    if( _codeEditor )
        _codeEditor->foldAll();
}

void MainWindow::onUnfoldAll() {
    if( _codeEditor )
        _codeEditor->unfoldAll();
}

void MainWindow::onGoBack() {
    if( _codeEditor )
        _codeEditor->goBack();
    else
        onHelpBack();
}

void MainWindow::onGoForward() {
    if( _codeEditor )
        _codeEditor->goForward();
    else
        onHelpForward();
}

void MainWindow::onAutoformatAll() {
    if( _codeEditor ) {
        QMessageBox m;
        m.setText("Autoformating whole document.\nPlease, wait...");
        m.showNormal();
        m.setStandardButtons(QMessageBox::Close);
        QApplication::processEvents();
        _codeEditor->autoformatAll();
        m.hide();
    }
}

void MainWindow::onKeyEscapePressed() {
    if (_ui->frameSearch->isVisible())
        _ui->frameSearch->hide();
}

void MainWindow::onCheckForUpdates(bool isQuiet) {
    //QUrl url("https://raw.githubusercontent.com/engor/Jentos_IDE/dev/.gitignore");
    QUrl url("http://fingerdev.com/updates/jentos.code.txt");
    if (_networkManager)
        delete _networkManager;
    _networkManager = new QNetworkAccessManager(this);
    connect(_networkManager, SIGNAL(finished(QNetworkReply*)), SLOT(onNetworkFinished(QNetworkReply*)));
    QNetworkRequest request;
    request.setUrl(url);
    _isUpdaterQuiet = isQuiet;
    _networkManager->get(request);
}

void MainWindow::onCheckForUpdatesQuiet()
{
    onCheckForUpdates(true);
}

void MainWindow::onNetworkFinished(QNetworkReply *reply) {
    reply->deleteLater();
    QByteArray bytes = reply->readAll();
    QString s(bytes);
    s = s.trimmed();
    int i = s.indexOf("$$$");
    int hasInfo = (i >= 0);
    QString title = "Updater";
    if (!hasInfo) {
        if (!_isUpdaterQuiet)
            QMessageBox::information(this, title, "<html><head><style>a{color:#CC8030;}</style></head><body>Check new version error.<br><br>Visit <a href=\"http://fingerdev.com/jentos\">Jentos Homepage</a> to get information about latest version.</body></html>");
        return;
    }
    s = s.mid(3).trimmed();// skip $$$
    i = s.indexOf("$$$");
    s = s.left(i); //now s contains our clean update info
    i = s.indexOf("\n");
    QString newVersion = s.left(i).trimmed();
    QString curVersion = APP_VERSION;
    if (newVersion > curVersion) {
        s = s.mid(i+1);
        s = s.replace("%CURRENT_VER%", curVersion);
        s = s.replace("%NEW_VER%", newVersion);
        QMessageBox::information(this, title, s);
    } else if (!_isUpdaterQuiet){
        QMessageBox::information(this, title, "<html><head><style>a{color:#CC8030;}</style></head><body>No updates available.<br><b>You are using the latest version "+curVersion+".</b><br><br>Visit <a href=\"http://fingerdev.com/jentos\">Jentos Homepage</a> to get information about latest version.</body></html>");
    }
}

void MainWindow::onOpenUrl() {
    QString url;
    if(sender() == _ui->actionHelpOnlineDocs)
        url = "http://www.monkey-x.com/docs/html/Home.html";
    else if(sender() == _ui->actionHelpMonkeyHomepage)
        url = "http://www.monkey-x.com/";
    else if(sender() == _ui->actionHelpFingerDevStudioHomepage)
        url = "http://fingerdev.com/";
    QDesktopServices::openUrl(url);
}

void MainWindow::onLowerUpperCase() {
    if( _codeEditor ) {
        _codeEditor->lowerUpperCase( sender() == _ui->actionFormatLowercase );
    }
}

void MainWindow::onUsagesRename() {
    QString newIdent = _ui->editUsagesRename->text();
    if(newIdent == "") {
        QMessageBox::information(this,"Rename","Field 'Rename with' is empty! Enter correct value.");
        return;
    }
    QWidget *w = _ui->usagesTabWidget->currentWidget();
    w = w->layout()->itemAt(0)->widget();
    QTreeWidget *tree = dynamic_cast<QTreeWidget*>(w);
    if(!tree)
        return;
    int newLen = newIdent.length();
    bool selOnly = true;//_ui->chbUsageSelectedOnly->isChecked();
    QTreeWidgetItem *root = tree->invisibleRootItem();
    for(int k = 0; k < root->childCount(); ++k) {
        QTreeWidgetItem *item = root->child(k);
        bool first = true;
        int delta = 0;
        for(int j = 0; j < item->childCount(); ++j) {
            QTreeWidgetItem *sub = item->child(j);
            if(selOnly && sub->checkState(0) != Qt::Checked) {
                continue;
            }
            UsagesResult *u = UsagesResult::item(sub);
            if(u) {
                if(first) {
                    openFile(u->path, true);
                    first = false;
                }
                if(_codeEditor) {
                    int from = u->positionStart+delta;
                    int to = u->positionEnd+delta;
                    _codeEditor->replaceInRange(from, to, newIdent);
                    delta += (newLen - u->ident.length());
                }
            }
        }
    }
}

void MainWindow::onUsagesSelectAll() {
    QWidget *w = _ui->usagesTabWidget->currentWidget();
    w = w->layout()->itemAt(0)->widget();
    QTreeWidget *tree = dynamic_cast<QTreeWidget*>(w);
    if(tree) {
        QTreeWidgetItem *root = tree->invisibleRootItem();
        for(int k = 0; k < root->childCount(); ++k) {
            QTreeWidgetItem *item = root->child(k);
            for(int j = 0; j < item->childCount(); ++j) {
                QTreeWidgetItem *sub = item->child(j);
                sub->setCheckState(0,Qt::Checked);
            }
        }
    }
}

void MainWindow::onUsagesUnselectAll() {
    QWidget *w = _ui->usagesTabWidget->currentWidget();
    w = w->layout()->itemAt(0)->widget();
    QTreeWidget *tree = dynamic_cast<QTreeWidget*>(w);
    if(tree) {
        QTreeWidgetItem *root = tree->invisibleRootItem();
        for(int k = 0; k < root->childCount(); ++k) {
            QTreeWidgetItem *item = root->child(k);
            for(int j = 0; j < item->childCount(); ++j) {
                QTreeWidgetItem *sub = item->child(j);
                sub->setCheckState(0,Qt::Unchecked);
            }
        }
    }
}

void MainWindow::onDocsZoomChanged(int) {
    _ui->webView->setZoomFactor(_ui->zoomSlider->value()/100.0f);
}

void MainWindow::on_actionAddProperty_triggered()
{
    if (_codeEditor != 0)
        _codeEditor->showDialogAddProperty();

}

void MainWindow::on_pushButtonClassSummary_clicked()
{
    ItemWithData *si = CodeAnalyzer::itemInList(0);
    QString msg;
    if (si != 0) {
        CodeItem *code = si->code();
        if (code && code->isClassOrInterface()) {
            msg = code->summary();
        }
    }
    if (msg.isEmpty()) {
        msg = "Class or Interface isn't selected in Source List.";
    }
    QMessageBox m;
    m.setWindowTitle("Summary Info");
    m.setText(msg);
    m.exec();
}

void MainWindow::on_actionHelpCheck_for_Updates_triggered()
{
    onCheckForUpdates(false);
}

void MainWindow::on_actionThemeAndroidStudio_triggered()
{
    updateTheme(Theme::ANDROID_STUDIO);
}

void MainWindow::on_actionThemeNetBeans_triggered()
{
    updateTheme(Theme::NETBEANS);
}

void MainWindow::on_actionThemeQt_triggered()
{
    updateTheme(Theme::QT_CREATOR);
}

void MainWindow::on_actionThemeMonokaiDarkSoda_triggered()
{
    updateTheme(Theme::DARK_SODA);
}

void MainWindow::on_actionThemeLightTable_triggered()
{
    updateTheme(Theme::LIGHT_TABLE);
}
