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
class ExtraSelection;
class SelItem;


enum Highlighting {
    HlCommon, HlCaretRow, HlError, HlCaretRowCentered
};

//***** CodeEditor *****

class CodeEditor : public QPlainTextEdit {
    Q_OBJECT

public:
    CodeEditor( QWidget *parent=0 );
    ~CodeEditor();

    bool aucompIsVisible();
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
    void highlightLine(int line , Highlighting kind = HlCommon);
    void highlightCurrentLine();

    bool findNext( const QString &findText,bool cased,bool wrap,bool backward=false );
    bool replace( const QString &findText,const QString &replaceText,bool cased );
    int  replaceAll( const QString &findText,const QString &replaceText,bool cased,bool wrap );

    QString identAtCursor(bool fullWord=true);

    Highlighter *highlighter(){ return _highlighter; }
    void updateCodeViews(QTreeView *tree, QListView *view);

    void aucompProcess(CodeItem *item , bool forInheritance=false);
    void aucompShowList(bool process=true , bool inheritance=false);

    void storeBlock( int num=-1 );
    QString cursorRowCol();
    void cursorLineChanged();

    void lineNumberAreaPaintEvent(QPaintEvent *event);
    void pressOnLineNumber(QMouseEvent *e);
    void moveOnLineNumber(QMouseEvent *e);
    void commentUncommentBlock();
    void bookmarkToggle();
    void bookmarkNext();
    void bookmarkPrev();
    void bookmarkFind( int dir, int start=-1 );

    void analyzeCode();
    void updateSourceNavigationByCurrentScope();
    void fillSourceListWidget(CodeItem *item, QStandardItem *si);
    void fillCodeTree();

    QTextBlock foldBlock(QTextBlock block);
    QTextBlock unfoldBlock(QTextBlock block);
    void foldAll();
    void unfoldAll();
    void adjustDocumentSize();
    void goBack();
    void goForward();
    void gotoPos(int blockNum, int posInBlock);
    void autoformatAll();
    void lowerUpperCase(bool lower);
    //
    bool canFindUsages();
    QString findUsages(QTreeWidget *tree);
    void replaceInRange(int from, int to, const QString &text);

    void showDialogAddProperty();

public slots:

    void undo();
    void redo();

    void onTextChanged();
    void onCursorPositionChanged();
    void onPrefsChanged( const QString &name );

    void onCodeTreeViewClicked( const QModelIndex &index );
    void onSourceListViewClicked( const QModelIndex &index );

    void onPaste(const QString &text);//replace quotes by ~q when paste in "{here}"
    void onThemeChanged();

private slots:

    void onCompleteProcess(QListWidgetItem *item=0);
    void onCompleteChangeItem(QListWidgetItem *current, QListWidgetItem *previous);
    void onCompleteFocusOut();
    void onUpdateLineNumberArea(const QRect &, int);

    void onShowAutocompleteList();

signals:
    void keyEscapePressed();
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

    void adjustShowLineNumbers();
    void showToolTip(QPoint pos, QString s, bool nowrap=true);
    void storeCurrentEditPosition(const QTextCursor &cursor);

    ExtraSelection *_selection;
    Highlighter *_highlighter;
    //
    QString _path;
    QString _fileType, _fileName;
    bool _txt, _code, _monkey;
    int _modified;

    ListWidgetComplete *_lcomp;//autocomplete list
    bool _lcompInheritance, _lcompProcess;
    CodeScope _scope;
    QTextBlock _storedBlock;
    bool _blockChangeCursorMethod;
    int _storedBlockNumber;
    LineNumberArea *_lineNumberArea;
    bool _showLineNumbers;
    int _prevCursorPos, _prevTextLen, _prevTextChangedPos;

    QList<int*> _editPosList;
    int _editPosIndex;

    bool _isHighlightLine, _isHighlightWord;
    bool _useAutoBrackets;
    bool _addVoidForMethods;
    int _charsCountForCompletion;

    friend class Highlighter;
};


//***** Highlighter *****

class Highlighter : public QSyntaxHighlighter{
    Q_OBJECT

public:
    Highlighter( CodeEditor *editor );
    ~Highlighter();

    CodeEditor *editor(){ return _editor; }

    enum Formats {
        FormatDefault, FormatIdentifier,
        FormatUserClass, FormatUserClassVar, FormatUserDecl,
        FormatMonkeyClass,
        FormatKeyword, FormatParam, FormatNumber, FormatString, FormatComment
    };
    void setEnabled( bool value ) { _enabled = value; }
    bool isEnabled() { return _enabled; }

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
    QColor _userwordsColor, _userwordsVarColor, _userwordsDeclColor, _paramsColor;
    QColor _commentsColor;
    QColor _highlightColor;

    void setTextFormat(int start, int end, Formats format, bool italic=false);
    bool _enabled;
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

    static BlockData* data(const QTextBlock &block, bool create=false);
    static void flush(QTextBlock &block);

    bool isFoldable() { return _foldable; }
    void setFoldable(bool value) { _foldable = value; }

    bool isFolded(){ return _folded; }
    void setFolded(bool value) { _folded = value; }
    void toggleFold() { _folded = !_folded; }

    CodeItem* code(){ return _code; }
    void setCode(CodeItem *code){ _code = code; }

    void addItem(CodeItem *item){ _items.append(item); }
    CodeItem *item(QString &ident);
    QList<CodeItem*> items() { return _items; }

    int foldType;

private:
    QTextBlock _block, _blockEnd;
    bool _marked, _folded, _foldable;
    int _modified;
    CodeItem *_code;
    QList<CodeItem*> _items;
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
    void setWidth(int value) { _wdth = value; }

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



/*  ExtraSelection */
class ExtraSelection {

public:

    ExtraSelection(CodeEditor *editor);
    ~ExtraSelection();

    void resetAll();
    void resetToolTip();
    void resetWords();
    void resetCaretRow();
    void appendCaretRow();
    void appendCaretRow(QTextCursor &cursor, Highlighting kind);
    void appendToolTip();
    void appendWords(QList<SelItem*> list);
    void setLastWord(QString word, int scroll);
    QList<QTextEdit::ExtraSelection> sels();

    QString lastSelWord(){return _lastSelWord;}
    int lastSelScrollPos(){return _lastSelScrollPos;}
    SelItem* caretRowSel(){return _caretRowSel;}
    SelItem* toolTipSel(){return _toolTipSel;}
    SelItem* wordSel(){return _wordSel;}

    void readPrefs();

private:

    QColor _commonColor, _wordColor, _caretColor, _errorColor, _toolTipColor;
    CodeEditor *_editor;
    QList<SelItem*> _items;
    QList<QTextEdit::ExtraSelection> _sels;
    SelItem *_caretRowSel, *_toolTipSel, *_wordSel;
    QString _lastSelWord;
    int _lastSelScrollPos;

    void adjust(bool isDirty);

};

class SelItem {
public:
    QTextEdit::ExtraSelection selection;
};

#endif // CODEEDITOR_H
