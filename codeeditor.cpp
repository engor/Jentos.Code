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
#include "theme.h"
#include "mainwindow.h"
#include "addpropertydialog.h"

CodeEditor *extraSelsEditor;
QList<QTextEdit::ExtraSelection> extraSels;
QTextEdit::ExtraSelection rowSelection;
QString _extraSelWord;
int _extraSelScrollPos;
QTextEdit::ExtraSelection hintWordSelection;

//static QList<QTextEdit::ExtraSelection> extraSelsWords;

void flushExtraSels() {
    if( !extraSelsEditor ) return;
    extraSels.clear();
    extraSelsEditor->setExtraSelections( extraSels );
    extraSelsEditor = 0;
}
void updateRowSelection(CodeEditor *editor) {
    extraSelsEditor = editor;
    if(!extraSels.isEmpty())
        extraSels.removeAt(0);
    extraSels.insert(0,rowSelection);
    extraSelsEditor->setExtraSelections( extraSels );
}
void flushWordsExtraSels() {
    if( !extraSelsEditor ) return;
    extraSels.clear();
    extraSels.append(rowSelection);
    extraSelsEditor->setExtraSelections( extraSels );
    //extraSelsEditor = 0;
}
void flushRowSelection() {
    if(!extraSels.isEmpty())
        extraSels.removeAt(0);
}
void appendExtraSels(const QTextEdit::ExtraSelection &sel, CodeEditor *editor) {
    extraSelsEditor = editor;
    extraSels.append(sel);
    extraSelsEditor->setExtraSelections( extraSels );
}


//***** CodeEditor *****

CodeEditor::CodeEditor( QWidget *parent ):QPlainTextEdit( parent ),_modified( 0 ){

    _editPosIndex = -1;
    _useAutoBrackets = Prefs::prefs()->getBool("AutoBracket");
    _charsCountForCompletion = Prefs::prefs()->getInt("CharsForCompletion");
    _addVoidForMethods = Prefs::prefs()->getBool("addVoidForMethods");

    _lineNumberArea = new LineNumberArea(this, 60);

    _prevCursorPos = _prevTextLen = _prevTextChangedPos = -1;
    _lcomp = 0;
    _blockChangeCursorMethod = false;
    _storedBlockNumber = -1;

    _highlighter = new Highlighter( this );

    connect( this,SIGNAL(textChanged()),SLOT(onTextChanged()) );
    connect( this,SIGNAL(cursorPositionChanged()),SLOT(onCursorPositionChanged()) );
    connect( this, SIGNAL(updateRequest(QRect,int)), this, SLOT(onUpdateLineNumberArea(QRect,int)) );
    connect( Prefs::prefs(),SIGNAL(prefsChanged(const QString&)),SLOT(onPrefsChanged(const QString&)) );
    connect( Theme::instance(),SIGNAL(endChange()),SLOT(onThemeChanged()) );

    setLineWrapMode( QPlainTextEdit::NoWrap );

    //qDebug()<<"editor";
    onPrefsChanged( "" );

    flushExtraSels();

    setViewportMargins(_lineNumberArea->maxwidth()-1, 0, 0, 0);

    setMouseTracking(true);
    setAcceptDrops(false);
}

CodeEditor::~CodeEditor(){
    flushExtraSels();
    //delete _lcomp;
    //_lcomp = 0;

}

bool CodeEditor::aucompIsVisible() {
    return (_lcomp != 0 && _lcomp->isVisible());
}

void CodeEditor::aucompProcess( CodeItem *item, bool forInheritance ) {
    QTextCursor c = textCursor();
    QString text = c.block().text();
    int i0 = c.positionInBlock();
    int i = i0-1;
    while ( i >= 0 && isIdent(text[i])) {
        --i;
    }
    ++i;
    int bp = c.block().position();
    c.setPosition(bp+i);
    c.setPosition(bp+i0, QTextCursor::KeepAnchor );
    setTextCursor(c);
    QString line = item->identForInsert();
    int blockEndPos = c.block().text().length();
    int d = 0;
    if (forInheritance) {
        line = item->descrAsItem();
    } else {
        if (item->isProperty()) {

        } else if (i0 == blockEndPos) {
            line = item->identWithParamsBraces(d);
        } else {
            QString s = text.mid(i0).trimmed();
            QChar ch = s[0];
            bool isident = isIdent(ch);
            if (ch != '(') {
                if (!isident) {
                    line = item->identWithParamsBraces(d);
                } else if (item->isFunc()) {
                    line += "(";
                }
            }
        }
    }
    insertPlainText( line );
    if ( d != 0 ) {
        c.setPosition( c.position()+d );
        setTextCursor(c);
    }
}

void CodeEditor::aucompShowList(bool process, bool inheritance )
{
    _lcompInheritance = inheritance;
    _lcompProcess = process;
    QTimer::singleShot(50, this, SLOT(onShowAutocompleteList()));
}

void CodeEditor::onCompleteProcess(QListWidgetItem *item) {
    if( !item ) {
        item = _lcomp->currentItem();
    }
    ListWidgetCompleteItem *icomp = dynamic_cast<ListWidgetCompleteItem*>(item);
    CodeItem *code = icomp->codeItem();
    bool forinh = _lcomp->isForInheritance();
    onCompleteFocusOut();//now we can close list
    aucompProcess(code,forinh);
    emit statusBarChanged( code->toString() );
}

void CodeEditor::onCompleteChangeItem(QListWidgetItem *current, QListWidgetItem *previous) {
    ListWidgetCompleteItem *lwi = dynamic_cast<ListWidgetCompleteItem*>(current);
    emit statusBarChanged( lwi->codeItem()->toString() );
}

void CodeEditor::onCompleteFocusOut() {
    if( _lcomp ) {
        _lcomp->disconnect();
        //_lcomp->parent()->r
        //this->layout()->removeWidget(_lcomp);
        //this->findChild()
        //delete _lcomp;
        //_lcomp = 0;
        _lcomp->hide();
        _lcomp->clear();
    }
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
    highlightCurrentLine();
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
        //store previous selection
        int sel1 = cursor.selectionStart();
        int sel2 = cursor.selectionEnd();
        if(sel1 > sel2) {
            int v = sel1;
            sel1 = sel2;
            sel2 = v;
        }
        QTextBlock begb = document()->findBlock( sel1 );
        QTextBlock endb = document()->findBlock( sel2 );
        //convert sel pos to in-block pos
        sel1 -= begb.position();
        sel2 -= endb.position();

        //replace text in our _storedBlock
        int len = s0.length();
        cursor.setPosition( b.position() );
        cursor.setPosition( b.position()+len, QTextCursor::KeepAnchor );
        setTextCursor( cursor );
        _blockChangeCursorMethod = true;
        insertPlainText( s );

        //restore cursor selection
        cursor.setPosition(begb.position()+sel1);
        cursor.setPosition(endb.position()+sel2, QTextCursor::KeepAnchor);
        setTextCursor( cursor );
    }
    updateSourceNavigationByCurrentScope();
}

void CodeEditor::replaceInRange(int from, int to, const QString &text) {
    QTextCursor c = textCursor();
    c.setPosition(from);
    c.setPosition(to, QTextCursor::KeepAnchor);
    setTextCursor(c);
    insertPlainText(text);
}

void CodeEditor::showDialogAddProperty()
{
    CodeItem *item = CodeAnalyzer::scopeAt(textCursor().block());
    if (item == 0 || !item->isClass()) {
        QMessageBox::information(0, "Add Property", "You can add property only inside a class.\n\nBut class isn't selected\n or \ncursor is inside of method.");
        return;
    }

    // get all classes names
    QList<CodeItem*> targetList;
    CodeAnalyzer::allClasses(targetList);

    QStringList types;
    types << "Bool" << "Float" << "Int" << "String";

    foreach (CodeItem *i, targetList) {
        types.append(i->ident());
    }

    // show dialog
    AddPropertyDialog *form = new AddPropertyDialog();
    form->fillTypes(types);
    form->exec();

    if (!form->pressOk)
        return;

    // insert text to document
    QTextCursor cursor = textCursor();
    QTextBlock block = cursor.block();
    QString indent = block.text(); //expecting only spaces or tabs here - it's line indent

    QString propName = form->propName, propType = form->propType, wrapName = form->wrappedName, defValue = form->defaultValue;
    bool addWrap = form->isNeedToAddWrapped;
    int addVariant = form->addVariant;

    QString tab = CodeAnalyzer::tab();

    QString getter = indent+"Method "+propName+":"+propType+"()\n"
            +indent+tab+"Return "+wrapName+"\n"
            +indent+"End\n";
    QString setter = indent+"Method "+propName+":Void(value:"+propType+")\n"
            +indent+tab+wrapName+" = value\n"
            +indent+"End\n";

    QString text;
    if (addVariant == AddPropertyDialog::ADD_GETTER_THEN_SETTER) {
        text = getter+setter;
    } else if (addVariant == AddPropertyDialog::ADD_SETTER_THEN_GETTER) {
        text = setter+getter;
    } else if (addVariant == AddPropertyDialog::ADD_SETTER_ONLY) {
        text = setter;
    } else if (addVariant == AddPropertyDialog::ADD_GETTER_ONLY) {
        text = getter;
    }

    if (addWrap) {
        if (propType == "String") {
            defValue = defValue.replace("\"", "~q");
            defValue = "\""+defValue+"\"";
        }
        QString def = defValue.isEmpty() ? "" : " = "+defValue;
        text = indent+"Field "+wrapName+":"+propType+def+"\n" + text;
    }

    text += "\n"+indent;

    // select line - to replace selection, because our text already contains indent
    cursor.setPosition(block.position(), QTextCursor::KeepAnchor);
    setTextCursor(cursor);

    insertPlainText(text);
    ensureCursorVisible();

    // clear resources
    targetList.clear();
    types.clear();
    delete form;
}

bool CodeEditor::canFindUsages() {
    QString s;
    QTextCursor c = textCursor();
    if(c.hasSelection()) {
        s = c.selectedText();
    }
    else {
        c.select(QTextCursor::WordUnderCursor);
        s = c.selectedText();
    }
    //qDebug()<<"use:"<<s;
    return (s != "" && isAlpha(s[0]) && !CodeAnalyzer::containsKeyword(s));
}

QString CodeEditor::findUsages(QTreeWidget *tree) {
    QString s;
    QTextCursor c = textCursor();
    if(c.hasSelection()) {
        s = c.selectedText();
    }
    else {
        c.select(QTextCursor::WordUnderCursor);
        s = c.selectedText();
    }
    //qDebug()<<"findUsage:"<<s;
    CodeItem *code = CodeAnalyzer::findInScope(c.block(), c.selectionEnd()-c.block().position(), 0, true, "");
    QTreeWidgetItem *root = tree->invisibleRootItem(), *item;
    if(!code) {
        item = new QTreeWidgetItem(root);
        item->setText(0,"Not found");
        root->addChild(item);
    }
    else {
        QString ident = s;
        s = code->ident();
        if(!code->isClassOrInterface())
            s += ":"+code->identType();
        //if(code->parent())
        //    s += " from "+code->parent()->ident();

        //search for all entrances
        QTextDocument::FindFlags flags = QTextDocument::FindCaseSensitively;
        QHash<QString, QTextDocument *> docs = CodeAnalyzer::docs();
        QList<QString> keys = docs.keys();
        int count = 0;
        CodeAnalyzer::storeCurFilePath();
        /*foreach (QString key, keys) {
            emit showCode(key,-1);
        }*/
        foreach (QString key, keys) {
            //qDebug()<<"doc:"<<key;
            //emit showCode(key,0);
            QTextDocument *d = docs.value(key);
            CodeAnalyzer::setCurFilePath(key);
            c = QTextCursor(d);
            bool first = true;
            //int lastNum = -1;
            while(true) {
                c = d->find(ident, c, flags);
                if(c.isNull())
                    break;
                //qDebug()<<"found:"<<c.selectedText()<<c.block().text().trimmed();

                int num = c.block().blockNumber();
                /*if(num == lastNum) {
                    //qDebug()<<"num == lastNum";
                    continue;
                }*/

                QTextCursor c2 = QTextCursor(c);
                c2.setPosition( (c2.selectionStart()+c2.selectionEnd())/2 );
                c2.select(QTextCursor::WordUnderCursor);
                if(c2.selectedText() != ident) {
                    //qDebug()<<"word:"<<c2.selectedText();
                    continue;
                }
                CodeItem *code2 = CodeAnalyzer::findInScope(c.block(), c.selectionEnd()-c.block().position(), 0, true, "");
                //if(code2)
                //    qDebug()<<"code2:"<<code2->descrAsItem()<<code2->filepath();
                if(code2 == code /*&& CodeItem::equals(code2,code)*/) {

                    if(first) {
                        item = new QTreeWidgetItem(root);
                        item->setText(0,key);
                        root->addChild(item);
                        first = false;
                    }
                    QTreeWidgetItem *sub = new QTreeWidgetItem(item);
                    QString snum = QString::number(num+1);
                    //QString t = c.block().text();
                    sub->setText(0, snum+":  "+c.block().text().trimmed());
                    sub->setToolTip(0,"Line: "+snum+", Position: "+QString::number(c.selectionStart()-c.block().position()+1));
                    sub->setCheckState(0,Qt::Checked);
                    item->addChild(sub);
                    ++count;
                    UsagesResult::add(sub,key,ident,num,c.selectionStart(),c.selectionEnd());
                    //sub->setData(0,Qt::UserRole,ur);
                }
            }
        }
        CodeAnalyzer::restoreCurFilePath();
        s += " ["+QString::number(count)+"]";
    }
    //qDebug()<<"done:"<<s;
    return s;
}

void CodeEditor::undo() {
    _blockChangeCursorMethod = true;
    QPlainTextEdit::undo();
}

void CodeEditor::redo() {
    _blockChangeCursorMethod = true;
    QPlainTextEdit::redo();
}

void CodeEditor::highlightCurrentLine(){
    highlightLine( textCursor().blockNumber(), HlCaretRow );
}

void CodeEditor::highlightLine( int line, Highlighting kind ){

    QTextBlock block = document()->findBlockByNumber( line );

    if( block.isValid() ){
        QColor lineColor;
        if( kind == HlCommon) {
            lineColor = Prefs::prefs()->getColor( "highlightColor" );
        }
        else if( kind == HlCaretRow || kind == HlCaretRowCentered ) {
            lineColor = Prefs::prefs()->getColor( "highlightCaretRowColor" );
        }
        else if( kind == HlError ) {
            lineColor = Prefs::prefs()->getColor( "highlightErrorColor" );
        }

        QTextCursor cursor = QTextCursor( block );
        if( kind == HlCommon || kind == HlError || kind == HlCaretRowCentered ) {
            setCenterOnScroll( true );
            cursor.setPosition(block.position());
            setTextCursor(cursor);
            setCenterOnScroll( false );
        }

        if( _isHighlightLine || (kind != HlCaretRow && kind != HlCaretRowCentered) ) {
            rowSelection.format.setBackground( lineColor );
            rowSelection.format.setProperty( QTextFormat::FullWidthSelection,true );
            rowSelection.cursor = cursor;
            rowSelection.cursor.clearSelection();
        }
        updateRowSelection(this);
    }

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

void CodeEditor::onThemeChanged() {
    /*if(_isHighlightWord) {
        QColor bg = QColor(255,0,0);//Theme::selWordColor();
        foreach (QTextEdit::ExtraSelection sel, extraSels) {
            sel.format.setBackground(bg);
        }
        setExtraSelections(extraSels);
    }*/
    flushExtraSels();
    if(_isHighlightLine) {
        rowSelection.format.setBackground( Prefs::prefs()->getColor("highlightColorCaretRow") );
        updateRowSelection(this);
    }
}

void CodeEditor::onPrefsChanged( const QString &name ){

    //qDebug() << "editor.prefs:"<<name;

    QString t( name );

    Prefs *prefs = Prefs::prefs();

    _useAutoBrackets = prefs->getBool("AutoBracket");
    _charsCountForCompletion = prefs->getInt("CharsForCompletion");
    _addVoidForMethods = prefs->getBool("addVoidForMethods");

    if( t=="" || t=="backgroundColor" || t=="fontFamily" || t=="fontSize" || t=="tabSize" || t=="smoothFonts" || t=="highlightLine" || t=="highlightWord" ){

        bool hl = _isHighlightLine;
        bool hw = _isHighlightWord;
        _isHighlightLine = prefs->getBool("highlightLine");
        _isHighlightWord = prefs->getBool("highlightWord");

        if(_isHighlightLine != hl || _isHighlightWord != hw) {
            flushExtraSels();
            if(_isHighlightLine)
                updateRowSelection(this);
        }

        QColor bg = prefs->getColor( "backgroundColor" );
        QColor fg( 255-bg.red(),255-bg.green(),255-bg.blue() );

        /*QPalette p = palette();
        p.setColor( QPalette::Base,bg );
        p.setColor( QPalette::Text,fg );
        setPalette( p );*/

        QString cbg = Theme::hexColor(bg);
        QString cfg = Theme::hexColor(fg);

        QString s = "background:"+cbg+";background-color:"+cbg+";color:"+cfg+";";
        setStyleSheet(s);

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
        setTabStopWidth( fm.width(' ')*prefs->getInt( "tabSize" ) );

        repaint();

    }
}

void CodeEditor::onCursorPositionChanged(){
    if(!_highlighter->isEnabled())
        return;
    QTextCursor cursor = textCursor();
    if( cursor.isNull() )
        return;
    int row = cursor.blockNumber();
    //highlightLine( row, HlCaretRow );
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
    //qDebug()<<"onCursorPositionChanged";

    if(_isHighlightWord) {
        cursor.select(QTextCursor::WordUnderCursor);
        QString s = cursor.selectedText();
        QScrollBar *sb = this->verticalScrollBar();
        int scroll = sb->value();
        //qDebug()<<"scroll:"<<scroll;
        if(s == _extraSelWord && scroll == _extraSelScrollPos) {
            //qDebug()<<"word is the same:"<<s;
            return;
        }
        _extraSelWord = s;
        _extraSelScrollPos = scroll;
        //qDebug()<<"word:"<<s;
        if(s == "" || !isAlpha(s[0]) || CodeAnalyzer::containsKeyword(s)) {
            return;
        }
        QTextBlock b = firstVisibleBlock();
        float bheight = blockBoundingRect(b).height();
        int areaHeight = rect().height();
        int n = (areaHeight/bheight)+2;
        int i = 0;
        flushWordsExtraSels();
        QColor bg = Theme::selWordColor();
        len = _extraSelWord.length();
        int count = 0;
        while (b.isValid() && i < n) {
            s = b.text();
            int p = -1;
            while( (p = s.indexOf(_extraSelWord,p+1)) >= 0) {
                QTextCursor cursor = textCursor();
                int pos = p+b.position();
                cursor.setPosition(pos+1);
                cursor.select(QTextCursor::WordUnderCursor);
                QString t = cursor.selectedText();
                if(t == _extraSelWord) {
                    QTextEdit::ExtraSelection sel;
                    sel.format.setBackground(bg);
                    sel.cursor = cursor;
                    appendExtraSels(sel, this);
                    ++count;
                }
                p += len;
            }
            if(b.isVisible())
                ++i;
            b = b.next();
        }
        //if(count < 2)
        //    flushWordsExtraSels();
    }

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

    if(CodeAnalyzer::isFillAucompWithInheritance()) {
        int i = cursor.positionInBlock();
        if(i > 6) {
            QString ss = cursor.block().text();
            if(ss[i-1] == ' ')
                ss = ss.left(i);
            if(ss.endsWith("Method ")) {
                aucompShowList(false, true);
            }
        }
    }
}

void CodeEditor::onSourceListViewClicked( const QModelIndex &index ) {
    if(!index.isValid()) {
        qDebug()<<"index is invalid";
        return;
    }
    ItemWithData *si = CodeAnalyzer::itemInList(index);
    if( !si ) {
        //qDebug()<<"si is null";
        return;
    }
    CodeItem *code = si->code();
    //CodeItem *code = CodeAnalyzer::getCodeItemFromStandardItem(si);
    //qDebug()<<"LIST: "<<code->descrAsItem();
    QTextBlock b = code->block();
    if(b.isValid()) {
        if(!b.isVisible()) {
            CodeItem *i = CodeAnalyzer::scopeAt(b, true);
            if(i) {
                unfoldBlock(i->block());
            }
        }
        QString file = code->filepath();
        _blockChangeCursorMethod = true;
        emit showCode( file, code->blockNumber() );
    }
    QApplication::processEvents();
}

void CodeEditor::onCodeTreeViewClicked( const QModelIndex &index ){

    QStandardItem *si = CodeAnalyzer::itemInTree(index);
    if(!si) {
        //qDebug()<<"si is null";
        return;
    }
    CodeItem *code = CodeAnalyzer::getCodeItemFromStandardItem(si);
    if(!code)
        return;

    fillSourceListWidget(code, si);

    QTextBlock b = code->block();

    if(b.isValid()) {
        _blockChangeCursorMethod = true;
        if(!b.isVisible()) {
            CodeItem *i = CodeAnalyzer::scopeAt(b, true);
            if(i) {
                unfoldBlock(i->block());
            }
        }
        QString file = code->filepath();
        emit showCode( file, code->blockNumber() );
    }
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

void CodeEditor::onShowAutocompleteList()
{
    if(_lcomp != 0) {
        _lcomp->disconnect();
        //this->layout()->removeWidget(_lcomp);
        //delete _lcomp;
        _lcomp->hide();
        _lcomp->clear();
    }
    else {
        _lcomp = new ListWidgetComplete(this);
    }

    _lcomp->setIsForInheritance(_lcompInheritance);

    QTextCursor cursor = textCursor();

    if (_lcompInheritance)
        CodeAnalyzer::fillListInheritance(cursor.block(), _lcomp);
    else
        CodeAnalyzer::findInScope(cursor.block(), cursor.positionInBlock(), _lcomp);

    int count = _lcomp->count();
    if( count > 0 ) {
        if( _lcompProcess && count == 1 ) { //select value if one
            ListWidgetCompleteItem *item = dynamic_cast<ListWidgetCompleteItem*>(_lcomp->item(0));
            aucompProcess(item->codeItem());
        }
        else {

            /*for (int k = 0; k < count; ++k) {
                _lcomp->addWidgetForItem( _lcomp->item(k) );
            }*/

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

        connect( _lcomp,SIGNAL(itemActivated(QListWidgetItem*)),SLOT(onCompleteProcess(QListWidgetItem*)) );
        connect( _lcomp,SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),SLOT(onCompleteChangeItem(QListWidgetItem*,QListWidgetItem*)) );
        connect( _lcomp,SIGNAL(focusOut()),SLOT(onCompleteFocusOut()) );

    }
    else {
        onCompleteFocusOut();
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

        CodeItem *i = CodeAnalyzer::scopeAt(b);
        if(i) {
            unfoldBlock(i->block());
            if(i->parent())
                unfoldBlock(i->parent()->block());
        }
        setCenterOnScroll(true);
        QTextCursor c = textCursor();
        c.setPosition(b.position());
        setTextCursor(c);

        ensureCursorVisible();
        setCenterOnScroll(false);
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

        CodeItem *i = CodeAnalyzer::scopeAt(b);
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

void CodeEditor::foldAll() {
    QTextBlock b, block = document()->firstBlock();
    while(block.isValid()) {
        /*if(!block.isVisible()) {
            block = block.next();
            continue;
        }*/
        b = block;
        BlockData *data = BlockData::data(block);
        if(data && data->isFoldable())
            /*b = */foldBlock(block);
        block = b.next();
    }
}

void CodeEditor::unfoldAll() {
    QTextBlock b, block = document()->firstBlock();
    while(block.isValid()) {
        b = block;
        BlockData *data = BlockData::data(block);
        if(data && data->isFoldable())
            /*b =*/ unfoldBlock(block);
        block = b.next();
    }
}

void CodeEditor::autoformatAll() {
    _highlighter->setEnabled(false);
    QTextCursor c = textCursor();
    c.beginEditBlock();
    int b = c.blockNumber();
    QTextBlock block = document()->firstBlock();
    while(block.isValid()) {
        QString s = block.text();
        c.setPosition(block.position());
        c.setPosition(block.position()+s.length(), QTextCursor::KeepAnchor);
        bool r = CodeAnalyzer::autoFormat(s, true);
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
    _highlighter->setEnabled(true);
    _highlighter->rehighlight();
}

void CodeEditor::lowerUpperCase(bool lower) {
    QTextCursor c = textCursor();
    if(!c.hasSelection())
        return;
    QString s = c.selectedText();
    QString s2 = (lower ? s.toLower() : s.toUpper());
    if(s2 != s) {
        int i1 = c.selectionStart();
        int i2 = c.selectionEnd();
        insertPlainText(s2);
        c.setPosition(i1);
        c.setPosition(i2, QTextCursor::KeepAnchor);
        setTextCursor(c);
    }
}

void CodeEditor::adjustDocumentSize() {
    document()->adjustSize();
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
            int r = l+block.length()-1;
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
    static QColor clrGray1 = QColor(0x313334);
    static QColor clrGray2 = QColor(0xE9E8E2);
    bool d = Theme::isDark();
    QColor clrGray = (d ? clrGray1 : clrGray2);
    static QImage imgBookmark1 = Theme::imageDark("Bookmark.png");
    static QImage imgBookmark2 = Theme::imageLight("Bookmark.png");
    static QImage imgPlus1 = Theme::imageDark("Unfold.png");
    static QImage imgPlus2 = Theme::imageLight("Unfold.png");
    static QImage imgMinus1 = Theme::imageDark("Fold.png");
    static QImage imgMinus2 = Theme::imageLight("Fold.png");
    QImage imgBookmark = (d ? imgBookmark1 : imgBookmark2);
    QImage imgPlus = (d ? imgPlus1 : imgPlus2);
    QImage imgMinus = (d ? imgMinus1 : imgMinus2);

    QPainter painter(_lineNumberArea);

    painter.fillRect(event->rect(), clrGray);
    if(!d) {
        painter.setPen(QColor(0xb8b8b8));
        painter.drawLine(event->rect().right(),event->rect().y(),event->rect().right(),event->rect().bottom());
    }
    painter.setPen(QColor(110,110,110));
    QTextBlock block = firstVisibleBlock();
    int hg = fontMetrics().height();
    float bheight = blockBoundingRect(block).height();
    float top = contentOffset().y();//blockBoundingRect(block).y();
    //float top = blockBoundingGeometry(block).translated(contentOffset()).top();
    int width = _lineNumberArea->width();
    int wd = 40;

    int areaHeight = rect().height();
    //qDebug()<<top;
    //return;
    int n = (areaHeight/bheight)+1;
    int i = 0;
    //
    while (block.isValid() && i < n) {
        //
        bheight = blockBoundingRect(block).height();
        //qDebug()<<bheight;
        if (!block.isVisible()) {
            //top += blockBoundingRect(block).y();
            block = block.next();
            continue;
        }
        int py = top;//blockBoundingRect(block).y()-top+contentOffset().y();
        BlockData *data = BlockData::data(block);
        bool foldable = (data && data->isFoldable());
        int ftype = (data ? data->foldType : 0);
        //qDebug()<<"ftype:"<<ftype<<block.text();
        bool endfold = ((ftype & 2) != 0);
        bool infold = ((ftype & 1) != 0);

        bool bkmrk = (data && data->isBookmarked());
        bool inher = (data && data->code() && data->code()->isInherited());
        if( inher ) {
            painter.setPen(QColor(130,130,130));
            int px = (bkmrk ? wd-(d?14:18) : wd);
            painter.drawText(0, py, px, hg, Qt::AlignRight, "ovr.");
        }
        if( bkmrk ) {
            int yy = (d ? py+1 : py-2);
            painter.drawImage(wd-imgBookmark.width(), yy, imgBookmark);
        }
        if( !bkmrk && !inher ) {
            int num = block.blockNumber() + 1 ;
            QString number = QString::number(num);
            painter.setPen(QColor(130,130,130));
            painter.drawText(0, py, wd, hg, Qt::AlignRight, number);
        }
        if( data && data->modified() > 0 ) {
            painter.fillRect(wd+3, py, 1, hg, (data->modified()==1 ? Qt::darkGray : clrGreen) );
        }
        //
        if(infold || endfold) {
            QPen pen0 = painter.pen();
            QPen pen;
            pen.setStyle(Qt::DotLine);
            pen.setColor(QColor(85,85,85));
            painter.setPen(pen);
            QLineF line;
            int px = width-10;
            if(endfold) {
                line.setLine(px,py+bheight/2,width,py+bheight/2);
                painter.drawLine(line);

            }
            if(infold) {
                line.setLine(px,py+bheight/2,px,py+bheight+bheight/2);
                painter.drawLine(line);
            }
            painter.setPen(pen0);
        }
        //
        if( foldable ) {
            if(data->isFolded()) {
                painter.drawImage(width-14, py+4, imgPlus );
            }
            else {
                painter.drawImage(width-14, py+4, imgMinus );
            }
        }
        top += bheight;
        ++i;
        block = block.next();
    }
    QWidget::paintEvent(event);
}

void CodeEditor::mousePressEvent(QMouseEvent *e) {
    if( (e->button() & Qt::LeftButton) && (e->modifiers() & Qt::ControlModifier) ) {
        CodeItem *item = _scope.item;
        if(item) {
            if(!item->isKeyword()) {
                QTextCursor cursor = textCursor();
                editPosInsert(cursor.blockNumber());//save 'from' position
                emit openCodeFile( item->filepath(),"", item->blockNumber() );
                e->accept();
                cursor = textCursor();
                editPosInsert(cursor.blockNumber());//save 'to' position
                return;
            }
            else if(_scope.ident == "Import") {
                //qDebug()<<"click to import";
                QString dir = extractDir(_path)+"/";
                QString line = textCursor().block().text();
                QString s = line.mid(7).replace(".","/") + ".monkey";
                //qDebug()<<dir+s;
                if(QFile::exists(dir+s)) {
                    showCode(dir+s,0);
                }
                else {
                    Prefs *p = Prefs::prefs();
                    dir = p->getString("monkeyPath")+"/modules/";
                    //qDebug()<<dir+s;
                    if(QFile::exists(dir+s)) {
                        showCode(dir+s,0);
                    }
                }
                _scope.flush();
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
    bool sel = textCursor().hasSelection();
    if( !sel && (e->modifiers() & Qt::ControlModifier) ) {
        QTextCursor cursor = cursorForPosition(e->pos());
        cursor.select(QTextCursor::WordUnderCursor);
        //flushExtraSels();
        QTextBlock block = cursor.block();
        if( block.isValid() ) {
            QTextEdit::ExtraSelection es = hintWordSelection;
            flushWordsExtraSels();
            es.format.setForeground( (Theme::isDark() ? QColor(250,250,250) : QColor(0,0,255)) );
            es.format.setFontUnderline( true );
            es.format.setFontWeight( QFont::Bold );
            es.cursor = cursor;
            extraSels.append( es );
        }
        setExtraSelections( extraSels );
        extraSelsEditor = this;
        QString word = cursor.selectedText();

        if( !word.isEmpty() ) {
            QString tip = "";
            if(_scope.ident == word) {
                tip = _scope.toolTip;
                if(!tip.isEmpty()) {
                    showToolTip(e->globalPos(), tip);
                }
                else {
                    QToolTip::hideText();
                }
                return;
            }

            cursor.setPosition(cursor.selectionEnd());
            setTextCursor(cursor);

            CodeItem *item = CodeAnalyzer::itemKeyword(word);
            if(!item) {
                item = CodeAnalyzer::findInScope(cursor.block(), cursor.positionInBlock(), 0, true);
            }
            if(item)
                tip = CodeAnalyzer::toolTip(item);
            if(!tip.isEmpty()) {
                showToolTip(e->globalPos(), tip);
            }
            else {
                QToolTip::hideText();
            }
            _scope.ident = word;
            _scope.toolTip = tip;
            _scope.item = item;
        }
        return;
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
    //highlightLine( textCursor().blockNumber(), HlCaretRow );
    QGuiApplication::restoreOverrideCursor();
}

void CodeEditor::commentUncommentBlock() {
    QTextCursor c = textCursor();
    if(!c.hasSelection())
        return;
    int i1 = c.selectionStart();
    int i2 = c.selectionEnd();
    if(i1 > i2) {//swap
        int tmp = i1;
        i1 = i2;
        i2 = tmp;
    }
    QTextBlock b = document()->findBlock(i1);
    QTextBlock end = document()->findBlock(i2);
    c.beginEditBlock();
    while(b.isValid()) {
        QString t = b.text();
        if(t.startsWith('\'')) {
            c.setPosition(b.position());
            c.setPosition(b.position()+1,QTextCursor::KeepAnchor);
            setTextCursor(c);
            insertPlainText("");
        }
        else {
            c.setPosition(b.position());
            setTextCursor(c);
            insertPlainText("'");
        }
        if(b == end)
            break;
        b = b.next();
    }
    c.endEditBlock();
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
        CodeItem *i = CodeAnalyzer::scopeAt(b);
        if(i) {
            unfoldBlock(i->block());
            if(i->parent())
                unfoldBlock(i->parent()->block());
        }
        //
        highlightLine(line, HlCaretRowCentered);
    }
}

void CodeEditor::analyzeCode() {
    //update analyzer
    CodeAnalyzer::begin();
    bool res = CodeAnalyzer::parse(document(), _path);
    if(res) {
        fillCodeTree();
        _highlighter->rehighlight();
        this->repaint();
        //qDebug() << "code analyze done";
    }
    CodeAnalyzer::end();
}

void CodeEditor::fillCodeTree() {
    CodeAnalyzer::fillTree();
    updateSourceNavigationByCurrentScope();
}

void CodeEditor::updateSourceNavigationByCurrentScope() {
    //qDebug()<<"1";
    CodeItem *code = CodeAnalyzer::scopeAt(textCursor().block(),false,true);
    while(code && !code->isClassOrInterface() && !code->isFunc()) {
        code = code->parent();
    }
    if(code) {
        //qDebug()<<"list from scope: "<<code->descrAsItem();
        QStandardItem *si = CodeAnalyzer::getStandardItem( code->fullItemPath() );
        fillSourceListWidget(code, si);
        if(code->parent()) {
            code = code->parent();
            si = CodeAnalyzer::getStandardItem( code->fullItemPath() );
        }
        if(si)
            CodeAnalyzer::treeView()->setCurrentIndex(si->index());//code->itemWithData(code->ident())->index());
    }
}

void CodeEditor::fillSourceListWidget(CodeItem *item, QStandardItem *si) {
    //qDebug()<<"fillSourceListWidget:"<<item->descrAsItem();
    //QStandardItem *si = CodeAnalyzer::getStandardItem( item->fullItemPath() );
    if(!si) {
        //qDebug()<<"si is null";
        return;
    }
    //qDebug()<<"si1:"<<si->text();
    QStandardItemModel *model = CodeAnalyzer::listItemModel();
    model->clear();
    CodeItem *selected = item;//, ident=item->ident();
    if(item->parent() != 0) {
        //qDebug()<<"fillSourceListWidget.parent"<<item->parent()->descrAsItem();
        //selected = item->descrAsItem();
        item = item->parent();
        //ident = item->parent()->ident();
        si = si->parent();
    }
    //qDebug()<<"si2:"<<si->text();

    ItemWithData *ilist;
    QStandardItem *itree;
    bool sort = CodeAnalyzer::isSortByName();
    for(int r = 0; r < si->rowCount(); ++r) {
        itree = si->child(r);
        ilist = new ItemWithData;
        model->appendRow(ilist);
        QString t = itree->text();
        if(sort) {
            QString tip = itree->toolTip();
            if(tip.startsWith("(function"))
                t = "---"+t;
            else if(tip.startsWith("(method"))
                t = "--"+t;
            else if(tip.startsWith("(global"))
                t = "-"+t;
        }
        ilist->setText(t);
        ilist->setIcon(itree->icon());
        ilist->setToolTip(itree->toolTip());
        //ilist->setData(itree->data());
        CodeItem *code = CodeAnalyzer::getCodeItemFromStandardItem(itree);
        ilist->setCode(code);
        if(code == selected)
            CodeAnalyzer::listView()->setCurrentIndex(ilist->index());
    }
    if(sort) {
        model->sort(0);
        for(int r = 0; r < si->rowCount(); ++r) {
            QStandardItem *i = model->item(r);
            QString t = i->text();
            if(t.startsWith("---"))
                t = t.mid(3);
            else if(t.startsWith("--"))
                t = t.mid(2);
            else if(t.startsWith("-"))
                t = t.mid(1);
            i->setText(t);
            //if(t == selFunc)
            //    CodeAnalyzer::listView()->setCurrentIndex(i->index());
        }
    }
    if(item->isClassOrInterface()) {
        ilist = new ItemWithData;
        ilist->setText(" :: "+item->descr()+" :: ");
        ilist->setIcon(si->icon());
        ilist->setToolTip(si->toolTip());
        //ilist->setData(si->data());
        //CodeItem *code = CodeAnalyzer::getCodeItemFromStandardItem(itree);
        ilist->setCode(item);
        model->insertRow(0,ilist);
    }
    if(selected->isClassOrInterface()) {

        CodeAnalyzer::listView()->setCurrentIndex(model->index(0,0));
    }
    //qDebug()<<"done";
}

void CodeEditor::keyPressEvent( QKeyEvent *e ) {

    int key = e->key();
    //qDebug() << "key: "+QString::number(key);

    bool ctrl = (e->modifiers() & Qt::ControlModifier);
    bool shift = (e->modifiers() & Qt::ShiftModifier);

    QTextCursor cursor = textCursor();
    QTextBlock block = cursor.block();
    bool hasSel = cursor.hasSelection();


    //escape
    if (key == Qt::Key_Escape && !aucompIsVisible()) {
        emit keyEscapePressed();
    }

    //ctrl + s
    if (ctrl && key == 83){
        e->accept();
        return;
    }
    //ctrl + z
    if (ctrl && key == Qt::Key_Z){
        undo();
        e->accept();
        return;
    }
    //ctrl + y
    if (ctrl && key == Qt::Key_Y){
        redo();
        e->accept();
        return;
    }

    //ctrl + x || shift + del
    bool ctrl_x = (ctrl && key == Qt::Key_X);
    bool shift_del = (shift && key == Qt::Key_Delete);
    if ( hasSel && (key == Qt::Key_Delete || key == Qt::Key_Backspace || ctrl_x || shift_del) ){
        //qDebug() << "cut";
        QString s = cursor.selectedText();
        if (ctrl_x || shift_del) {
            QApplication::clipboard()->setText(s);
            //qDebug() << "clipboard";
        }
        insertPlainText("");
        ensureCursorVisible();
        e->accept();
        return;
    }

    //ctrl + E - delete line under cursor
    if (ctrl && key == Qt::Key_E){
        int pos = cursor.positionInBlock();
        cursor.setPosition(block.position());
        cursor.setPosition(block.position()+block.length(), QTextCursor::KeepAnchor);
        setTextCursor(cursor);
        insertPlainText("");// replace selection to empty str
        // new cursor, move it to the same indent that was on deleted line
        cursor = textCursor();
        block = cursor.block();
        if (pos > block.length()-1)
            pos = block.length()-1;
        cursor.setPosition(block.position()+pos);
        setTextCursor(cursor);
        e->accept();
        return;
    }


    //insert code completion by template
    if(!hasSel && key == Qt::Key_Tab) {
        if(!aucompIsVisible()) {
            QString t, s = identAtCursor();
            //qDebug() << "template: "+s;
            t = CodeAnalyzer::templateWord(s);
            if(!t.isNull()) {
                int n = s.length();
                int pos = cursor.position() - n;
                int i = t.indexOf("%cursor%");
                if(i >= 0) {
                    t = t.replace("%cursor%","");
                    i += pos;
                }
                cursor.setPosition(pos,QTextCursor::KeepAnchor);
                setTextCursor(cursor);
                onPaste(t);
                if(i >= 0) {
                    cursor.setPosition(i,QTextCursor::MoveAnchor);
                    setTextCursor(cursor);
                }
                e->accept();
                return;
            }
        }
        else {
            //qDebug()<<"if1";
            onCompleteProcess();
            e->accept();
            return;
        }
    }

    QString evtxt = e->text();
    QChar typedChar = evtxt.length() > 0 ? evtxt.at(0) : QChar();
    //qDebug()<<"typedChar:"<<typedChar;

    // skip spec chars if they were already added in this place
    int i = cursor.positionInBlock();
    if (i < block.text().length()) {
        if (typedChar == '"' || typedChar == '\'' || typedChar == ')' || typedChar == ']') {
            QChar ch = block.text().at(i);
            if (ch == typedChar) {
                cursor.setPosition(cursor.position()+1);
                setTextCursor(cursor);
                e->accept();
                return;
            }
        }
    }

    //autocomplete for "",'',(),[]
    if (_useAutoBrackets) {
        bool k1 = (evtxt == "\"");
        bool k2 = (evtxt == "'");
        bool k3 = (evtxt == "(");
        bool k4 = (evtxt == "[");

        if (k1 || k2 || k3 || k4 ) {
            QString s = block.text();
            int len = s.length();
            int p = cursor.positionInBlock();
            QChar c1, c2;
            if(p > 0)
                c1 = s.at(p-1);
            if(p < len)
                c2 = s.at(p);
            bool skip = false;
            if (!c1.isNull()) {
                skip = (k1 && c1 == '\"') || (k2 && c1 == 39);
            }
            if (k1) {//calc quotes count
                int i = -1, cnt = 0;
                while ( (i = s.indexOf("\"", i+1)) >= 0)
                    ++cnt;
                skip |= (cnt % 2 == 1);
            }
            if (!skip && !c2.isNull()) {
                skip = (k1 && c2 == '\"') || (k2 && c2 == 39) || (k3 && c2 == ')') || (k4 && c2 == ']');
                skip |= isIdent(c2);
            }
            if (!skip) {
                if (k1)
                    insertPlainText("\"\"");
                else if (k2)
                    insertPlainText("''");
                else if (k3)
                    insertPlainText("()");
                else if (k4)
                    insertPlainText("[]");
                cursor.setPosition(cursor.position()-1);
                setTextCursor(cursor);
                e->accept();
                return;
            }
        }
        //Disabled in options so do nothing
    }


    //dot, check for ClassInstance.{fields,funcs}
    if (key == 46) {
        int i = cursor.positionInBlock();
        bool flagShow = (i > 0 && isIdent(cursor.block().text()[i-1]));
        insertPlainText(".");
        if( flagShow ) {
            aucompShowList(false);
        }
        e->accept();
        return;
    }

    //
    if( ctrl && this->underMouse() ) {
        QGuiApplication::setOverrideCursor(QCursor(Qt::PointingHandCursor));
    }


    //select word in autocomplete list
    if( aucompIsVisible()) {
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
            onCompleteProcess();
            e->accept();
            return;
        }
    }

    bool checkNew = false;

    QString tab = CodeAnalyzer::tab();

    //Ctrl + v
    if ( (ctrl && key == Qt::Key_V) || (shift && key == Qt::Key_Insert) ) {
        onPaste(QApplication::clipboard()->text());
        e->accept();
        return;
    }
    //Ctrl + space
    else if( ctrl && key == Qt::Key_Space ) {
        aucompShowList( );
        e->accept();
        return;
    }
    else if( key == Qt::Key_Tab || key == Qt::Key_Backtab ) {
        //block tab/untab
        int tablen = tab.length();
        if( hasSel ) {
            int sel1 = cursor.selectionStart();
            int sel2 = cursor.selectionEnd();
            if(sel1 > sel2) {
                int v = sel1;
                sel1 = sel2;
                sel2 = v;
            }
            QTextBlock begb = document()->findBlock( sel1 );
            QTextBlock endb = document()->findBlock( sel2 );
            int beg = begb.blockNumber();
            int end = endb.blockNumber();
            //convert sel pos to in-block pos
            sel1 -= begb.position();
            sel2 -= endb.position();

            //qDebug()<<"beg,end: "<<beg<<","<<end<<"sel1,2:"<<sel1<<","<<sel2;
            cursor.beginEditBlock();
            if( key == Qt::Key_Backtab || shift ) {
                QTextBlock b = begb;
                for( int i = beg; i <= end; ++i, b = b.next() ) {
                    if( !b.length() )
                        continue;
                    cursor.setPosition( b.position() );
                    cursor.setPosition( b.position()+tablen,QTextCursor::KeepAnchor );
                    if( cursor.selectedText() != tab )
                        continue;
                    cursor.insertText("");
                    if(i == beg)
                        sel1 -= tablen;
                    if(i == end)
                        sel2 -= tablen;
                }
            }
            else {
                QTextBlock b = begb;
                for( int i = beg; i <= end; ++i, b = b.next() ) {
                    cursor.setPosition( b.position() );
                    cursor.insertText(tab);
                    if(i == beg)
                        sel1 += tablen;
                    if(i == end)
                        sel2 += tablen;
                }
            }
            cursor.setPosition(begb.position()+sel1);
            cursor.setPosition(endb.position()+sel2, QTextCursor::KeepAnchor);
            cursor.endEditBlock();
            setTextCursor(cursor);
        }
        else {
            if( key == Qt::Key_Backtab || shift ) {
                int i = cursor.position();
                cursor.setPosition(i-tablen, QTextCursor::KeepAnchor);
                if(cursor.selectedText() == tab) {
                    setTextCursor(cursor);
                    insertPlainText("");
                }
            }
            else {
                insertPlainText(tab);
            }
        }
        e->accept();
        return;
    }
    else if( key==Qt::Key_Enter || key==Qt::Key_Return ){
        //auto indent
        if( !hasSel ){
            int i;
            QString text = block.text();
            int len = text.length();
            int pos = cursor.positionInBlock();
            for( i = 0 ; i < pos && text[i] <= ' ' ; ++i ){}
            QString spaces = text.left(i), s = "";

            //if cursor isn't at the end of line
            if (pos != len) {
                insertPlainText("\n"+spaces);
                ensureCursorVisible();
                e->accept();
                return;
            }

            text = text.trimmed().toLower();
            int deltaPos = 0;
            QString closed = "";

            if (!text.isEmpty()) {
                bool isMethod = text.startsWith("method ");
                bool isFunc = text.startsWith("function ");
                if( isMethod || isFunc || text.startsWith("class ") || text.startsWith("interface ") || text.startsWith("select ") || text.startsWith("catch ") ) {
                    closed = "End";
                }
                else if(text.startsWith("if") || text.startsWith("else")) {
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

                // check return type of method
                // and add :Void if it's empty
                // exclude New()
                if (_addVoidForMethods && (isMethod || isFunc)) {
                    QString origText = block.text();
                    int i1 = origText.indexOf(":");
                    int i2 = origText.indexOf("(");
                    if ((i1 == -1 && i2 > 0) // has no :
                            || i2 < i1) { // has : but after (
                        int i = i2;
                        while (i > 0 && origText[i] != ' ') --i;
                        QString name = origText.mid(i+1, i2-i-1);
                        if (name != "New") {
                            int pp = cursor.position();
                            cursor.setPosition(block.position()+i2);
                            setTextCursor(cursor);
                            insertPlainText(":Void");
                            cursor.setPosition(pp+5);
                            setTextCursor(cursor);
                        }
                    }
                }
            }
            if (!closed.isEmpty()) {
                s = '\n'+spaces;
                bool add;
                add = !( text.startsWith("if") && (text.contains("then ") || text.contains("return")) );

                if (add && text.startsWith("method")) {
                    if (text.contains("abstract")) {
                        add = false;
                    }
                    else {
                        CodeItem *i = CodeAnalyzer::scopeAt(block, true);
                        if (i && i->isInterface())
                            add = false;
                    }
                }
                if (add)
                    s += tab;
                int len = closed.length();
                if (ctrl && len > 1) {
                    s += '\n'+spaces+closed;
                    deltaPos = -(i+len+1);
                }
            }
            else {
                s = '\n'+spaces;
            }
            cursor.insertText(s);
            if (deltaPos != 0) {
                cursor.setPosition( cursor.position()+deltaPos );
                setTextCursor(cursor);
            }
            ensureCursorVisible();
            //i = cursor.blockNumber();
            e->accept();
            return;
        }
    }

    if( key == Qt::Key_Backspace ) {
        if(ctrl) {//remove tab or all spaces, emulated tab
            cursor.setPosition(cursor.position()-tab.length(), QTextCursor::KeepAnchor);
            if(cursor.selectedText() == tab) {
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
                    cursor.setPosition(pos);
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
    if( !ctrl && key == Qt::Key_Home && block.length() > 1 ) {
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

    bool listShown = aucompIsVisible();

    if( e ) QPlainTextEdit::keyPressEvent( e );

    //auto ident for var = new ...
    if((key == ' ' && !ctrl && !shift) || checkNew) {
        /*QString s = block.text().trimmed();//.replace(" ","");
        if(s.endsWith("= New") || s.endsWith("=New")) {

            int i = s.length()-6;
            QChar c = s[i];
            qDebug()<<"char:"<<c;
            if(c != ':') {//not :=
                s = s.left(i).trimmed();
                qDebug()<<"s1:"<<s;
                i = s.lastIndexOf(' ');
                if(i) {
                    s = s.mid(i+1);
                    qDebug()<<"s2:"<<s;
                    i = s.indexOf(':');
                    if(i) {
                        s = s.mid(i+1);
                        qDebug()<<"s3:"<<s;

                    }
                }
            }
            e->accept();
            return;
        }*/

        //exit if we typed 'method ' and shown inherited methods
        if (!listShown && aucompIsVisible()) {
            e->accept();
            return;
        }
    }

    //
    if( aucompIsVisible() ) {
        if( key == Qt::Key_Escape) {
            onCompleteFocusOut();
            e->accept();
            return;
        }
        else if( (key >= Qt::Key_A && key <= Qt::Key_Z) || (key >= Qt::Key_0 && key <= Qt::Key_9) || key == Qt::Key_Backspace ) {
            if(key != Qt::Key_Backspace || !_lcomp->isForInheritance()) {
                aucompShowList( false );
            }
            e->accept();
            return;
        }
        else if(key != 46) {
            onCompleteFocusOut();
        }
    }


    if ( _monkey ){
        if ( key >= 32 && key <= 255 ) {
            if (!ctrl && !aucompIsVisible() && !block.text().trimmed().startsWith("'")) {
                QString ident = identAtCursor(false);
                if (ident.length() >= _charsCountForCompletion) {
                    aucompShowList( false );
                }
            }
        }
    }

}

bool CodeEditor::findNext( const QString &findText,bool cased,bool wrap,bool backward ){

    QTextDocument::FindFlags flags=0;
    if( cased )
        flags |= QTextDocument::FindCaseSensitively;
    if( backward )
        flags |= QTextDocument::FindBackward;

    bool found = find( findText,flags );

    if(found) {
        setCenterOnScroll( true );
    }

    if( !found && wrap ){

        setCenterOnScroll( false );

        QTextCursor cursor = textCursor();
        int i = cursor.position();
        int p = 0;
        if(backward)
            p = document()->lastBlock().position()+document()->lastBlock().length()-1;
        cursor.setPosition(p);
        setTextCursor(cursor);

        found = find( findText,flags );

        if( !found ) {
            cursor.setPosition(i);
            setTextCursor(cursor);
        }
    }

    setCenterOnScroll( false );

    /*if(found) {
        QTextCursor cursor = textCursor();
        qDebug()<<"cpib:"<<cursor.positionInBlock();
        int len = findText.length();
        int i = cursor.position()-len;
        cursor.setPosition(i);
        cursor.setPosition(i+len,QTextCursor::KeepAnchor);
        setTextCursor(cursor);
        document()->adjustSize();
    }*/


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
                ensureCursorVisible();
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
        indent = s.left(i);
    }
    clip = clip.replace("\r\n","\n");
    QStringList list = clip.split("\n");
    clip = "";
    //calc minimum indent of clipboard text
    int min = 1024;
    for (int k = 1, size = list.size(); k < size; ++k) {
        s = list.at(k);
        i = 0;
        n = s.length();
        while( i < n && s[i] <= ' ' ) ++i;
        if (i < min)
            min = i;
    }
    //delete indent for first line
    s = list.at(0).trimmed();
    clip = s;
    for (int k = 1, size = list.size(); k < size; ++k) {
        s = list.at(k);
        n = s.length()-min;
        clip += "\n"+indent+s.right(n);
    }
    insertPlainText(clip);
    ensureCursorVisible();
}

QString CodeEditor::identAtCursor(bool fullWord) {
    QTextCursor c = textCursor();
    QString s;
    if(fullWord) {
        c.select(QTextCursor::WordUnderCursor);
        s = c.selectedText();
    }
    else {
        QString text = c.block().text();
        int i0 = c.positionInBlock();
        int i = i0-1;
        while( i >= 0 && isIdent(text[i])) {
            --i;
        }
        ++i;
        s = text.mid(i, i0-i);
    }
    //qDebug()<<"identAtCursor: "+s;
    return (isAlpha(s[0]) ? s : "");
}

//***** Highlighter *****

Highlighter::Highlighter( CodeEditor *editor ):QSyntaxHighlighter( editor->document() ),_editor( editor ){

    connect( Prefs::prefs(),SIGNAL(prefsChanged(const QString&)),SLOT(onPrefsChanged(const QString&)) );
    _enabled = true;
    onPrefsChanged( "" );

}

Highlighter::~Highlighter(){

}

void Highlighter::onPrefsChanged( const QString &name ){
    QString t(name);
    if( t=="" || t.endsWith( "Color" ) ){
        Prefs *prefs = Prefs::prefs();
        _backgroundColor = prefs->getColor( "backgroundColor" );
        _defaultColor = prefs->getColor("defaultColor");
        _numbersColor = prefs->getColor("numbersColor");
        _stringsColor = prefs->getColor("stringsColor");
        _identifiersColor = prefs->getColor("identifiersColor");
        _keywordsColor = prefs->getColor("keywordsColor");
        _monkeywordsColor = prefs->getColor("monkeywordsColor");
        _userwordsColor = prefs->getColor("userwordsColor");
        _userwordsVarColor = prefs->getColor("userwordsVarColor");
        _userwordsDeclColor = prefs->getColor("userwordsDeclColor");
        _paramsColor = prefs->getColor("paramsColor");
        _commentsColor = prefs->getColor("commentsColor");
        _highlightColor = prefs->getColor("highlightColor");
        rehighlight();
        _editor->highlightCurrentLine();
    }
}

void Highlighter::highlightBlock( const QString &ctext ){

    //_enabled = false;
    if (!_enabled)
        return;

    if (!_editor->isCode()){
        setTextFormat( 0, ctext.length(), FormatDefault );
        return;
    }

    int i = 0, n = ctext.length();

    while( i < n && ctext[i] <= ' ' ) ++i;

    // is it empty line
    if (i == n)
        return;

    //qDebug() << "highlight:" << ctext;

    QTextBlock block = currentBlock();
    CodeItem *rem = CodeAnalyzer::remAt( block );
    if( rem ) {
        //qDebug()<<"rem: "+ctext;
        setTextFormat( 0, n, FormatComment );
        return;
    }

    bool monkeyFile = _editor->isMonkey();

    Formats format = FormatDefault;

    int prev = i;

    while( i < n ) {

        QChar c0 = (i > 0 ? ctext[i-1] : ' ');
        QChar c = ctext[i++];
        bool italic = false;

        if( c <= ' ' ) {
            //prev = i;
            while( i < n && ctext[i] <= ' ' ) {
                ++i;
            }
            prev = i;
            format = FormatDefault;
        }
        else if( isAlpha(c) ) {
            while( i < n && isIdent(ctext[i]) ) ++i;
            format = FormatDefault;
            QString ident = ctext.mid(prev,i-prev);
            //qDebug()<<"my_ident: "+ident;
            if (c0 == '.') {
                //format = FormatDefault;
            }
            else if ( CodeAnalyzer::containsKeyword(ident) ) {
                format = FormatKeyword;
                //try to capitalize
                bool allow = (i < n && !isIdent(ctext[i]));
                if (allow && ident[0].isLower()) {
                    QTextCursor c = _editor->textCursor();
                    int p0 = c.position();
                    int p1 = block.position()+prev;
                    c.setPosition(p1);
                    c.setPosition(p1+1, QTextCursor::KeepAnchor);
                    _editor->setTextCursor(c);
                    _editor->insertPlainText(ident[0].toUpper());
                    c.setPosition(p0);
                    _editor->setTextCursor(c);
                }
            }
            else {
                CodeItem *item = CodeAnalyzer::findInScope(block, i);
                if (item) {
                    if (item->isParam()) {
                        format = FormatParam;
                    }
                    else if (item->isUser()) {
                        if (item->isField()) {
                            format = FormatUserClassVar;
                        }
                        else if (item->block() == block) {
                            if (item->isClassOrInterface() || item->isFunc())//for local color is default
                                format = FormatUserDecl;
                        }
                        else if (item->isClassOrInterface()) {
                            format = FormatUserClass;
                        }
                    }
                    else if (item->isMonkey() && item->isClassOrInterface()) {
                        format = FormatMonkeyClass;
                    }
                    italic = (item->decl()=="const"||item->decl()=="global"||(item->parent()!=0 && item->decl()=="function"));
                }
            }
        }
        else if( c == '0' && !monkeyFile ) {
            if( i < n && ctext[i] == 'x' ) {
                for( ++i ; i < n && isHexDigit( ctext[i] ) ; ++i ){}
            }
            else {
                for( ;i<n && isOctDigit( ctext[i] );++i ){}
            }
            format = FormatNumber;
        }
        else if( isDigit(c) || (c == '.' && i < n && isDigit(ctext[i])) ){
            bool flt = (c=='.');
            while( i < n && isDigit(ctext[i]) ) ++i;
            if( !flt && i < n && ctext[i] == '.' ) {
                ++i;
                flt = true;
                while( i < n && isDigit(ctext[i]) ) ++i;
            }
            if( i < n && (ctext[i] == 'e' || ctext[i] == 'E') ){
                flt = true;
                if( i < n && (ctext[i] == '+' || ctext[i] == '-') ) ++i;
                while( i < n && isDigit(ctext[i]) ) ++i;
            }
            format = FormatNumber;
        }
        else if( c == '%' && monkeyFile && i < n && isBinDigit( ctext[i] ) ){
            for( ++i ; i < n && isBinDigit( ctext[i] ) ; ++i ){}
            format = FormatNumber;
        }
        else if( c == '$' && monkeyFile && i < n && isHexDigit( ctext[i] ) ){
            for( ++i ; i < n && isHexDigit( ctext[i] ) ; ++i ){}
            format = FormatNumber;
        }
        else if( c == '\"' ){
            if( monkeyFile ){
                for( ; i < n && ctext[i] != '\"' ; ++i ){}
            }
            else {
                for( ; i < n && ctext[i] != '\"' ; ++i ){
                    if( ctext[i] == '\\' && i+1 < n && ctext[i+1] == '\"' ) ++i;
                }
            }
            if( i < n )
                ++i;
            format = FormatString;
        }
        else if( c == '\'' ){
            if( monkeyFile ){
                for( ;i < n && ctext[i] != '\n' ; ++i ){}
                if( i < n ) ++i;
                format = FormatComment;
            }
        }
        else {
            format = FormatDefault;
        }

        if(i != prev) {
            setTextFormat(prev, i, format, italic);
            prev = i;
        }

    }

}

void Highlighter::setTextFormat(int start, int end, Formats format, bool italic) {
    //qDebug() << "settextformat: ["+QString::number(start)+","+QString::number(end)+"]";
    QTextCharFormat tcf;
    //QFont f(_editor->font());
    //tcf.setFont(f);
    QColor color = _defaultColor;
    if(format == FormatKeyword)
        color = _keywordsColor;
    else if(format == FormatString)
        color = _stringsColor;
    else if(format == FormatNumber)
        color = _numbersColor;
    else if(format == FormatComment)
        color = _commentsColor;
    /*else if(format == FormatParam)
        color = _paramsColor;*/
    else if(format == FormatUserClass) {
        color = _userwordsColor;
        //tcf.setFontWeight(QFont::Bold);
        //tcf.setFontItalic(true);
    }
    else if(format == FormatUserClassVar)
        color = _userwordsVarColor;
    else if(format == FormatUserDecl)
        color = _userwordsDeclColor;
    else if(format == FormatMonkeyClass)
        color = _monkeywordsColor;
    tcf.setForeground(QBrush(color));
    tcf.setFontItalic(italic);
    setFormat(start, end-start, tcf);
    /*if(curcol == _userwordsColor || curcol == _userwordsColorVar || curcol == _paramsColor) {

        tcf.setForeground(QBrush(curcol));
        setFormat( colst,i-colst,tcf );
        if(curcol == _paramsColor)
            tcf.setFontItalic(true);
    }
    else {
        setFormat( colst,i-colst,curcol );
    }*/
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
    foldType = 0;
}

BlockData::~BlockData(){
}

BlockData* BlockData::data( const QTextBlock &block, bool create) {
    BlockData *d = (block.isValid() ? dynamic_cast<BlockData*>(block.userData()) : 0);
    if( !d && create ) {
        //qDebug()<<"new block data:"<<block.text();
        d = new BlockData(block);
        QTextBlock b = block;
        b.setUserData(d);
    }
    return d;
}

void BlockData::flush(QTextBlock &block) {
    BlockData *d = BlockData::data(block);
    int mod = 0;
    bool bm = false;
    if(d) {
        //qDebug()<<"flush block data:"<<block.text();
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

CodeItem* BlockData::item(QString &ident) {
    //qDebug() << "blockdata.items.size: "+QString::number(_items.size());
    foreach (CodeItem *i, _items) {
        if(i->ident() == ident)
            return i;
    }
    return 0;
}
