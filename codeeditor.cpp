/*
Ted,imple text editor/IDE.

Copyright 2012, Blitz Research Ltd.

See LICENSE.TXT for licensing terms.
*/

#include "codeeditor.h"
#include "listwidgetcomplete.h"

#include "prefs.h"
#include "math.h"
#include "quickhelp.h"
#include "codeanalyzer.h"


static CodeEditor *extraSelsEditor;
static QList<QTextEdit::ExtraSelection> extraSels;

static void flushExtraSels(){
    if( !extraSelsEditor ) return;
    extraSels.clear();
    extraSelsEditor->setExtraSelections( extraSels );
    extraSelsEditor=0;
}

//***** CodeEditor *****

int CodeEditor::HL_COMMON = 0;
int CodeEditor::HL_CARETROW = 1;
int CodeEditor::HL_ERROR = 2;
int CodeEditor::HL_CARETROW_CENTERED = 3;

CodeEditor::CodeEditor( QWidget *parent ):QPlainTextEdit( parent ),_modified( 0 ){

    _editPosIndex = -1;

    _lineNumberArea = new LineNumberArea(this, 60);

    _prevCursorPos = _prevTextLen = _prevTextChangedPos = -1;
    _lcomp = 0;
    _blockChangeCursorMethod = false;
    _storedBlockNumber = -1;

    _highlighter = new Highlighter( this );

    _codeTreeView = 0;
    _codeTreeModel = new QStandardItemModel;
    _sourceListModel = new QStandardItemModel;

    connect( this,SIGNAL(textChanged()),SLOT(onTextChanged()) );
    connect( this,SIGNAL(cursorPositionChanged()),SLOT(onCursorPositionChanged()) );
    connect(this, SIGNAL(updateRequest(QRect,int)), this, SLOT(onUpdateLineNumberArea(QRect,int)));
    connect( Prefs::prefs(),SIGNAL(prefsChanged(const QString&)),SLOT(onPrefsChanged(const QString&)) );

    setLineWrapMode( QPlainTextEdit::NoWrap );

    onPrefsChanged( "" );

    flushExtraSels();

    setViewportMargins(_lineNumberArea->maxwidth()-1, 0, 0, 0);

    horizontalScrollBar()->setStyleSheet("QScrollBar:horizontal {margin: 1px 1px 1px 61px;}");
    setMouseTracking(true);
    setAcceptDrops(false);
}

CodeEditor::~CodeEditor(){

    flushExtraSels();

    CodeAnalyzer::removeUserFile(_path);

    delete _codeTreeModel;
    delete _sourceListModel;
    delete _lcomp;
    _lcomp = 0;
}

void CodeEditor::updateCodeViews(QTreeView *tree, QListView* view) {
    _codeTreeView = tree;
    _sourceListView = view;
    //
    if(tree->model() != _codeTreeModel)
        tree->setModel(_codeTreeModel);
    connect( tree,SIGNAL(clicked(QModelIndex)),SLOT(onCodeTreeViewClicked(QModelIndex)) );
    //
    if(view->model() != _sourceListModel)
        view->setModel(_sourceListModel);
    connect( view, SIGNAL(clicked(QModelIndex)), SLOT(onSourceListWidgetClicked(QModelIndex)) );
}

QString CodeEditor::aucompProcess( ListWidgetCompleteItem *item ) {
    QString line = item->codeItem()->identForInsert();
    if( line == identStr && !item->codeItem()->isClass() && !item->codeItem()->isFunc())
        return item->codeItem()->toString();
    QTextDocument *doc = document();
    QTextCursor cursor( doc );
    if(identStr != "") {
        cursor.setPosition( identBeforeStart );
        cursor.setPosition( identBeforeEnd, QTextCursor::KeepAnchor );
    }
    else {
        cursor.setPosition( identBeforeStart );
    }
    setTextCursor(cursor);
    int blockEndPos = cursor.block().position() + cursor.block().text().length();
    int d = 0;
    if(identBeforeEnd >= blockEndPos) {
        line = item->codeItem()->identWithParamsBraces(d);
    }
    else {
        bool isident = isIdent(doc->characterAt(identBeforeEnd+1));
        if(!isident) {
            line = item->codeItem()->identWithParamsBraces(d);
        }
        else if(item->codeItem()->isFunc()) {
            line += "(";
        }
    }
    insertPlainText( line );
    if( d != 0 ) {
        cursor.setPosition( cursor.position()+d );
        setTextCursor(cursor);
    }
    return item->codeItem()->toString();
}

void CodeEditor::aucompShowList(const QString &ident, const QTextCursor &cursor, const QString &kind, bool process ) {

    if(_lcomp != 0) {
        _lcomp->disconnect();
        delete _lcomp;
    }
    _lcomp = new ListWidgetComplete(this);

    qDebug() << "aucomp for: "+ident+", type: "+kind;

    identStr = ident;
    _aucompKind = kind;

    QString text = ident;
    int i = text.indexOf(".");
    if(i > 0) {
        CodeAnalyzer::findScopeForIdent(ident, cursor, _scope);
        int i2 = text.lastIndexOf(".");
        if(i2 > 0) {
            text = text.mid(i2+1);
            identStr = text;
            identBeforeEnd = cursor.position();
            identBeforeStart = identBeforeEnd-(ident.length()-i2-1);

        }
        if(_scope.item) {
            CodeAnalyzer::fillListByChildren(_lcomp, identStr, _scope);
        }
    }
    else {
        if(kind == "vartype") {
            CodeAnalyzer::fillListVarType(_lcomp, identStr);
            identStr = "";
        }
        else {
            CodeAnalyzer::fillListCommon(_lcomp, identStr, cursor.block());
        }
    }
    //_
    if( _lcomp->count() > 0 ) {
        if( process && _lcomp->count() == 1 ) { //select value if one
            ListWidgetCompleteItem *item = dynamic_cast<ListWidgetCompleteItem*>(_lcomp->item(0));
            aucompProcess(item);
        }
        else {
            int xx=0, yy=0, ww=0, hh=0;
            hh = _lcomp->sizeHintForRow(0)*_lcomp->count()+10;
            if(hh < 60)
                hh = 60;
            ww = _lcomp->sizeHintForColumn(0)+10;
            if(ww > 444) {
                ww = 444;
                hh += 30;
            }
            else if(ww < 200) {
                ww = 200;
            }

            QRect r = cursorRect();
            QRect r2 = this->rect();

            if(hh > 444)
                hh = 444;
            if(hh > r2.height()-20)
                hh = r2.height()-20;

            xx = r.right()+20+60;//60 for left margin
            yy = r.top();
            if( xx+ww > r2.right()-15 ) {
                xx = r2.right()-ww-15;
                yy += 30;
            }
            if( yy < r2.top() )
                yy = r2.top();
            if( yy+hh > r2.bottom()-15 ) {
                yy = r2.bottom()-hh-15;
            }

            _lcomp->setGeometry(xx, yy, ww, hh);
            _lcomp->setCurrentRow(0);
            _lcomp->setVisible(true);
        }
        ListWidgetCompleteItem *lwi = dynamic_cast<ListWidgetCompleteItem*>(_lcomp->item(0));
        emit statusBarChanged( lwi->codeItem()->toString() );
    }
    else {
        onCompleteFocusOut();
    }

    connect( _lcomp,SIGNAL(itemActivated(QListWidgetItem*)),SLOT(onCompleteProcess(QListWidgetItem*)) );
    connect( _lcomp,SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),SLOT(onCompleteChangeItem(QListWidgetItem*,QListWidgetItem*)) );
    connect( _lcomp,SIGNAL(focusOut()),SLOT(onCompleteFocusOut()) );

}

void CodeEditor::onCompleteProcess(QListWidgetItem *item) {
    onCompleteFocusOut();
    if( !item ) {
        item = _lcomp->currentItem();
    }
    ListWidgetCompleteItem *icomp = dynamic_cast<ListWidgetCompleteItem*>(item);
    QString s = aucompProcess(icomp);
    emit statusBarChanged( s );
}

void CodeEditor::onCompleteChangeItem(QListWidgetItem *current, QListWidgetItem *previous) {
    ListWidgetCompleteItem *lwi = dynamic_cast<ListWidgetCompleteItem*>(current);
    emit statusBarChanged( lwi->codeItem()->toString() );
}

void CodeEditor::onCompleteFocusOut() {
    if( _lcomp )
        _lcomp->hide();
    setFocus();
    _scope.set(0);
}

bool CodeEditor::open( const QString &path ){

    QFile file( path );

    if( !file.open( QIODevice::ReadOnly ) ) return false;

    rename( path );

    QTextStream stream( &file );

    stream.setCodec( "UTF-8" );

    setPlainText( stream.readAll() );

    file.close();

    document()->setModified( false );
    _modified=0;


    return true;
}

bool CodeEditor::save( const QString &path ){

    QFile file( path );

    if( !file.open( QIODevice::WriteOnly | QIODevice::Truncate ) ) return false;

    rename( path );

    QTextStream stream( &file );

    stream.setCodec( "UTF-8" );

    stream<<toPlainText();

    stream.flush();
    file.close();

    document()->setModified( false );
    _modified=0;

    //flush all 'modified' marks
    QTextBlock block = document()->firstBlock();
    while( block.isValid() ) {
        BlockData *data = BlockData::data(block);
        if(data) {
            data->setModified(false);
        }
        block = block.next();
    }
    _prevTextLen = textCursor().position();
    _prevTextLen = document()->characterCount();

    return true;
}

void CodeEditor::rename( const QString &path ){

    _path = path;
    _fileName = stripDir(path);
    _fileType=extractExt( _path ).toLower();
    QString t=';'+_fileType+';';

    _txt = textFileTypes.contains( t );
    _code = codeFileTypes.contains( t );
    _monkey = _fileType=="monkey";

    if( _txt ){
        setLineWrapMode( QPlainTextEdit::WidgetWidth );
    }
    else {
        setLineWrapMode( QPlainTextEdit::NoWrap );
    }
}

void CodeEditor::gotoLine( int line ){

    setCenterOnScroll( true );
    setTextCursor( QTextCursor( document()->findBlockByNumber( line ) ) );
    setCenterOnScroll( false );
}

QString CodeEditor::cursorRowCol() {
    QTextCursor cursor = textCursor();
    if( cursor.isNull() )
        return "";
    int row = cursor.blockNumber()+1;
    int col = cursor.positionInBlock()+1;
    return QString::number(row) + " : " + QString::number(col)+" ";
}

void CodeEditor::cursorLineChanged() {
    onCompleteFocusOut();
    if( _blockChangeCursorMethod ) {
        _blockChangeCursorMethod = false;
        return;
    }
    if(isTxt())
        return;
    //autoformat line
    //replace # -> :Float, % -> :Int, etc...
    QTextCursor cursor = textCursor();
    QTextBlock b = _storedBlock;
    QString s0 = b.text();
    QString s = s0;
    bool repl = CodeAnalyzer::autoFormat(s);
    if( repl && s != s0 ) {
        int len = s0.length();
        int cursel1 = cursor.selectionStart();
        int cursel2 = cursor.selectionEnd();
        cursor.setPosition( b.position() );
        cursor.setPosition( b.position()+len, QTextCursor::KeepAnchor );
        setTextCursor( cursor );
        _blockChangeCursorMethod = true;
        insertPlainText( s );
        int delta = s.length()-len;
        if(cursel1 == cursel2) {
            cursel1 += delta;
        }
        cursor.setPosition( cursel1 );
        if(cursel1 != cursel2) {
            cursel2 += delta;
            cursor.setPosition( cursel2, QTextCursor::KeepAnchor );
        }
        setTextCursor( cursor );
    }
    updateSourceNavigationByCurrentScope();
}

void CodeEditor::undo() {
    _blockChangeCursorMethod = true;
    QPlainTextEdit::undo();
}

void CodeEditor::redo() {
    _blockChangeCursorMethod = true;
    QPlainTextEdit::redo();
}

void CodeEditor::highlightLine( int line, int kind ){

    flushExtraSels();

    QTextBlock block = document()->findBlockByNumber( line );

    if( block.isValid() ){

        QTextEdit::ExtraSelection selection;

        QColor lineColor;
        if( kind == HL_COMMON) {
            lineColor = Prefs::prefs()->getColor( "highlightColor" );
        }
        else if( kind == HL_CARETROW || kind == HL_CARETROW_CENTERED ) {
            lineColor = Prefs::prefs()->getColor( "highlightColorCaretRow" );
        }
        else if( kind == HL_ERROR ) {
            lineColor = Prefs::prefs()->getColor( "highlightColorError" );
        }
        selection.format.setBackground( lineColor );
        selection.format.setProperty( QTextFormat::FullWidthSelection,true );

        QTextCursor cursor = QTextCursor( block );
        if( kind == HL_COMMON || kind == HL_ERROR || kind == HL_CARETROW_CENTERED ) {
            setCenterOnScroll( true );
            cursor.setPosition(block.position());
            setTextCursor(cursor);
            setCenterOnScroll( false );
        }
        selection.cursor = cursor;
        selection.cursor.clearSelection();

        extraSels.append( selection );
    }
    setExtraSelections( extraSels );
    extraSelsEditor=this;
}

void CodeEditor::storeBlock(int num) {
    if( num == -1 ) {
        _storedBlock = textCursor().block();
        _storedBlockNumber = textCursor().blockNumber();
    }
    else {
        _storedBlock = document()->findBlockByLineNumber( num );
        _storedBlockNumber = num;
    }
}

void CodeEditor::onPrefsChanged( const QString &name ){

    QString t( name );

    Prefs *prefs=Prefs::prefs();

    if( t=="" || t=="backgroundColor" || t=="fontFamily" || t=="fontSize" || t=="tabSize" || t=="smoothFonts" ){

        QColor bg=prefs->getColor( "backgroundColor" );
        QColor fg( 255-bg.red(),255-bg.green(),255-bg.blue() );

        QPalette p=palette();
        p.setColor( QPalette::Base,bg );
        p.setColor( QPalette::Text,fg );
        setPalette( p );

        QFont font;
        font.setFamily( prefs->getString( "fontFamily" ) );
        font.setPixelSize( prefs->getInt( "fontSize" ) );

        if( prefs->getBool( "smoothFonts" ) ){
            font.setStyleStrategy( QFont::PreferAntialias );
        }else{
            font.setStyleStrategy( QFont::NoAntialias );
        }

        setFont( font );

        QFontMetrics fm( font );
        setTabStopWidth( fm.width( 'X' )* prefs->getInt( "tabSize" ) );
    }
}

void CodeEditor::onCursorPositionChanged(){
    QTextCursor cursor = textCursor();
    if( cursor.isNull() )
        return;
    int row = cursor.blockNumber();
    highlightLine( row, HL_CARETROW );
    if( row != _storedBlockNumber ) {
        if( _storedBlockNumber == -1 )
            storeBlock( 0 );
        cursorLineChanged();
    }
    storeBlock();

    //mark blocks as modified
    int pos = cursor.position();
    int len = document()->characterCount();
    if( len != _prevTextLen) {
        if(_prevCursorPos >= 0) {//not mark if it's 'open document' action
            cursor.setPosition(_prevCursorPos);
            QTextBlock block = cursor.block();
            while(block.isValid() && block.position() <= pos) {
                BlockData *data = BlockData::data(block, true);
                data->setModified(true);
                block = block.next();
            }
            cursor.setPosition(pos);
            editPosInsert(cursor.blockNumber());
        }
        _prevTextLen = len;
    }
    _prevCursorPos = pos;
}

void CodeEditor::onTextChanged(){

    if( document()->isModified() ){
        ++_modified;
    }else{
        _modified=0;
    }


    QTextCursor cursor = textCursor();
    int pos = cursor.position();
    //for del & cut when cursor pos not changed, need mark modified blocks manually
    if(pos == _prevCursorPos)
        onCursorPositionChanged();

    _prevTextChangedPos = pos;

}

void CodeEditor::onSourceListWidgetClicked( const QModelIndex &index ) {
    ItemWithData *item = dynamic_cast<ItemWithData*>( _sourceListModel->itemFromIndex( index ) );
    if( !item )
        return;
    QTextBlock b = item->code()->block();
    if(b.isValid()) {
        if(!b.isVisible()) {
            CodeItem *i = CodeAnalyzer::findScopeForBlock(b, true);
            if(i) {
                unfoldBlock(i->block());
            }
        }
        QString file = item->code()->filepath();
        emit showCode( file, b.blockNumber() );
    }
}

void CodeEditor::onCodeTreeViewClicked( const QModelIndex &index ){

    ItemWithData *item = dynamic_cast<ItemWithData*>( _codeTreeModel->itemFromIndex( index ) );
    if( !item || !item->code() )
        return;

    fillSourceListWidget(item);

    QTextBlock b = item->code()->block();
    if(b.isValid()) {
        _blockChangeCursorMethod = true;
        if(!b.isVisible()) {
            CodeItem *i = CodeAnalyzer::findScopeForBlock(b, true);
            if(i) {
                unfoldBlock(i->block());
            }
        }
        QString file = item->code()->filepath();
        emit showCode( file, b.blockNumber() );
    }
}

void CodeEditor::onCodeTreeViewDoubleClicked( const QModelIndex &index ){
    /*
    ItemWithData *item = dynamic_cast<ItemWithData*>( _codeTreeModel->itemFromIndex( index ) );
    if( !item || !item->code() )
        return;
    if(item->code()->block().isValid()) {
        emit showCode( path(), item->code()->block().blockNumber() );
    }*/
}

void CodeEditor::onUpdateLineNumberArea(const QRect &rect, int dy) {
    if (dy) {
        _lineNumberArea->scroll(0, dy);
    }
    else {
        _lineNumberArea->update(rect.x()+1, rect.y()+1, _lineNumberArea->maxwidth(), rect.height()-1+20);
    }

    if (rect.contains(viewport()->rect())) {
        _lineNumberArea->setGeometry(rect.x()+1,rect.y()+1, _lineNumberArea->maxwidth(),rect.height()-1+20);
    }

}

void CodeEditor::editPosInsert(int pos) {
    if(_editPosList.isEmpty()) {
        _editPosList.append(pos);
        _editPosIndex = 0;
    }
    else if(_editPosList.value(_editPosIndex) != pos) {
        ++_editPosIndex;
        _editPosList.insert(_editPosIndex, pos);
    }
}

void CodeEditor::goBack() {
    if(_editPosList.isEmpty())
        return;
    int pos = -1;
    if(_editPosList.length() > 1 && _editPosIndex > 0) {
        --_editPosIndex;
        pos = _editPosList.value(_editPosIndex);
    }
    else {
        pos = _editPosList.first();
    }
    QTextBlock b = document()->findBlockByNumber(pos);
    if(b.isValid()) {

        CodeItem *i = CodeAnalyzer::findScopeForBlock(b);
        if(i) {
            unfoldBlock(i->block());
            if(i->parent())
                unfoldBlock(i->parent()->block());
        }

        QTextCursor c = textCursor();
        c.setPosition(b.position());
        setTextCursor(c);

        ensureCursorVisible();
        setCenterOnScroll(true);
    }
}
void CodeEditor::goForward() {
    if(_editPosList.isEmpty())
        return;
    int pos = -1;
    int len = _editPosList.length();
    if(len > 1 && _editPosIndex < len-1) {
        ++_editPosIndex;
        pos = _editPosList.value(_editPosIndex);
    }
    else {
        pos = _editPosList.first();
    }
    QTextBlock b = document()->findBlockByNumber(pos);
    if(b.isValid()) {

        CodeItem *i = CodeAnalyzer::findScopeForBlock(b);
        if(i) {
            unfoldBlock(i->block());
            if(i->parent())
                unfoldBlock(i->parent()->block());
        }

        QTextCursor c = textCursor();
        c.setPosition(b.position());
        setTextCursor(c);

        ensureCursorVisible();
        setCenterOnScroll(true);
    }
}

void CodeEditor::foldAll() {
    QTextBlock b, block = document()->firstBlock();
    while(block.isValid()) {
        if(!block.isVisible()) {
            block = block.next();
            continue;
        }
        b = block;
        BlockData *data = BlockData::data(block);
        if(data && data->isFoldable())
            b = foldBlock(block);
        block = b.next();
    }
}

void CodeEditor::unfoldAll() {
    QTextBlock b, block = document()->firstBlock();
    while(block.isValid()) {
        b = block;
        BlockData *data = BlockData::data(block);
        if(data && data->isFoldable())
            b = unfoldBlock(block);
        block = b.next();
    }
}

void CodeEditor::autoformatAll() {
    QTextCursor c = textCursor();
    c.beginEditBlock();
    int b = c.blockNumber();
    QTextBlock block = document()->firstBlock();
    while(block.isValid()) {
        QString s = block.text();
        c.setPosition(block.position());
        c.setPosition(block.position()+s.length(), QTextCursor::KeepAnchor);
        bool r = CodeAnalyzer::autoFormat(s);
        if(r) {
            setTextCursor(c);
            insertPlainText(s);

        }
        block = block.next();
    }
    block = document()->findBlockByLineNumber(b);
    if(block.isValid()) {
        c.setPosition(block.position());
        setTextCursor(c);
    }
    c.endEditBlock();
}

void CodeEditor::adjustDocumentSize() {
    document()->adjustSize();
}

QTextBlock CodeEditor::foldBlock(QTextBlock block) {
    BlockData *data = BlockData::data(block);
    if( !data || !data->isFoldable())
        return block;
    data->setFolded(true);
    block = block.next();
    while(block.isValid()) {
        block.setVisible(false);
        if(block == data->blockEnd())
            break;
        block = block.next();
    }
    adjustDocumentSize();
    return data->blockEnd();
}

QTextBlock CodeEditor::unfoldBlock(QTextBlock block) {
    BlockData *data = BlockData::data(block);
    if( !data || !data->isFoldable())
        return block;
    data->setFolded(false);
    block = block.next();
    while(block.isValid()) {
        block.setVisible(true);
        BlockData *d = BlockData::data(block);
        if( d && d->isFoldable() && d->isFolded()) {
            block = foldBlock(block);
        }
        if(block == data->blockEnd())
            break;
        block = block.next();
    }
    adjustDocumentSize();
    return data->blockEnd();
}

void CodeEditor::pressOnLineNumber(QMouseEvent *e) {
    _lineNumberArea->pressed(-1,-1);
    QPoint p = e->pos();
    int px = p.x();
    p.setX(px+100);
    QTextCursor cursor = cursorForPosition(p);
    if( e->button() == Qt::LeftButton ) {
        QTextBlock block = cursor.block();
        //select lines in textarea
        if(px < _lineNumberArea->width()-12) {
            int l = block.position();
            int r = l+block.length();
            _lineNumberArea->pressed(l,r);
            cursor.setPosition(l);
            cursor.setPosition(r, QTextCursor::KeepAnchor);
            setTextCursor(cursor);
        }
        else {//try to fold/unfold block
            BlockData *data = BlockData::data(block);
            if( data ) {
                highlightLine(cursor.blockNumber());
                if(data->isFoldable()) {
                    if( data->isFolded() ) {
                        unfoldBlock(block);
                    }
                    else {
                        foldBlock(block);
                    }
                }
            }
        }

    }
    else if( e->button() == Qt::RightButton ) {
        highlightLine(cursor.blockNumber());
        bookmarkToggle();
    }
}

void CodeEditor::moveOnLineNumber(QMouseEvent *e) {
    int left = _lineNumberArea->pressedLeft();
    if( left >= 0 ) {
        QPoint p = e->pos();
        p.setX(p.x()+100);
        QTextCursor cursor = cursorForPosition(p);
        QTextBlock block = cursor.block();
        int pos = block.position();
        if(pos != left ) {
            if(pos > left) {
                cursor.setPosition(left);
                cursor.setPosition(pos+block.length(), QTextCursor::KeepAnchor);
            }
            else {
                cursor.setPosition(_lineNumberArea->pressedRight());
                cursor.setPosition(pos, QTextCursor::KeepAnchor);
            }
            setTextCursor(cursor);
        }
    }
}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event) {
    static QColor clrGreen(25,165,15);
    static QColor clrGray(0x313334);
    static QImage imgBookmark(":/ui/bookmark.png");
    static QImage imgPlus(":/ui/unfold.png");
    static QImage imgMinus(":/ui/fold.png");
    QPainter painter(_lineNumberArea);
    painter.fillRect(event->rect(), clrGray);

    QTextBlock block = firstVisibleBlock();
    int hg = fontMetrics().height();
    int top = 0;
    if(block.blockNumber() == 0)
        top = 4;
    int bottom = top + (int) blockBoundingRect(block).height();
    painter.setPen(QColor(100,100,100));
    int width = _lineNumberArea->width();
    int wd = 40;
    CodeItem *codeClass = 0, *codeFunc = 0, *codeRem = 0, *codeFold = 0;
    while (block.isValid() && top <= event->rect().bottom()) {
        //
        if (block.isVisible() && bottom >= event->rect().top()) {
            BlockData *data = dynamic_cast<BlockData*>(block.userData());

            if( data && data->isBookmarked() ) {
                painter.drawImage(wd-imgBookmark.width(), top+1, imgBookmark);
            }
            else {
                int num = block.blockNumber() + 1 ;
                QString number = QString::number(num);
                painter.setPen(QColor(130,130,130));
                painter.drawText(0, top, wd, hg, Qt::AlignRight, number);
            }
            if(  data && data->modified() > 0 ) {
                painter.fillRect(wd+5, top, 2, hg, (data->modified()==1 ? Qt::darkGray : clrGreen) );
            }
            //
            CodeItem *i = CodeAnalyzer::findScopeForBlock(block);
            if(i) {
                if(i->isClass()) {
                    codeClass = i;
                }
                else {
                    codeFunc = i;
                    codeClass = codeFunc->parent();
                }
            }
            i = CodeAnalyzer::findRemForBlock(block);
            if(i) {
                codeRem = i;
            }
            i = CodeAnalyzer::findFoldForBlock(block);
            if(i) {
                codeFold = i;
            }
            //
            if(codeClass) {
                QPen pen0 = painter.pen();
                QPen pen;
                pen.setStyle(Qt::DotLine);
                pen.setColor(QColor(85,85,85));
                painter.setPen(pen);
                QLineF line;
                int px = width-7;
                int h = (int) blockBoundingRect(block).height();
                if(block == codeClass->blockEnd()) {
                    line.setLine(px,top+h/2,width,top+h/2);
                    painter.drawLine(line);
                }
                else if( !data || (!data->isFolded() || data->code()->isFunc() ) ){
                    line.setLine(px,top+h/2,px,top+h+h/2);
                    painter.drawLine(line);
                }
                painter.setPen(pen0);
            }
            if(codeFunc) {
                painter.setPen(QColor(85,85,85));
                QLineF line;
                int px = width-7;
                int h = (int) blockBoundingRect(block).height();
                if(block == codeFunc->blockEnd()) {
                    line.setLine(px,top+h/2,width,top+h/2);
                    painter.drawLine(line);
                }
                else if( !data || !data->isFolded() ){
                    line.setLine(px,top+h/2,px,top+h+h/2);
                    painter.drawLine(line);
                }
            }
            if(codeRem) {
                QPen pen0 = painter.pen();
                QPen pen;
                pen.setStyle(Qt::DotLine);
                pen.setColor(QColor(85,85,85));
                painter.setPen(pen);
                QLineF line;
                int px = width-7;
                int h = (int) blockBoundingRect(block).height();
                if(block == codeRem->blockEnd()) {
                    line.setLine(px,top+h/2,width,top+h/2);
                    painter.drawLine(line);
                }
                else if( !data || !data->isFolded() ){
                    line.setLine(px,top+h/2,px,top+h+h/2);
                    painter.drawLine(line);
                }
                painter.setPen(pen0);
            }
            if(codeFold) {
                QPen pen0 = painter.pen();
                QPen pen;
                pen.setStyle(Qt::DotLine);
                pen.setColor(QColor(85,85,85));
                painter.setPen(pen);
                QLineF line;
                int px = width-7;
                int h = (int) blockBoundingRect(block).height();
                if(block == codeFold->blockEnd()) {
                    line.setLine(px,top+h/2,width,top+h/2);
                    painter.drawLine(line);
                }
                else if( !data || !data->isFolded() || data->code() != codeFold ){
                    line.setLine(px,top+h/2,px,top+h+h/2);
                    painter.drawLine(line);
                }
                painter.setPen(pen0);
            }
            //
            if( data && data->isFoldable() ) {
                if(data->isFolded()) {
                    painter.drawImage(width-11, top+3, imgPlus );
                }
                else {
                    painter.drawImage(width-11, top+3, imgMinus );
                }
            }
        }
        //
        if(codeClass && block == codeClass->blockEnd()) {
            codeClass = 0;
        }
        if(codeFunc && block == codeFunc->blockEnd()) {
            codeFunc = 0;
        }
        if(codeRem && block == codeRem->blockEnd()) {
            codeRem = 0;
        }
        if(codeFold && block == codeFold->blockEnd()) {
            codeFold = 0;
        }
        //
        block = block.next();
        top = bottom;
        bottom += (int) blockBoundingRect(block).height();
    }
    QWidget::paintEvent(event);
}

void CodeEditor::mousePressEvent(QMouseEvent *e) {
    if( (e->button() & Qt::LeftButton) && (e->modifiers() & Qt::ControlModifier) ) {
        QTextCursor cursor = cursorForPosition(e->pos());
        cursor.select(QTextCursor::WordUnderCursor);
        QString word = cursor.selectedText();
        CodeItem *item = 0;
        if( !word.isEmpty() ) {
            int sel1 = cursor.selectionStart();
            if(sel1 > 0 && document()->characterAt(sel1-1) == '.') {
                int pos = cursor.position();
                cursor.setPosition(sel1-1);
                setTextCursor(cursor);
                QString ident = identBeforeCursor();
                qDebug() << "search ident: "+ident;
                cursor.setPosition(pos);
                setTextCursor(cursor);
                if(!ident.isEmpty()) {
                    item = CodeAnalyzer::itemAtBlock(ident, cursor.block());
                    if(item) {
                        item = item->child("", word);
                    }
                }
            }
            if(!item)
                item = CodeAnalyzer::itemAtBlock(word, cursor.block());
            if( item && !item->isKeyword() ) {
                editPosInsert(cursor.blockNumber());//save 'from' position
                emit openCodeFile( item->filepath(),"", item->lineNumber()-1 );
                e->accept();
                cursor = textCursor();
                editPosInsert(cursor.blockNumber());//save 'to' position
                return;
            }
        }
    }
    QPlainTextEdit::mousePressEvent(e);
}

void CodeEditor::showToolTip(QPoint pos, QString s, bool nowrap) {
    if(nowrap)
        s = "<p style='white-space:pre'>"+s+"</p>";
    QToolTip::showText(pos, s);
}

void CodeEditor::mouseMoveEvent(QMouseEvent *e) {
    if( (e->modifiers() & Qt::ControlModifier) ) {
        QTextCursor cursor = cursorForPosition(e->pos());
        cursor.select(QTextCursor::WordUnderCursor);
        flushExtraSels();
        QTextBlock block = cursor.block();
        if( block.isValid() ){
            QTextEdit::ExtraSelection selection;
            static QColor clr(240,240,240);
            selection.format.setForeground( clr );
            selection.format.setFontUnderline( true );
            selection.format.setFontWeight( QFont::Bold );
            selection.cursor = cursor;
            extraSels.append( selection );
        }
        setExtraSelections( extraSels );
        extraSelsEditor = this;
        QString word = cursor.selectedText();
        if( !word.isEmpty() ) {
            CodeItem *item = 0;
            int sel1 = cursor.selectionStart();
            if(sel1 > 0 && document()->characterAt(sel1-1) == '.') {
                int pos = cursor.position();
                cursor.setPosition(sel1-1);
                setTextCursor(cursor);
                QString ident = identBeforeCursor();
                cursor.setPosition(pos);
                setTextCursor(cursor);
                if(!ident.isEmpty()) {
                    item = CodeAnalyzer::itemAtBlock(ident, cursor.block());
                    if(item) {
                        item = item->child("", word);
                    }
                }
            }
            if(!item)
                item = CodeAnalyzer::itemAtBlock(word, cursor.block());
            if( item ) {
                QString s = CodeAnalyzer::toolTip(item);
                if( !s.isEmpty() ) {
                    showToolTip(e->globalPos(), s);
                    e->accept();
                    return;
                }
            }
        }
    }
    QToolTip::hideText();
    QPlainTextEdit::mouseMoveEvent(e);

}

void CodeEditor::resizeEvent(QResizeEvent *e) {
    QPlainTextEdit::resizeEvent(e);
    if( _lineNumberArea) {
        _lineNumberArea->setGeometry(1,_lineNumberArea->rect().y()+1, _lineNumberArea->width(),e->size().height()-1);
    }
}

void CodeEditor::keyReleaseEvent( QKeyEvent *event ) {
    highlightLine( textCursor().blockNumber(), HL_CARETROW );
    QGuiApplication::restoreOverrideCursor();
}

void CodeEditor::bookmarkToggle() {
    QTextBlock block = textCursor().block();
    if( block.isValid() ) {
        BlockData *data = BlockData::data(block, true);
        data->toggleBookmark();
    }
}

void CodeEditor::bookmarkPrev() {
    bookmarkFind(-1);
}

void CodeEditor::bookmarkNext() {
    bookmarkFind(1);
}

void CodeEditor::bookmarkFind(int dir , int start) {
    int line = -1;
    QTextBlock block;
    if(start == -1)
        block = textCursor().block();
    else
        block = document()->findBlockByLineNumber(start);
    if( !block.isValid() )
        return;
    int startFrom = block.blockNumber();
    block = (dir == 1 ? block.next() : block.previous());
    while( block.isValid() ) {
        BlockData *data = BlockData::data(block);
        if( data && data->isBookmarked() ) {
            line = block.blockNumber();
            break;
        }
        block = (dir == 1 ? block.next() : block.previous());
    }
    //do cyclic search
    if(line < 0) {
        int i = (dir == 1 ? 0 : document()->blockCount()-1);
        block = document()->findBlockByLineNumber(i);
        while( block.isValid() && block.blockNumber() != startFrom ) {
            BlockData *data = dynamic_cast<BlockData*>(block.userData());
            if( data && data->isBookmarked() ) {
                line = block.blockNumber();
                break;
            }
            block = (dir == 1 ? block.next() : block.previous());
        }
    }
    if(line >= 0) {
        QTextBlock b = document()->findBlockByNumber( line );
        CodeItem *i = CodeAnalyzer::findScopeForBlock(b);
        if(i) {
            unfoldBlock(i->block());
            if(i->parent())
                unfoldBlock(i->parent()->block());
        }
        //
        highlightLine(line,HL_CARETROW_CENTERED );
    }
}

void CodeEditor::analyzeCode() {
    //update analyzer
    bool res = CodeAnalyzer::parse(document()->firstBlock(), _path);
    if(res) {//
        qDebug() << "code analyze";
        CodeAnalyzer::fillTree(_codeTreeModel);
        _highlighter->rehighlight();
        this->repaint();
    }
    if(_codeTreeView) {
        QList<QStandardItem*> list = _codeTreeModel->findItems(_fileName);
        if(!list.isEmpty())
            _codeTreeView->expand(list.first()->index());
    }
}

void CodeEditor::updateSourceNavigationByCurrentScope() {
    CodeItem *code = CodeAnalyzer::findScopeForBlock(textCursor().block());
    if(code) {
        fillSourceListWidget(code->itemWithData());
        if(code->parent())
            code = code->parent();
        _codeTreeView->setCurrentIndex(code->itemWithData()->index());
    }
}

void CodeEditor::fillSourceListWidget(ItemWithData *item) {
    _sourceListModel->clear();
    QString selFunc = "";
    if(item->code()->parent() != 0) {
        selFunc = item->code()->descrAsItem();
        item = dynamic_cast<ItemWithData*>(item->parent());
    }
    ItemWithData *ilist;
    ItemWithData *itree;

    for(int r = 0; r < item->rowCount(); ++r) {
        itree = dynamic_cast<ItemWithData*>(item->child(r));
        ilist = new ItemWithData;
        QString t = itree->text();
        QString decl = itree->code()->decl();
        if(decl == "function")
            t = "---"+t;
        else if(decl == "method")
            t = "--"+t;
        else if(decl == "global")
            t = "-"+t;
        ilist->setText(t);
        ilist->setIcon(itree->icon());
        ilist->setToolTip(itree->toolTip());
        ilist->setData(itree->data());
        ilist->setCode(itree->code());
        _sourceListModel->appendRow(ilist);
    }
    _sourceListModel->sort(0);
    for(int r = 0; r < item->rowCount(); ++r) {
        QStandardItem *i = _sourceListModel->item(r);
        QString t = i->text();
        if(t.startsWith("---"))
            t = t.mid(3);
        else if(t.startsWith("--"))
            t = t.mid(2);
        else if(t.startsWith("-"))
            t = t.mid(1);
        i->setText(t);
        if(t == selFunc)
            _sourceListView->setCurrentIndex(i->index());
    }
    ilist = new ItemWithData;
    ilist->setText(" :: "+item->code()->descr()+" :: ");
    ilist->setIcon(item->icon());
    ilist->setToolTip(item->toolTip());
    ilist->setData(item->data());
    ilist->setCode(item->code());
    _sourceListModel->insertRow(0,ilist);

}


void CodeEditor::keyPressEvent( QKeyEvent *e ) {

    int key = e->key();
    bool ctrl = (e->modifiers() & Qt::ControlModifier);
    bool shift = (e->modifiers() & Qt::ShiftModifier);

    //ctrl + s
    if(ctrl && key == 83){
        return;
    }
    //ctrl + z
    if(ctrl && key == 90) {
        undo();
        return;
    }
    //ctrl + y
    if(ctrl && key == 89) {
        redo();
        return;
    }

    QTextCursor cursor = textCursor();
    QTextBlock block = cursor.block();
    bool hasSel = cursor.hasSelection();

    //autocomplete for "",'',(),[]
    QString evtxt = e->text();
    bool k1 = (evtxt == "\"");
    bool k2 = false;//(evtxt == "'");
    bool k3 = (evtxt == "(");
    bool k4 = (evtxt == "[");

    if(k1 || k2 || k3 || k4 ) {
        QString s = block.text();
        int len = s.length();
        int p = cursor.positionInBlock();
        QChar c1, c2;
        if(p > 0)
            c1 = s.at(p-1);
        if(p < len)
            c2 = s.at(p);
        bool skip = false;
        if(!c1.isNull()) {
            skip = (k1 && c1 == '\"') || (k2 && c1 == 39);
        }
        if(k1) {//calc quotes count
            int i = -1, cnt = 0;
            while( (i = s.indexOf("\"", i+1)) >= 0)
                ++cnt;
            skip |= (cnt % 2 == 1);
        }
        if(!skip && !c2.isNull()) {
            skip = (k1 && c2 == '\"') || (k2 && c2 == 39) || (k3 && c2 == ')') || (k4 && c2 == ']');
            skip |= isIdent(c2);
        }
        if(!skip) {
            if(k1)
                insertPlainText("\"\"");
            else if(k2)
                insertPlainText("''");
            else if(k3)
                insertPlainText("()");
            else if(k4)
                insertPlainText("[]");
            cursor.setPosition(cursor.position()-1);
            setTextCursor(cursor);
            e->accept();
            return;
        }
    }


    //dot, check for ClassInstance.{fields,funcs}
    if(key == 46) {
        insertPlainText(".");
        QString s = identBeforeCursor();
        if( s != "" ) {
            identBeforeStart = identBeforeEnd = cursor.position();
            aucompShowList(s, cursor, "class", false);
        }
        e->accept();
        return;
    }

    //insert code completion by template
    if(key == Qt::Key_Tab) {
        QString s = identBeforeCursor();
        s = CodeAnalyzer::templateWord(s);
        if(!s.isNull()) {
            int i = s.indexOf("%cursor%");
            if(i >= 0) {
                s = s.replace("%cursor%","");
                i += identBeforeStart;
            }
            cursor.setPosition(identBeforeStart,QTextCursor::KeepAnchor);
            setTextCursor(cursor);
            onPaste(s);
            if(i >= 0) {
                cursor.setPosition(i,QTextCursor::MoveAnchor);
                setTextCursor(cursor);
            }
            e->accept();
            return;
        }
    }

    //delete
    if(hasSel && (key == Qt::Key_Delete || (ctrl && key == Qt::Key_X) || (!ctrl && key == Qt::Key_Backspace))) {
        /**/
    }

    //
    if( ctrl && this->underMouse() ) {
        QGuiApplication::setOverrideCursor(QCursor(Qt::PointingHandCursor));
    }


    //select word in autocomplete list
    if( _lcomp && _lcomp->isVisible()) {
        if( key == Qt::Key_Up ) {
            _lcomp->selectNear(-1);
            e->accept();
            return;
        }
        else if( key == Qt::Key_Down ) {
            _lcomp->selectNear(1);
            e->accept();
            return;
        }
        else if( key == Qt::Key_Return ) {
            onCompleteProcess(0);
            e->accept();
            return;
        }
    }

    highlightLine( textCursor().blockNumber(), HL_CARETROW );

    flushExtraSels();

    bool checkNew = false;

    //Ctrl + v
    if ( (ctrl && key == Qt::Key_V) || (shift && key == Qt::Key_Insert) ) {
        onPaste(QApplication::clipboard()->text());
        e->accept();
        return;
    }
    //Ctrl + space
    else if( ctrl && key == Qt::Key_Space ) {
        QString ident = identBeforeCursor();
        aucompShowList( ident, cursor );
        e->accept();
        return;
    }
    else if( key == Qt::Key_Tab || key == Qt::Key_Backtab ){
        //block tab/untab
        if( hasSel ) {
            QTextBlock anchor = document()->findBlock( cursor.anchor() );
            int beg, end;
            if( block < anchor ){
                beg = block.blockNumber();
                end = document()->findBlock( cursor.anchor()-1 ).blockNumber();
            }else{
                beg = anchor.blockNumber();
                end = document()->findBlock( cursor.position()-1 ).blockNumber();
            }
            if( end < beg )
                end = beg;
            cursor.beginEditBlock();
            if( key==Qt::Key_Backtab || shift ){
                for( int i=beg;i<=end;++i ) {
                    QTextBlock block=document()->findBlockByNumber( i );
                    if( !block.length() ) continue;
                    cursor.setPosition( block.position() );
                    cursor.setPosition( block.position()+4,QTextCursor::KeepAnchor );
                    if( cursor.selectedText() != "    " ) continue;
                    cursor.insertText( "" );
                }

            }
            else {
                if( beg != end) {
                    for( int i=beg;i<=end;++i ){
                        QTextBlock block=document()->findBlockByNumber( i );
                        cursor.setPosition( block.position() );
                        cursor.insertText("    ");
                    }
                }
                else {
                    QTextBlock block=document()->findBlockByNumber( beg );
                    int p = cursor.position();
                    cursor.setPosition( block.position() );
                    cursor.insertText( "    " );
                    cursor.setPosition( p+4 );
                }
            }
            //does editing doc invalidated blocks?
            if( beg != end ) {
                QTextBlock block0=document()->findBlockByNumber( beg );
                QTextBlock block1=document()->findBlockByNumber( end );
                cursor.setPosition( block0.position() );
                cursor.setPosition( block1.position()+block1.length(),QTextCursor::KeepAnchor );
            }
            cursor.endEditBlock();
            setTextCursor( cursor );
        }
        else {
            insertPlainText("    ");
        }
        e->accept();
        return;
    }
    else if( key==Qt::Key_Enter || key==Qt::Key_Return ){
        //auto indent
        if( !hasSel ){
            int i;
            QString text = block.text();
            for( i = 0 ; i < cursor.positionInBlock() && text[i] <= ' ' ; ++i ){}
            QString spaces = text.left(i), s = "";
            //if haven't text before cursor
            if(i == cursor.positionInBlock()) {
                int d = i+1;
                cursor.setPosition(cursor.position()-d);
                setTextCursor(cursor);
                insertPlainText("\n"+spaces);
                cursor.setPosition(cursor.position()+d);
                setTextCursor(cursor);
                ensureCursorVisible();
                e->accept();
                return;
            }

            text = text.trimmed().toLower();
            int deltaPos = 0;
            QString closed = "";
            //don't check if the cursor is before text
            if(cursor.positionInBlock() > i) {
                if( text.startsWith("class ") || text.startsWith("interface ") || text.startsWith("function ") || text.startsWith("method ") || text.startsWith("select ") || text.startsWith("catch ") ) {
                    closed = "End";
                }
                else if(text.startsWith("if") || text.startsWith("else") /*|| text.startsWith("endif") || text.startsWith("end if")*/) {
                    closed = "EndIf";
                }
                else if(text.startsWith("case") || text.startsWith("default")) {
                    closed = "*";
                }
                else if(text.startsWith("while")) {
                    closed = "Wend";
                }
                else if(text.startsWith("repeat")) {
                    closed = "Forever";
                }
                else if(text.startsWith("for")) {
                    closed = "Next";
                }
                else if(text.startsWith("try")) {
                    closed = "Catch ";
                }
            }
            if(closed != "") {
                s = '\n'+spaces;
                bool add = true;
                if(text.startsWith("if") && text.contains("then"))
                    add = false;

                if(text.startsWith("method")) {
                    if(text.contains("abstract")) {
                        add = false;
                    }
                    else {
                        CodeItem *i = CodeAnalyzer::findScopeForBlock(block, true);
                        if(i && i->decl() == "interface")
                            add = false;
                    }
                }
                if(add)
                    s += "    ";
                int len = closed.length();
                if(ctrl && len > 1) {
                    s += '\n'+spaces+closed;
                    deltaPos = -(i+len+1);
                }
            }
            else {
                s = '\n'+spaces;
            }
            cursor.insertText(s);
            if(deltaPos != 0) {
                cursor.setPosition( cursor.position()+deltaPos );
                setTextCursor(cursor);
            }
            ensureCursorVisible();
            i = cursor.blockNumber();
            if(i > 0) {
                _highlighter->capitalize( document()->findBlockByNumber(i-1),cursor );
            }
            e->accept();
            return;
        }
    }

    if( key == Qt::Key_Backspace ) {
        if(ctrl) {//remove <= 4 spaces at once, emulate tab removing
            int pos = cursor.position();
            int cnt = 0;
            while(cnt < 4 && pos > 0 && document()->characterAt(pos-1) <= ' ') {
                --pos;
                ++cnt;
            }
            if(cnt > 0) {
                cursor.setPosition(cursor.position()-cnt, QTextCursor::KeepAnchor);
                setTextCursor(cursor);
                insertPlainText("");
            }
            e->accept();
            return;
        }//remove closed pair for "", (), []
        else {
            QString s = block.text();
            int len = s.length();
            int p = cursor.positionInBlock();
            QChar c1, c2;
            if(p > 0)
                c1 = s.at(p-1);
            if(p < len)
                c2 = s.at(p);
            if(!c1.isNull() && !c2.isNull()) {
                if((c1 == '\"' && c2 == '\"') || (c1 == '(' && c2 == ')') || (c1 == '[' && c2 == ']')) {
                    int pos = cursor.position()-1;
                    cursor.setPosition(pos, QTextCursor::MoveAnchor);
                    cursor.setPosition(pos+2, QTextCursor::KeepAnchor);
                    setTextCursor(cursor);
                    insertPlainText("");
                    e->accept();
                    return;
                }
                if(c1 == '.') {
                    onCompleteFocusOut();
                }
            }
        }
    }

    //HOME, smart move at begin pos
    if( !ctrl && key == Qt::Key_Home && block.length() > 1 ) {// && cursor.position() == block.position() ) {
        QString s = block.text();
        int i = 0, len = s.length();
        while( i < len && s.at(i) <= ' ' ) {
            ++i;
        }
        int bpos = block.position();
        i += bpos;
        int pos = cursor.position();
        if(pos > i || pos == bpos)
            pos = i;
        else
            pos = bpos;
        cursor.setPosition( pos, (shift ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor) );
        setTextCursor( cursor );
        e->accept();
        return;
    }

    block = cursor.block();

    if( e ) QPlainTextEdit::keyPressEvent( e );

    //auto ident for var = new ...
    if((key == ' ' && !ctrl && !shift) || checkNew) {
        QString s = block.text().trimmed().replace(" ","");
        if(s.endsWith("=new") || s.endsWith("=New")) {
            int i1 = s.indexOf(':');
            int i2 = s.indexOf("=");
            if(i1 && i1+1 < i2)
                s = s.mid(i1+1,i2-i1-1);
            identBeforeStart = identBeforeEnd = cursor.position();
            identStr = "";
            aucompShowList(s, cursor, "vartype", false);
            e->accept();
            return;
        }
    }

    //
    if( _lcomp && _lcomp->isVisible()) {
        if( key == Qt::Key_Escape) {
            onCompleteFocusOut();
            e->accept();
            return;
        }
        else if( (key >= Qt::Key_A && key <= Qt::Key_Z) || (key >= Qt::Key_0 && key <= Qt::Key_9) || key == Qt::Key_Backspace ) {
            aucompShowList( identBeforeCursor(), cursor, _aucompKind, false );
            e->accept();
            return;
        }
        else if(key != 46) {
            onCompleteFocusOut();
        }
    }


    if( _monkey && block.userState()==-1 ){
        //auto capitalize

        if( key >= 32 && key <= 255 ){
            if(!ctrl && !block.text().trimmed().startsWith("'") && (!_lcomp || !_lcomp->isVisible())) {
                QString ident = identBeforeCursor();
                if(ident.length() >= 2)
                    aucompShowList( ident,cursor,"",false );
            }
            if( (key >= Qt::Key_A && key <= Qt::Key_Z) || (key >= Qt::Key_0 && key <= Qt::Key_9) || (key == Qt::Key_Underscore) ) return;
        }else{
            if( block==textCursor().block() ) return;
        }

        _highlighter->capitalize( block,textCursor() );
    }


}

bool CodeEditor::findNext( const QString &findText,bool cased,bool wrap,bool backward ){

    QTextDocument::FindFlags flags=0;
    if( cased ) flags|=QTextDocument::FindCaseSensitively;
    if( backward ) flags|=QTextDocument::FindBackward;

    setCenterOnScroll( true );

    bool found=find( findText,flags );

    if( !found && wrap ){

        QTextCursor cursor=textCursor();

        setTextCursor( QTextCursor( document() ) );

        found=find( findText,flags );

        if( !found ) setTextCursor( cursor );
    }

    setCenterOnScroll( false );

    return found;
}

bool CodeEditor::replace( const QString &findText,const QString &replaceText,bool cased ){

    Qt::CaseSensitivity cmpFlags=cased ? Qt::CaseSensitive : Qt::CaseInsensitive;

    if( textCursor().selectedText().compare( findText,cmpFlags )!=0 ) return false;

    insertPlainText( replaceText );

    return true;
}

int CodeEditor::replaceAll( const QString &findText,const QString &replaceText,bool cased,bool wrap ){

    QTextDocument::FindFlags flags=0;
    if( cased ) flags|=QTextDocument::FindCaseSensitively;

    if( wrap ){
        QTextCursor cursor=textCursor();
        setTextCursor( QTextCursor( document() ) );
        if( !find( findText,flags ) ){
            setTextCursor( cursor );
            return 0;
        }
    }else{
        if( !find( findText,flags ) ) return 0;
    }

    insertPlainText( replaceText );

    int n=1;

    while( findNext( findText,cased,false ) ){
        insertPlainText( replaceText );
        ++n;
    }

    return n;
}

void CodeEditor::onPaste(const QString &text) {
    QTextCursor cursor = textCursor();
    QTextBlock block = cursor.block();
    QString s = block.text();
    QString clip = text;
    int i;
    //check if the cursor is between quotes
    if( block.length() > 3) {
        int pos = cursor.positionInBlock();
        i = s.indexOf("\"");
        if( i >= 0 && i < pos ) {
            i = s.indexOf("\"", pos);
            if( i > 0) {
                insertPlainText(clip.replace("\"","~q"));
                return;
            }
        }
    }
    //add indent
    i = 0;
    int n = s.length();
    //calc indent of current line
    while( i < n && s[i] <= ' ' ) ++i;
    QString indent = "";
    if(i) {
        indent = s.mid(0,i);
    }
    clip = clip.replace("\r\n","\n");
    QStringList list = clip.split("\n");
    clip = "";
    //calc minimum indent of clipboard text
    int min = 1024;
    for(int k = 1, size = list.size(); k < size; ++k) {
        s = list.at(k);
        i = 0;
        n = s.length();
        while( i < n && s[i] <= ' ' ) ++i;
        if(i < min)
            min = i;
    }
    //delete indent for first line
    s = list.at(0);
    i = 0;
    n = s.length();
    while( i < n && s[i] <= ' ' ) ++i;
    if(i)
        s = s.right(n-i);
    clip = s;
    for(int k = 1, size = list.size(); k < size; ++k) {
        s = list.at(k);
        n = s.length()-min;
        clip += "\n"+indent+s.right(n);
    }
    insertPlainText(clip);
}

QString CodeEditor::identAtCursor() {

    QTextDocument *doc=document();
    int len=doc->characterCount();

    int pos=textCursor().position();

    while( pos>=0 && pos<len && !isAlpha( doc->characterAt(pos) ) ){
        --pos;
    }
    if( pos<0 ) return "";

    while( pos>0 && isAlpha( doc->characterAt(pos-1) ) ){
        --pos;
    }
    if( pos==len ) return "";

    int start=pos;
    while( pos<len && isIdent( doc->characterAt(pos) ) ){
        ++pos;
    }
    if( pos==start ) return "";

    QTextCursor cursor( doc );
    cursor.setPosition( start );
    cursor.setPosition( pos,QTextCursor::KeepAnchor );
    QString ident=cursor.selectedText();
    return ident;
}

QString CodeEditor::identBeforeCursor() {
    QTextCursor cursor = textCursor();
    QString txt = cursor.block().text();
    int pos = cursor.positionInBlock();
    return identInLineBefore( txt, pos, cursor.position() );
}

QString CodeEditor::identInLineBefore( const QString &line, int posInLine, int posTotal ) {
    identBeforeStart = identBeforeEnd = posTotal;
    int p0 = posInLine;
    bool findBrace = false;
    while( posInLine > 0 ){
        bool norm = false;
        if(!findBrace) {
            norm = (isIdent(line.at(posInLine-1)) || line.at(posInLine-1) == '.');
            if(!norm) {
                if(line.at(posInLine-1) == ']') {
                    norm = true;
                    findBrace = true;
                }
            }
            if(!norm)
                break;
        }
        else {
            if(line.at(posInLine-1) == '[') {
                findBrace = false;
            }
        }
        --posInLine;
    }
    if( posInLine == p0 ) {
        return "";
    }

    int len = p0-posInLine;
    identBeforeStart = identBeforeEnd-len;
    QString ident = line.left(p0).mid(posInLine);
    return ident;
}

//***** Highlighter *****

Highlighter::Highlighter( CodeEditor *editor ):QSyntaxHighlighter( editor->document() ),_editor( editor ){

    connect( Prefs::prefs(),SIGNAL(prefsChanged(const QString&)),SLOT(onPrefsChanged(const QString&)) );

    onPrefsChanged( "" );

}

Highlighter::~Highlighter(){

}

void Highlighter::onPrefsChanged( const QString &name ){
    QString t(name);
    if( t=="" || t.endsWith( "Color" ) ){
        Prefs *prefs=Prefs::prefs();
        _backgroundColor=prefs->getColor( "backgroundColor" );
        _defaultColor=prefs->getColor("defaultColor");
        _numbersColor=prefs->getColor("numbersColor");
        _stringsColor=prefs->getColor("stringsColor");
        _identifiersColor=prefs->getColor("identifiersColor");
        _keywordsColor=prefs->getColor("keywordsColor");
        _monkeywordsColor = QColor(0xc8c8c8); //prefs->getColor("keywordsColor");
        _userwordsColor = QColor(0xc8c8c8); //prefs->getColor("keywordsColor");
        _userwordsColorVar = QColor(0x9876AA);
        _commentsColor=prefs->getColor("commentsColor");
        _highlightColor=prefs->getColor("highlightColor");
        rehighlight();
    }
}

QString Highlighter::parseToke( QString &text,QColor &color ){
    if( !text.length() )
        return "";

    int i = 0, n = text.length();
    QChar c = text[i++];

    bool monkeyFile = _editor->isMonkey();

    if( c <= ' ' ) {
        while( i<n && text[i]<=' ' ) ++i;
    }
    else if( isAlpha(c) ) {
        while( i < n && isIdent(text[i]) ) ++i;
        color = _identifiersColor;
        QString ident = text.left(i);
        if(monkeyFile) {
            if(CodeAnalyzer::containsKeyword(ident)){
                color = _keywordsColor;
            }
            else if(CodeItem *item = CodeAnalyzer::itemUser(ident, true)){
                color = (item->isField() ? _userwordsColorVar : _userwordsColor);
            }
            else if(CodeItem *item = CodeAnalyzer::itemMonkey(ident)){
                color = _monkeywordsColor;
            }
        }

    }
    else if( c == '0' && !monkeyFile ) {
        if( i < n && text[i] == 'x' ) {
            for( ++i ; i < n && isHexDigit( text[i] ) ; ++i ){}
        }
        else {
            for( ;i<n && isOctDigit( text[i] );++i ){}
        }
        color = _numbersColor;
    }
    else if( isDigit(c) || (c == '.' && i < n && isDigit(text[i])) ){
        bool flt = (c=='.');
        while( i < n && isDigit(text[i]) ) ++i;
        if( !flt && i < n && text[i] == '.' ) {
            ++i;
            flt = true;
            while( i < n && isDigit(text[i]) ) ++i;
        }
        if( i < n && (text[i] == 'e' || text[i] == 'E') ){
            flt = true;
            if( i < n && (text[i] == '+' || text[i] == '-') ) ++i;
            while( i < n && isDigit(text[i]) ) ++i;
        }
        color = _numbersColor;
    }
    else if( c == '%' && monkeyFile && i < n && isBinDigit( text[i] ) ){
        for( ++i ; i < n && isBinDigit( text[i] ) ; ++i ){}
        color = _numbersColor;
    }
    else if( c == '$' && monkeyFile && i < n && isHexDigit( text[i] ) ){
        for( ++i ; i < n && isHexDigit( text[i] ) ; ++i ){}
        color = _numbersColor;
    }
    else if( c == '\"' ){
        if( monkeyFile ){
            for( ; i < n && text[i] != '\"' ; ++i ){}
        }
        else {
            for( ; i < n && text[i] != '\"' ; ++i ){
                if( text[i] == '\\' && i+1 < n && text[i+1] == '\"' ) ++i;
            }
        }
        if( i < n )
            ++i;
        color = _stringsColor;
    }
    /*else if( !monkeyFile && c == '/' && i < n && text[i] == '/' ){
        for( ++i ; i < n && text[i] != '\n' ; ++i ){}
        if( i < n ) ++i;
        color = _commentsColor;
    }*/
    else if( c == '\'' ){
        if( monkeyFile ){
            for( ;i < n && text[i] != '\n' ; ++i ){}
            if( i < n ) ++i;
            color = _commentsColor;
        }
        /*else {
            for( ; i < n && text[i] != '\'' ; ++i ){
                if( text[i] == '\\' && i+1 < n && text[i+1] == '\'' ) ++i;
            }
            if( i < n ) ++i;
            color = _stringsColor;
        }*/
    }
    else {
        color = _defaultColor;
    }
    QString t = text.left(i);
    text = text.mid(i);
    return t;
}

bool Highlighter::capitalize( const QTextBlock &block,QTextCursor cursor ){

    QString text=block.text();
    QColor color;

    if(text.startsWith("import") || text.startsWith("Import"))
        return false;

    int i = 0, pos = cursor.position();

    cursor.beginEditBlock();

    for(;;) {
        QString t = parseToke( text,color );
        if( t.isEmpty() ) break;

        QString kw = CodeAnalyzer::keyword(t.toLower());

        if( !kw.isEmpty() && t!=kw ){
            int i0 = block.position()+i;
            int i1 = i0+t.length();
            cursor.setPosition( i0 );
            cursor.setPosition( i1,QTextCursor::KeepAnchor );
            cursor.insertText( kw );
        }

        i += t.length();
    }

    cursor.endEditBlock();

    cursor.setPosition( pos );

    return true;
}

void Highlighter::highlightBlock( const QString &ctext ){

    QString text = ctext;

    int i = 0, n = text.length();
    while( i < n && text[i] <= ' ' ) ++i;

    CodeItem *rem = CodeAnalyzer::findRemForBlock(currentBlock());
    if(rem) {
        setFormat( 0,text.length(),_commentsColor );
        return;
    }

    if( !_editor->isCode() ){
        setFormat( 0,text.length(),_defaultColor );
        return;
    }

    text = text.mid(i);

    int colst = 0;
    QColor curcol = _defaultColor;

    QVector<QString> tokes;

    for(;;) {

        QColor col = curcol;

        QString t = parseToke( text,col );
        if( t.isEmpty() ) break;

        if( t[0] > ' ' ) {
            tokes.push_back( t );
        }

        if( col != curcol ){
            if(curcol == _userwordsColor || curcol == _userwordsColorVar) {
                QFont f(_editor->font());
                QTextCharFormat tcf;
                tcf.setFont(f);
                tcf.setForeground(QBrush(curcol));
                setFormat( colst,i-colst,tcf );
            }
            else {
                setFormat( colst,i-colst,curcol );
            }
            curcol = col;
            colst = i;
        }

        i += t.length();
    }

    if( colst < n ) setFormat( colst,n-colst,curcol );

}


//***** BlockUserData *****

BlockData::BlockData(const QTextBlock &block) {
    _block = block;
    _blockEnd = block;
    _marked = false;
    _modified = 0;
    _folded = false;
    _foldable = false;
    _code = 0;
}

BlockData::~BlockData(){
}

BlockData* BlockData::data( QTextBlock &block, bool create) {
    BlockData *d = dynamic_cast<BlockData*>(block.userData());
    if( !d && create ) {
        d = new BlockData(block);
        block.setUserData(d);
    }
    return d;
}

void BlockData::flush(QTextBlock &block) {
    BlockData *d = dynamic_cast<BlockData*>(block.userData());
    int mod = 0;
    bool bm = false;
    if(d) {
        mod = d->modified();
        bm = d->isBookmarked();
        block.setUserData(0);
        if(mod || bm) {
            d = new BlockData(block);
            if(mod) {
                d->setModified( true );
                if(mod == 1)
                    d->setModified( false );
            }
            d->setBookmark(bm);
            block.setUserData(d);
        }
    }
}
