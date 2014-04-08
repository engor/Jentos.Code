/*
Ted, a simple text editor/IDE.

Copyright 2012, Blitz Research Ltd.

See LICENSE.TXT for licensing terms.
*/

#ifndef CODEEDITOR_H
#define CODEEDITOR_H

#include "std.h"
#include "quickhelp.h"
#include "codeanalyzer.h"

class CodeDocument;
class CodeEditor;
class Highlighter;
class Prefs;
class BlockData;
class QuickHelp;
class ListWidgetComplete;
class LineNumberArea;
class ListWidgetCompleteItem;
class ItemWithData;


//***** CodeEditor *****

class CodeEditor : public QPlainTextEdit {
    Q_OBJECT

public:
    CodeEditor( QWidget *parent=0 );
    ~CodeEditor();

    //return true if successful and path updated
    bool open( const QString &path );
    bool save( const QString &path );
    void rename( const QString &path );
    const QString &path(){ return _path; }
    const QString &fileName(){ return _fileName; }
    int modified(){ return _modified; }

    QString fileType(){ return _fileType; }

    bool isTxt(){ return _txt; }
    bool isCode(){ return _code; }
    bool isMonkey(){ return _monkey; }

    void gotoLine( int line );
    void highlightLine(int line , int kind=0);


    bool findNext( const QString &findText,bool cased,bool wrap,bool backward=false );
    bool replace( const QString &findText,const QString &replaceText,bool cased );
    int  replaceAll( const QString &findText,const QString &replaceText,bool cased,bool wrap );

    QString identAtCursor();
    QString identBeforeCursor();
    QString identInLineBefore(const QString &line, int posInLine , int posTotal);

    Highlighter *highlighter(){ return _highlighter; }
    void updateCodeViews(QTreeView *tree, QListView *view);

    QString aucompProcess(ListWidgetCompleteItem *item );
    void aucompShowList(const QString &ident, const QTextCursor &cursor, const QString &kind="", bool process=true );

    void storeBlock( int num=-1 );
    QString cursorRowCol();
    void cursorLineChanged();

    void lineNumberAreaPaintEvent(QPaintEvent *event);
    void pressOnLineNumber(QMouseEvent *e);
    void moveOnLineNumber(QMouseEvent *e);
    void bookmarkToggle();
    void bookmarkNext();
    void bookmarkPrev();
    void bookmarkFind( int dir, int start=-1 );

    void analyzeCode();
    void updateSourceNavigationByCurrentScope();
    void fillSourceListWidget(ItemWithData *item);

    QTextBlock foldBlock(QTextBlock block);
    QTextBlock unfoldBlock(QTextBlock block);
    void foldAll();
    void unfoldAll();
    void adjustDocumentSize();
    void goBack();
    void goForward();
    void autoformatAll();

    static int HL_COMMON;
    static int HL_CARETROW;
    static int HL_ERROR;
    static int HL_CARETROW_CENTERED;



public slots:

    void undo();
    void redo();

    void onTextChanged();
    void onCursorPositionChanged();
    void onPrefsChanged( const QString &name );

    void onCodeTreeViewClicked( const QModelIndex &index );
    void onCodeTreeViewDoubleClicked( const QModelIndex &index );
    void onSourceListWidgetClicked( const QModelIndex &index );
    void onPaste(const QString &text);//replace quotes by ~q when paste in "{here}"

private slots:

    void onCompleteProcess(QListWidgetItem *item);
    void onCompleteChangeItem(QListWidgetItem *current, QListWidgetItem *previous);
    void onCompleteFocusOut();
    void onUpdateLineNumberArea(const QRect &, int);

signals:

    void showCode( const QString &file, int line);
    void statusBarChanged( const QString &text );
    void keyPressed(QKeyEvent *event);
    void openCodeFile( const QString &file, const QString &path, const int &lineNumber );

protected:

    void resizeEvent(QResizeEvent *e);
    void keyPressEvent( QKeyEvent *e );
    void keyReleaseEvent( QKeyEvent *e );
    void mouseMoveEvent(QMouseEvent *e);
    void mousePressEvent(QMouseEvent *e);
    void leaveEvent(QEvent *) {
        QGuiApplication::restoreOverrideCursor();
    }

private:

    void showToolTip(QPoint pos, QString s, bool nowrap=true);

    void editPosInsert(int pos);

    Highlighter *_highlighter;
    QTreeView *_codeTreeView;
    QListView* _sourceListView;
    QStandardItemModel *_codeTreeModel;
    QStandardItemModel *_sourceListModel;
    //
    QString _path;
    QString _fileType, _fileName;
    bool _txt, _code, _monkey;
    int _modified;
    ListWidgetComplete *_lcomp;//autocomplete list
    CodeScope _scope;
    QTextBlock _storedBlock;
    bool _blockChangeCursorMethod;
    int _storedBlockNumber;
    LineNumberArea *_lineNumberArea;
    int _prevCursorPos, _prevTextLen, _prevTextChangedPos;

    int identBeforeStart, identBeforeEnd;
    QString identStr, _aucompKind;

    QList<int> _editPosList;
    int _editPosIndex;

    friend class Highlighter;
};

//***** Highlighter *****

class Highlighter : public QSyntaxHighlighter{
    Q_OBJECT

public:
    Highlighter( CodeEditor *editor );
    ~Highlighter();

    CodeEditor *editor(){ return _editor; }

    bool capitalize( const QTextBlock &block,QTextCursor cursor );

public slots:

    void onPrefsChanged( const QString &name );

protected:

    void highlightBlock( const QString &text );

private:
    CodeEditor *_editor;

    QColor _backgroundColor;
    QColor _defaultColor;
    QColor _numbersColor;
    QColor _stringsColor;
    QColor _identifiersColor;
    QColor _keywordsColor;
    QColor _monkeywordsColor;
    QColor _userwordsColor, _userwordsColorVar;
    QColor _commentsColor;
    QColor _highlightColor;

    QString parseToke( QString &text,QColor &color );

    friend class BlockData;
};


//***** BlockData *****

class BlockData : public QTextBlockUserData {
public:
    BlockData( const QTextBlock &block );
    ~BlockData();

    QTextBlock block(){ return _block; }
    QTextBlock blockEnd(){ return _blockEnd; }
    void setBlockEnd(QTextBlock block){ _blockEnd = block; }

    bool isBookmarked(){ return _marked; }
    void setBookmark(bool mark) { _marked = mark; }
    void toggleBookmark() { _marked = !_marked; }

    int modified(){ return _modified; }
    void setModified(bool value) {
        if(value) {
            _modified += 2;
        }
        else {
            _modified = (_modified>0 ? 1 : 0);
        }
    }

    static BlockData* data(QTextBlock &block, bool create=false);
    static void flush(QTextBlock &block);

    bool isFoldable() { return _foldable; }
    void setFoldable(bool value) { _foldable = value; }

    bool isFolded(){ return _folded; }
    void setFolded(bool value) { _folded = value; }
    void toggleFold() { _folded = !_folded; }

    CodeItem* code(){ return _code; }
    void setCode(CodeItem *code){ _code = code; }


private:
    QTextBlock _block, _blockEnd;
    bool _marked, _folded, _foldable;
    int _modified;
    CodeItem *_code;

};


//***** CodeTreeItem *****

class ItemWithData : public QStandardItem{
public:
    ItemWithData() : _data(0), _code(0){
        setEditable( false );
    }
    ItemWithData(const QIcon &icon, const QString &text) : QStandardItem(icon,text), _data(0), _code(0){
        setEditable( false );
    }
    void setData( BlockData *data ){
        _data = data;
    }
    void setCode( CodeItem *code ){
        _code = code;
    }
    BlockData* data(){
        return _data;
    }
    CodeItem* code(){
        return _code;
    }

private:
    BlockData *_data;
    CodeItem *_code;
};



//----------------------------------------------------------------------------------
class LineNumberArea : public QWidget {
public:

    LineNumberArea(CodeEditor *editor, int width) : QWidget(editor),_wdth(width) {
        _codeEditor = editor;
    }

    int sizeHint() { return _wdth; }

    int maxwidth() { return _wdth; }

    void pressed(int left, int right) { _pressedPosLeft = left; _pressedPosRight = right; }
    int pressedLeft() { return _pressedPosLeft; }
    int pressedRight() { return _pressedPosRight; }

protected:
    void paintEvent(QPaintEvent *event) {
        _codeEditor->lineNumberAreaPaintEvent(event);
    }

    void mousePressEvent(QMouseEvent *e) {
        _codeEditor->pressOnLineNumber(e);
    }

    void mouseMoveEvent(QMouseEvent *e) {
        _codeEditor->moveOnLineNumber(e);
    }

    void enterEvent(QEvent *) {
        QGuiApplication::setOverrideCursor(QCursor(Qt::PointingHandCursor));
    }

    void leaveEvent(QEvent *) {
        QGuiApplication::restoreOverrideCursor();
    }

private:
    CodeEditor *_codeEditor;
    int _wdth;
    int _pressedPosLeft, _pressedPosRight;
};


#endif // CODEEDITOR_H
