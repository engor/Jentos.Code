/*
Jentos IDE.

Copyright 2014, Evgeniy Goroshkin.

See LICENSE.TXT for licensing terms.
*/

#include "codeanalyzer.h"
#include "listwidgetcomplete.h"
#include "codeeditor.h"


QStringList CodeAnalyzer::_userFiles;
QString CodeAnalyzer::_lastToolTipIdent;
QString CodeAnalyzer::_lastToolTipString;
QString CodeAnalyzer::_curFilePath;
bool CodeAnalyzer::_isShowVariables;
bool CodeAnalyzer::_isSortByName;
bool CodeAnalyzer::_disabled;

CodeAnalyzer::CodeAnalyzer(QObject *parent) :
    QObject(parent)
{
}

void CodeAnalyzer::init() {
    _isShowVariables = _isSortByName = true;
    _disabled = false;
}

void CodeAnalyzer::finalize() {
    _userFiles.clear();
    mapTemplates()->clear();
    mapMonkey()->clear();
    mapUser()->clear();
    mapKeywords()->clear();
    mapRem()->clear();
    mapFolds()->clear();
    delete mapTemplates();
    delete mapMonkey();
    delete mapUser();
    delete mapKeywords();
    delete mapRem();
    delete mapFolds();
}

void CodeAnalyzer::setShowVariables(bool value){
    _isShowVariables = value;
}
void CodeAnalyzer::setSortByName(bool value){
    _isSortByName = value;
}
void CodeAnalyzer::setCurFilePath(const QString &filepath) {
    _curFilePath = filepath;
}

QString CodeAnalyzer::templateWord( const QString &key ) {
    return mapTemplates()->value( key );
}

QString CodeAnalyzer::keyword( const QString &key ) {
    CodeItem *item = mapKeywords()->value(key.toLower(), 0);
    if(item)
        return item->ident();
    return "";
}

void CodeAnalyzer::loadTemplates( const QString &path ) {
    QFile file( path );
    if( file.open( QIODevice::ReadOnly ) ) {
        mapTemplates()->clear();
        QTextStream stream( &file );
        stream.setCodec( "UTF-8" );
        QString text = stream.readAll();
        file.close();
        int i1,i2=-1,len = text.length();
        while(true) {
            i1 = text.indexOf('<',i2+1);
            if(i1 < 0)
                break;
            i2 = text.indexOf('>',i1+1);
            if(i2 < 0)
                break;
            QString s = text.mid(i1+1,i2-i1-1);
            i1 = i2;
            i2 = text.indexOf("</"+s+">",i2+1);
            if(i2 > 0) {
                QString s2 = text.mid(i1+1,i2-i1-1);
                mapTemplates()->insert(s, s2);
            }
            else {
                i2 = i1;
            }
        }
    }
}

void CodeAnalyzer::loadKeywords( const QString &path ) {
    QString text;
    QFile file( path );
    if( file.open( QIODevice::ReadOnly ) ) {
        mapKeywords()->clear();
        QTextStream stream( &file );
        stream.setCodec( "UTF-8" );
        text = stream.readAll();
        file.close();
    }
    else {
        text = "Void;Strict;Public;Private;Property;"
            "Bool;Int;Float;String;Array;Object;Mod;Continue;Exit;"
            "Include;Import;Module;Extern;"
            "New;Self;Super;Eachin;True;False;Null;Not;"
            "Extends;Abstract;Final;Native;Select;Case;Default;"
            "Const;Local;Global;Field;Method;Function;Class;Interface;Implements;"
            "And;Or;Shl;Shr;End;If;Then;Else;Elseif;Endif;While;Wend;Repeat;Until;Forever;For;To;Step;Next;Return;Inline;"
            "Try;Catch;Throw;Throwable;"
            "Print;Error;Alias";
    }
    QStringList lines = text.split(';');
    QTextBlock block;
    foreach(QString s, lines){
        CodeItem *item = new CodeItem("keyword",s,0,block,"");
        mapKeywords()->insert(item->ident().toLower(), item);
    }
}

QMap<QString,QString>* CodeAnalyzer::mapTemplates() {
    static QMap<QString,QString> *m = 0;
    if( !m )
        m = new QMap<QString,QString>;
    return m;
}

QMap<QString, CodeItem *>* CodeAnalyzer::mapMonkey() {
    static QMap<QString,CodeItem*> *m = 0;
    if( !m )
        m = new QMap<QString,CodeItem*>;
    return m;
}

QMap<QString, CodeItem *>* CodeAnalyzer::mapUser() {
    static QMap<QString,CodeItem*> *m = 0;
    if( !m )
        m = new QMap<QString,CodeItem*>;
    return m;
}

QMap<QString, CodeItem *>* CodeAnalyzer::mapKeywords() {
    static QMap<QString,CodeItem*> *m = 0;
    if( !m )
        m = new QMap<QString,CodeItem*>;
    return m;
}

QMap<QString, CodeItem *>* CodeAnalyzer::mapRem() {
    static QMap<QString,CodeItem*> *m = 0;
    if( !m )
        m = new QMap<QString,CodeItem*>;
    return m;
}

QMap<QString, CodeItem *>* CodeAnalyzer::mapFolds() {
    static QMap<QString,CodeItem*> *m = 0;
    if( !m )
        m = new QMap<QString,CodeItem*>;
    return m;
}

CodeItem *CodeAnalyzer::findInScope(const QString &ident, const QTextBlock &block) {
    QList<CodeItem*> list = mapUser()->values();
    CodeItem *item, *res;
    int num = block.blockNumber();
    foreach (item, list) {
        if((item->isClass() || item->isFunc()) && item->filepath() == _curFilePath) {
            if(num >= item->block().blockNumber() && num <= item->blockEnd().blockNumber()) {
                if(item->isFunc()) {
                    res = item->child("", ident);
                    if(res) {
                        return res;
                    }
                }
                else {
                    res = item->child("", ident);
                    if(res) {
                        return res;
                    }
                    if(item->hasChildren()) {
                        QList<CodeItem*> list2 = item->children();
                        CodeItem *item2;
                        foreach (item2, list2) {
                            if(item2->isFunc() && num >= item2->block().blockNumber() && num <= item2->blockEnd().blockNumber()) {
                                res = item2->child("", ident);
                                if(res) {
                                    return res;
                                }
                            }
                        }
                    }
                }
                break;
            }
        }
    }
    item = mapUser()->value(ident,0);
    if(item && item->filepath() == _curFilePath) {
        return item;
    }
    return mapMonkey()->value(ident,0);
}

CodeItem* CodeAnalyzer::findScopeForIdent(const QString &ident, const QTextCursor &cursor, CodeScope &scope) {
    QTextBlock block = cursor.block();
    QString text = ident;
    int last = text.lastIndexOf(".")-1;
    int pos = last;
    bool findBrace = false;
    while( pos >= 0 ){
        bool norm = false;
        if(!findBrace) {
            norm = (isIdent(text.at(pos)) || text.at(pos) == '.');
            if(!norm) {
                if(text.at(pos) == ']') {
                    norm = true;
                    findBrace = true;
                }
            }
            if(!norm)
                break;
        }
        else {
            if(text.at(pos) == '[') {
                findBrace = false;
            }
        }
        --pos;
    }
    text = text.mid(pos+1,last-pos);
    QStringList list = text.split(".");
    //skip for array brackets []
    for(int k = 0; k < list.size(); ++k) {
        QString s = list.at(k);
        int i = s.indexOf("[");
        if(i) {
            s = s.left(i);
            list.replace(k, s);
        }
    }
    CodeItem *item = 0, *res = 0;
    if(list.size() > 0) {
        if(list.at(0).toLower() == "self") {
            item = findScopeForBlock(block, true);
            scope.set(item,true);
        }
        else if(list.at(0).toLower() == "super") {
            item = findScopeForBlock(block, true);
            scope.set(item,false,true);
        }
        else {
            item = findInScope(list.at(0), block);
            if(!item)
                item = itemUser(list.at(0));
            if(!item)
                item = itemMonkey(list.at(0));
            scope.set(item);
        }
        if(item) {
            res = item;
            if(list.size() == 1)
                return res;
            bool global = item->isClass();
            QString it = item->identTypeCleared();
            if(!global) {
                item = itemUser(it);
                if(!item)
                    item = itemMonkey(it);
            }
            if(item) {
                for(int k = 1; k < list.size(); ++k) {
                    item = item->child("", list.at(k));
                    if(!item)
                        break;
                    res = item;
                    scope.set(res);
                    it = item->identTypeCleared();
                    item = itemUser(it);
                    if(!item)
                        item = itemMonkey(it);
                }
            }
        }
    }
    return res;
}

CodeItem* CodeAnalyzer::findScopeForBlock(const QTextBlock &block, bool classOnly) {
    QList<CodeItem*> list = mapUser()->values();
    CodeItem *item, *res=0;
    int num = block.blockNumber();
    foreach (item, list) {
        if((item->isClass() || (!classOnly && item->isFunc()))&& item->filepath() == _curFilePath) {
            if(num >= item->block().blockNumber() && num <= item->blockEnd().blockNumber()) {
                if(item->isClass()) {
                    res = item;
                    if(classOnly || num == item->block().blockNumber())//class definition line
                        break;
                    QList<CodeItem*> list2 = item->children();
                    foreach (item, list2) {
                        if(item->isFunc()) {
                            if(num >= item->block().blockNumber() && num <= item->blockEnd().blockNumber()) {
                                res = item;
                                break;
                            }
                        }
                    }
                }
                else {
                    res = item;
                }
                break;
            }
        }
    }
    return res;
}

CodeItem* CodeAnalyzer::findRemForBlock(const QTextBlock &block) {
    QList<CodeItem*> list = mapRem()->values();
    CodeItem *item = 0;
    int num = block.blockNumber();
    foreach (item, list) {
        if(item->filepath() == _curFilePath && num >= item->block().blockNumber() && num <= item->blockEnd().blockNumber()) {
            return item;
        }
    }
    return 0;
}

CodeItem* CodeAnalyzer::findFoldForBlock(const QTextBlock &block) {
    QList<CodeItem*> list = mapFolds()->values();
    CodeItem *item = 0;
    int num = block.blockNumber();
    foreach (item, list) {
        if(item->filepath() == _curFilePath && num >= item->block().blockNumber() && num <= item->blockEnd().blockNumber()) {
            return item;
        }
    }
    return 0;
}

QStringList CodeAnalyzer::extractParams(const QString &text) {
    QStringList list;
    int st1=0, st2=0, st3=0, pos = 0;
    QChar ch;
    QString s;
    int len = text.length();
    for(int k = 0; k < len; ++k) {
        ch = text.at(k);
        bool end = (k == len-1);
        if(end) {
            s = text.mid(pos,len-pos).trimmed();
            list.append(s);
        }
        else if(ch == ',') {
            if( st1 <= 0 && st3 <= 0 && st2 % 2 == 0 ) {
                s = text.mid(pos,k-pos).trimmed();
                list.append(s);
                pos = k+1;
            }
        }
        else if(ch == '[')
            ++st1;
        else if(ch == ']')
            --st1;
        else if(ch == '\"')
            ++st2;
        else if(ch == '(')
            ++st3;
        else if(ch == ')')
            --st3;

    }
    return list;
}

void CodeAnalyzer::clearMonkey() {
    mapMonkey()->clear();
}

void CodeAnalyzer::analyzeDir( const QString &path, const QStringList &exclude ) {

    QDir dir(path);
    QStringList filters;
    filters << "*.monkey";
    QStringList list = dir.entryList(dir.filter());
    QString file;
    for( int k = 0; k < list.size() ; ++k ) {
        file = list.at(k);
        if(file=="." || file=="..")
            continue;
        QFileInfo info(dir,file);
        if(info.isDir()) {
            if(!exclude.contains(file))
                analyzeDir(path+file+"/", exclude);
        }
        else if(extractExt(file) == "monkey"){
            analyzeFile(path+file);
        }
    }
}

void CodeAnalyzer::analyzeFile( const QString &path ) {
    QFile file(path);
    if( file.open( QIODevice::ReadOnly ) ) {
        QTextStream stream( &file );
        stream.setCodec( "UTF-8" );
        QString text = stream.readAll();
        file.close();
        QTextDocument doc;
        doc.setPlainText(text);
        parse(doc.firstBlock(), path, KIND_MONKEY);
    }
}

bool CodeAnalyzer::needCloseWithEnd( const QString &line ) {
    static QString arr[] = {"method","class", "function","select","try"};
    int i = line.indexOf("'");//comment in line
    QString s = (i < 0 ? line : line.left(i));
    if(s.startsWith("interface"))
        return false;
    if(s.startsWith(arr[0]) && s.indexOf("abstract") < 0)
        return true;
    for( int k = 1 ; k < 5 ; ++k ) {
        if(s.startsWith(arr[k]))
            return true;
    }
    return false;
}

QIcon CodeAnalyzer::identIcon( const QString &ident ) {
    static QIcon iconst( ":/code/const.png" );
    static QIcon iglob( ":/code/global.png" );
    static QIcon ifield( ":/code/property.png" );
    static QIcon imethod( ":/code/method.png" );
    static QIcon ifunc( ":/code/function.png" );
    static QIcon iclass( ":/code/class.png" );
    static QIcon iinterf( ":/code/interface.png" );
    static QIcon ikeyword( ":/code/keyword.png" );
    static QIcon iother( ":/code/other.png" );
    QString s = ident;//.toLower();
    if(s == "const")
        return iconst;
    if(s == "global")
        return iglob;
    if(s == "field")
        return ifield;
    if(s == "method")
        return imethod;
    if(s == "function")
        return ifunc;
    if(s == "class")
        return iclass;
    if(s == "interface")
        return iinterf;
    if(s == "keyword")
        return ikeyword;
    return iother;
}

void CodeAnalyzer::insertItem(QString ident, CodeItem *codeItem, int kind) {

    if(kind == KIND_MONKEY) {
        mapMonkey()->insert(ident, codeItem);
        codeItem->setIsMonkey(true);
    }
    else {
        mapUser()->insert(ident, codeItem);
        codeItem->setIsUser(true);
    }
}

void CodeAnalyzer::remove(CodeItem *item) {
    if(item) {
        mapUser()->remove(item->ident());
    }
}

void CodeAnalyzer::removeByPath(const QString &path) {
    QList<CodeItem*> list = mapUser()->values();
    foreach (CodeItem *i, list) {
        if(i->filepath() == path) {
            mapUser()->remove(i->ident());
            delete i;
        }
    }
    list = mapRem()->values();
    foreach (CodeItem *i, list) {
        if(i->filepath() == path) {
            mapRem()->remove(i->ident());
            delete i;
        }
    }
    list = mapFolds()->values();
    foreach (CodeItem *i, list) {
        if(i->filepath() == path) {
            mapFolds()->remove(i->ident());
            delete i;
        }
    }
}

void CodeAnalyzer::removeUserFile(const QString &path) {
    if(!_userFiles.isEmpty()) {
        removeByPath(path);
        _userFiles.removeOne(path);
    }
}

bool CodeAnalyzer::parse(QTextBlock block, const QString &path , int kind) {
    if(_disabled)
        return false;
    //
    if(kind == KIND_USER && !_userFiles.contains(path))
        _userFiles.append(path);

    //remove previous items
    //need to fix this
    removeByPath(path);

    _curFilePath = path;

    CodeItem *containerClass = 0, *containerFunc = 0, *remBlock = 0, *foldBlock = 0;
    bool isPrivateInFile = false;
    bool isPrivateInClass = false;

    while(block.isValid()) {

        BlockData::flush(block);

        QString line = block.text();
        int len = line.length();
        int indent = 0;
        while( indent < len && line.at(indent) <= ' ' )
            ++indent;
        if(indent > 0)
            line = line.mid(indent);

        //comments
        if(line.startsWith("'")) {
            if(kind == KIND_USER) {
                QString s1 = "'/**", s2 = "'**/";
                if(line.startsWith(s1)) {
                    foldBlock = new CodeItem("","fold"+QString::number(block.blockNumber()),0,block,path,0);
                    foldBlock->setFoldable(true);
                    mapFolds()->insert(foldBlock->ident(),foldBlock);
                }
                else if(foldBlock && line.startsWith(s2)) {
                    foldBlock->setBlockEnd(block);
                    foldBlock = 0;
                }
            }
            block = block.next();
            continue;
        }
        //
        int i = line.indexOf(' ');
        QString lower = line.toLower();
        if(i < 0 || lower.startsWith("end ")) {
            line = lower;

            if(line.startsWith("#rem")) {
                if(kind == KIND_USER) {
                    remBlock = new CodeItem("","rem"+QString::number(block.blockNumber()),0,block,path,0);
                    remBlock->setFoldable(true);
                    mapRem()->insert(remBlock->ident(),remBlock);
                    //qDebug() << "rem starts: "+block.text()+", at: "+QString::number(block.blockNumber());
                }
                int st = 0;
                while(block.isValid()) {
                    line = block.text().trimmed().toLower();
                    if(line.startsWith("#if")) {
                        ++st;
                    }
                    else if(line == "#end") {
                        if(st == 0) {
                            if(remBlock) {
                                remBlock->setBlockEnd(block);
                                remBlock = 0;
                            }
                            break;
                        }
                        else
                            --st;
                    }
                    block = block.next();
                }
                block = block.next();
                continue;
            }
            //
            if(line == "end" || line.startsWith("end ")) {
                if(containerFunc) {
                    if(indent == containerFunc->indent()) {
                        containerFunc->setFoldable(true);
                        containerFunc->setBlockEnd(block);
                        containerFunc = 0;
                    }
                }
                if(containerClass) {
                    if(indent == containerClass->indent()) {
                        containerClass->setBlockEnd(block);
                        containerClass = 0;
                        isPrivateInClass = false;
                    }
                }

            }
            else if(line == "private") {
                if(containerClass)
                    isPrivateInClass = true;
                else
                    isPrivateInFile = true;
            }
            else if(line == "public") {
                if(containerClass)
                    isPrivateInClass = false;
                else
                    isPrivateInFile = false;
            }
            block = block.next();
            continue;
        }
        if(isPrivateInFile || isPrivateInClass) {
            block = block.next();
            continue;
        }
        QString decl = line.left(i).toLower();
        line = line.mid(i+1).trimmed();
        i = line.indexOf("'");
        if(i)
            line = line.left(i);
        if(kind == KIND_MONKEY) {
            autoFormat(line);
        }
        if(decl == "class" || decl == "interface") {
            CodeItem *item = new CodeItem(decl, line, indent, block, path, 0);
            item->setFoldable(true);
            insertItem(item->ident(), item, kind);
            containerClass = item;
        }
        else if(decl == "field" || decl == "global" || decl == "const") {
            QStringList l = extractParams(line);
            foreach (QString s, l) {
                s = s.trimmed();
                CodeItem *item = new CodeItem(decl, s, indent, block, path, 0);
                if(containerClass) {
                    containerClass->addChild(item);
                    if(kind == KIND_MONKEY && decl == "global")
                        insertItem(item->ident(), item, kind);
                }
                else {
                    insertItem(item->ident(), item, kind);
                }
            }
        }
        else if(decl == "method" || decl == "function") {
            CodeItem *item = new CodeItem(decl, line, indent, block, path, 0);
            if(containerClass) {
                containerClass->addChild(item);
                if(kind == KIND_MONKEY && decl == "function")
                    insertItem(item->ident(), item, kind);
            }
            else {
                insertItem(item->ident(), item, kind);
            }
            containerFunc = item;
        }
        else if(decl == "local") {
            if(containerFunc || containerClass) {
                QStringList l = extractParams(line);
                foreach (QString s, l) {
                    s = s.trimmed();
                    CodeItem *item = new CodeItem(decl, s, indent, block, path, 0);
                    if(containerFunc) {
                        containerFunc->addChild(item);
                    }
                    else if(containerClass) {
                        containerClass->addChild(item);
                    }
                }
            }
        }
        block = block.next();
    }
    return true;
}

bool CodeAnalyzer::containsMonkey( const QString &ident ) {
    return mapMonkey()->contains(ident);
}

bool CodeAnalyzer::containsUser( const QString &ident ) {
    return mapUser()->contains(ident);
}

bool CodeAnalyzer::containsKeyword( const QString &ident ) {
    return mapKeywords()->contains(ident.toLower());
}

CodeItem* CodeAnalyzer::itemMonkey(const QString &ident ) {
    return mapMonkey()->value(ident, 0);
}

CodeItem* CodeAnalyzer::itemUser(const QString &ident, bool withChildren ) {
    QList<CodeItem*> items = mapUser()->values();
    foreach(CodeItem *i, items){
        if(i->ident() == ident && i->filepath() == _curFilePath) {
            return i;
        }
        if(withChildren && i->hasChildren()) {
            QList<CodeItem*> items2 = i->children();
            foreach(CodeItem *i2, items2){
                if(i2->ident() == ident && i->filepath() == _curFilePath)
                    return i2;
            }
        }
    }
    return 0;
}

CodeItem* CodeAnalyzer::findItem(CodeItem *parent, const QString &ident) {
    if(parent->ident() == ident)
        return parent;
    foreach (CodeItem *i, parent->children()) {
        if(i->ident() == ident)
            return i;
        if(i->hasChildren()) {
            CodeItem *r = findItem(i, ident);
            if(r)
                return r;
        }
    }
    return 0;
}

CodeItem* CodeAnalyzer::classItem( const QString &ident ) {
    QString s = ident;
    CodeItem *item = mapMonkey()->value(s);
    if( item && item->decl()=="class")
        return item;
    return 0;
}

void CodeAnalyzer::fillListCommon( QListWidget *l, const QString &ident, const QTextBlock &block ) {

    CodeItem *item;
    ListWidgetCompleteItem *lwi;
    QIcon icon;
    QList<CodeItem*> list = mapKeywords()->values();

    QList<CodeItem*> listRes;

    foreach (item, list) {
        if( ident == "" || item->ident().startsWith(ident, Qt::CaseInsensitive) ) {
            CodeItem::addUnique(listRes,item);
        }
    }

    CodeItem *scope = findScopeForBlock(block);
    if(scope) {
        list = scope->children();
        foreach (item, list) {
            if( ident == "" || item->ident().startsWith(ident, Qt::CaseInsensitive) ) {
                CodeItem::addUnique(listRes,item);
            }
        }
        if(scope->parent()) {
            list = scope->parent()->children();
            foreach (item, list) {
                if( ident == "" || item->ident().startsWith(ident, Qt::CaseInsensitive) ) {
                    CodeItem::addUnique(listRes,item);
                }
            }
        }
    }

    list = mapUser()->values();
    foreach (item, list) {
        if( ident == "" || item->ident().startsWith(ident, Qt::CaseInsensitive) ) {
            CodeItem::addUnique(listRes,item);
        }
    }

    list = mapMonkey()->values();
    foreach (item, list) {
        if( ident == "" || item->ident().startsWith(ident, Qt::CaseInsensitive) ) {
            CodeItem::addUnique(listRes,item);
        }
    }

    foreach (item, listRes) {
        icon = identIcon(item->decl());
        lwi = new ListWidgetCompleteItem(icon, item->descrAsItem(), item, l);
        lwi->setToolTip( item->toString() );
        l->addItem( lwi );
    }

}

void CodeAnalyzer::fillListVarType( QListWidget *l, const QString &vartype ) {
    CodeItem *item;
    ListWidgetCompleteItem *lwi;
    QIcon icon;
    QList<CodeItem*> list = mapKeywords()->values();
    foreach (item, list) {
        if( item->ident() == vartype ) {
            icon = identIcon(item->decl());
            lwi = new ListWidgetCompleteItem(icon, item->descrAsItem(), item, l);
            lwi->setToolTip( item->toString() );
            l->addItem( lwi );
        }
    }
}

void CodeAnalyzer::fillListByChildren( QListWidget *l, const QString &ident, CodeScope scope ) {

    CodeItem *item;
    ListWidgetCompleteItem *lwi;
    QIcon icon;
    item = scope.item;
    bool global = (item->isClass() && !scope.isSelf && !scope.isSuper);
    QString identType = item->identTypeCleared();
    QStringList templField = item->templWords();
    bool hasTempl = !templField.isEmpty();
    item = itemUser(identType);
    if(!item)
        item = itemMonkey(identType);
    if(!item) {
        return;
    }
    CodeItem *itemWithBase = scope.item;
    if(!scope.isSelf && !scope.isSuper)
        itemWithBase = item;
    QList<CodeItem*> list;
    if(!scope.isSuper) {
        list = item->children();
        QStringList templClass;
        if(hasTempl) {
            templClass = item->templWords();
            hasTempl = (!templField.isEmpty() && templClass.size() == templField.size());
        }
        foreach (item, list) {
            //skip local if needs globals only
            if(global && (item->decl() != "function" && item->decl() != "global" && item->decl() != "const")) {
                continue;
            }
            if( (ident == "" || item->ident().startsWith(ident,Qt::CaseInsensitive)) && item->ident() != "Self" ) {
                icon = identIcon(item->decl());
                QString descr = item->descrAsItem();
                if(hasTempl) {
                    for(int k = 0, size = templField.size(); k < size; ++k) {
                        descr = descr.replace(":"+templClass.at(k), ":"+templField.at(k));
                    }
                }
                lwi = new ListWidgetCompleteItem(icon, descr, item, l);
                lwi->setToolTip( item->toString() );
                l->addItem( lwi );
            }
        }
    }
    //from base classes
    QStringList baseClasses = itemWithBase->baseClasses();
    foreach (QString s, baseClasses) {
        CodeItem *base = itemUser(s);
        if(!base)
            base = itemMonkey(s);
        if(base) {
            if(scope.isSuper && base->decl() != "class")
                continue;
            list = base->children();
            int k = 0;
            foreach (item, list) {
                if(global && (item->decl() != "function" && item->decl() != "global" && item->decl() != "const")) {
                    continue;
                }
                if( (ident == "" || item->ident().startsWith(ident,Qt::CaseInsensitive)) && item->ident() != "Self" ) {
                    if(k == 0) {
                        icon = identIcon("keyword");
                        lwi = new ListWidgetCompleteItem(icon, "... from :: "+base->ident()+" ::", base, l);
                        l->addItem( lwi );
                    }
                    icon = identIcon(item->decl());
                    lwi = new ListWidgetCompleteItem(icon, item->descrAsItem(), item, l);
                    lwi->setToolTip( item->toString() );
                    l->addItem( lwi );
                    ++k;
                }
            }
        }
    }
}

void CodeAnalyzer::fillTree(QStandardItemModel *im) {
    im->clear();
    QStandardItem *root = im->invisibleRootItem();

    QStandardItem *parent;
    QList<CodeItem*> list = mapUser()->values();
    CodeItem *item, *childItem;
    foreach (QString file, _userFiles) {
        parent = root;
        ItemWithData *treeItem = new ItemWithData;
        treeItem->setText(stripDir(file));
        QFont f = treeItem->font();
        f.setBold(true);
        treeItem->setFont(f);
        treeItem->setCode(0);
        parent->appendRow(treeItem);
        parent = treeItem;
        foreach (item, list) {
            if(item->filepath() != file)
                continue;
            ItemWithData *treeItem = new ItemWithData(identIcon(item->decl()), item->descrAsItem());
            treeItem->setCode(item);
            item->setItemWithData(treeItem);
            item->setItemWithData(treeItem);
            parent->appendRow(treeItem);
            if(item->hasChildren()) {
                QList<CodeItem*> children = item->children();
                foreach (childItem, children) {
                    if(!_isShowVariables && childItem->isField())
                        continue;
                    ItemWithData *itch = new ItemWithData(identIcon(childItem->decl()), childItem->descrAsItem());
                    itch->setCode(childItem);
                    childItem->setItemWithData(itch);
                    childItem->setItemWithData(itch);
                    treeItem->appendRow(itch);
                }
            }
        }
        if(_isSortByName)
            im->sort(0);
    }
}

CodeItem* CodeAnalyzer::itemAtBlock(const QString &ident, const QTextBlock &block) {
    CodeItem *item = mapKeywords()->value(ident.toLower(),0);
    if(item)
        return item;
    QList<CodeItem*> list = mapUser()->values();
    CodeItem *main=0;
    int num = block.blockNumber();
    int n, val = 0;
    foreach (item, list) {
        n = item->block().blockNumber();
        if(n < num) {
            if(n > val) {
                main = item;
                val = n;
            }
        }
        else if(n == num) {
            return item;
        }
    }
    if(main) {
        list = main->children();
        foreach (item, list) {
            if(item->ident() == ident) {
                return item;
                break;
            }
        }
        //not found in this class
        item = mapUser()->value(ident,0);
        if(item)
            return item;
    }
    return mapMonkey()->value(ident,0);
}

QString CodeAnalyzer::toolTip(const QString &ident, const QTextBlock &block) {
    if(ident == _lastToolTipIdent) {
        return _lastToolTipString;
    }
    CodeItem *item = itemAtBlock(ident, block);
    _lastToolTipIdent = ident;
    _lastToolTipString = toolTip(item);
    return _lastToolTipString;
}

QString CodeAnalyzer::toolTip(CodeItem *item) {
    if(!item)
        return "";
    QString s="";
    if(item->isKeyword()) {
        s = "(keyword) <b>"+item->ident()+"</b>";
    }
    else {
        s = "("+item->decl()+") <b>"+item->descrAsItem()+"</b>\n<i>Declared in:</i> "+item->module()+"\n<i>at line "+QString::number(item->lineNumber())+"</i>";
    }
    return s;
}

bool CodeAnalyzer::autoFormat( QString &s ) {
    //replace tabs at beginning of string with 4 spaces
    int i = 0;
    bool repl = false;
    int len = s.length();
    QString s2 = "";
    while( i < len && s[i] < ' ' ) {
        if(s[i] == 9) {
            s2 += "    ";
            repl = true;
        }
        else {
            s2 += s[i];
        }
        ++i;
    }
    if(repl) {
        s = s2+s.mid(i);
    }
    QString trimmed = s.trimmed();
    if(trimmed.length() == 0)
        return repl;
    static QStringList *sFrom = 0, *sTo = 0;
    static int cnt = 0;
    if(!sFrom) {
        sFrom = new QStringList;
        sTo = new QStringList;
        sFrom->append("#");
        sTo->append(":Float");
        sFrom->append("$");
        sTo->append(":String");
        sFrom->append("%");
        sTo->append(":Int");
        sFrom->append("?");
        sTo->append(":Bool");
        sFrom->append("/*");
        sTo->append("#Rem");
        sFrom->append("*/");
        sTo->append("#End");
        cnt = 6;
    }

    int pos = -1;
    bool checkQuotes = (s.indexOf("\"") > 0);
    if(trimmed.indexOf("'") == 0) {//skip comments
        return false;
    }
    if(trimmed.indexOf("#") == 0) {//skip preprocessor directives
        return false;
    }
    int comPos = s.indexOf("'");
    for(i = 0 ; i < cnt ; ++i) {
        pos = -1;
        while( (pos = s.indexOf(sFrom->at(i), pos+1)) >= 0) {
            if(comPos >= 0 && pos > comPos)
                break;
            if(i < 4 && pos == 0)
                continue;
            if(checkQuotes) {
                bool quo = isBetweenQuotes(s, pos);
                if(quo)
                    continue;
            }
            if(i < 4 && !isIdent(s.at(pos-1)))
                continue;
            s = s.left(pos) + sTo->at(i) + s.mid(pos+sFrom->at(i).length());
            repl = true;
        }
    }
    int len2 = s.length();
    //trim right spaces, if contains non-spacing char
    for( pos = len2-1 ; pos >= 0 && s.at(pos) <= ' ' ; --pos ) {}

    if( pos != len2-1 && pos >= 0 ) {
        s = s.left(pos+1);
        repl = true;
    }

    bool iff = (trimmed.startsWith("if") || trimmed.startsWith("If"));
    //autoformat
    len2 = s.length()-1;
    QString res = "";
    for(int k = 0; k < len2; ++k) {
        QChar c = s.at(k);
        QString ins = c;
        bool quo = false;
        if(checkQuotes)
            quo = isBetweenQuotes(s, k);
        if(!quo) {
            if(c == ',' && s.at(k+1) != ' ') {
                ins = ", ";
                repl = true;
            }
            else {
                QString ss = s.mid(k,2);
                if(ss == ":=" || ss == "+=" || ss == "-=" || ss == "/=" || ss == "*=" || ss == "~=" || ss == "&=" || ss == "|=" || ss == "<=" || ss == ">=" || ss == "<>") {
                    ins = " "+ss+" ";
                    repl = true;
                    ++k;
                }
                else if(trimmed.at(0) !='#') {
                    if(c == '=' || (iff && (c == '<' || c == '>'))) {
                        ins = " "+ins+" ";
                        repl = true;
                    }
                }
            }
        }
        res += ins;
        if(k == len2-1)
            res += s.at(k+1);
    }
    s = res;
    //clear spaces
    len2 = s.length();
    res = "";
    QChar c, prev;
    bool indent = true;
    int q = 0;
    for(int k = 0; k < len2; ++k) {
        c = s.at(k);
        if(indent && c <= ' ') {
            res += ' ';
            continue;
        }
        indent = false;
        if(c == '\"') {
            ++q;
        }
        //if not between quotes
        if(!(q > 0 && q % 2 == 1) && (k > 0 && prev == ' ' && c == prev)) {
            continue;
        }
        res += c;
        prev = c;
    }
    if(s != res) {
        s = res;
        repl = true;
    }
    return repl;
}

bool CodeAnalyzer::isBetweenQuotes(QString text, int pos) {
    int q = text.indexOf("\"");
    if(q < 0)
        return false;
    int st = 1, end = -1;
    int begin = q;
    while( (q = text.indexOf("\"", q+1)) > 0 ) {
        if(st == 0) {
            begin = q;
            st = 1;
        }
        else if(st == 1) {
            end = q;
            st = 0;
        }
        if(pos > begin && pos < end) {
            return true;
        }
    }
    return false;
}

QString CodeAnalyzer::clearSpaces( QString &s ) {
    int len = s.length();
    QString rez = "";
    bool space = false;
    for(int k = 0; k < len; ++k) {
        if(s.at(k) != ' ') {
            if(space)
                rez += " ";
            rez.append(s.at(k));
            space = false;
        }
        else {
            space = true;
        }
    }
    return rez;
}


//----------- CODE ITEM -----------------------------

CodeItem::CodeItem(QString decl, QString line, int indent, QTextBlock &block, const QString &path, CodeItem *parent) {
    line = CodeAnalyzer::clearSpaces(line);
    _decl = decl;
    _indent = indent;
    _parent = parent;
    _block = block;
    _blockEnd = block;
    _descr = _descrAsItem = line;
    _identType = "Int";
    _foldable = false;
    _isClass = _isFunc = _isField = _isKeyword = _isMonkey = _isUser = _isParam = false;
    _identForInsert = "";
    _filepath = path;
    _module = "";
    _lineNumber = block.blockNumber();

    int i = path.indexOf("modules");
    if(i > 0) {
        _module = path.mid(i+8);
        _module = _module.left(_module.length()-7);// .monkey
    }

    if(decl == "class" || decl == "interface") {
        //store base class and interfaces
        int iext = line.toLower().indexOf("extends");
        int iimpl = line.toLower().indexOf("implements");
        QString base;
        if(iext > 0) {
            int last = line.length();
            if(iimpl > 0)
                last = iimpl-1;
            base = line.mid(iext+8,last-(iext+8));
            base = base.trimmed();
            _baseClasses.append(base);
        }
        if(iimpl > 0) {
            base = line.mid(iimpl+11,line.length());
            base = base.trimmed();
            QStringList l = base.split(",");
            foreach (QString s, l) {
                s = s.trimmed();
                _baseClasses.append(s);
            }
        }

        _ident = line;
        //templates: Map <K, V>
        i = line.indexOf("<");
        if(i > 0 && (iext < 0 || i < iext) && (iimpl < 0 || i < iimpl)) {
            int i2 = line.indexOf(">");
            if(i2) {
                QString templ = line.mid(i+1,i2-i-1);
                templ = templ.trimmed();
                QStringList l = templ.split(",");
                foreach (QString s, l) {
                    s = s.trimmed();
                    _templWords.append(s);
                }
            }
            _ident = _ident.left(i);
        }

        int i = line.indexOf(" ");
        if(i > 0)
            _ident = line.left(i);
        if(_ident.startsWith("@")) {//correct for @String, @Array etc.
            _ident = _ident.mid(1);
        }
        i = _ident.indexOf("<");
        if(i > 0) {
            _ident = _ident.left(i);
        }
        _descrAsItem = _ident;
        _identType = _ident;
        _isClass = true;

    }
    else if(decl == "method" || decl == "function") {
        int i1 = line.indexOf("(");
        int i2 = line.indexOf(":");
        if(i1 < i2 || i2 < 0) {
            _ident = line.left(i1);
        }
        else if(i2) {
            _ident = line.left(i2);
            _identType = line.mid(i2+1, i1-i2-1);
        }
        //
        if( i1 ) {
            i2 = line.indexOf(")");
            if(!i2)
                i2 = line.length();
            _descr = _descrAsItem = line.left(i2+1);
            int len = i2-i1-1;
            if(len > 0) {
                line = line.mid(i1+1,len);
                _params = CodeAnalyzer::extractParams(line);
                foreach (QString s, _params) {
                    s = s.trimmed();
                    CodeItem *item = new CodeItem("param", s, indent, block, path, 0);
                    this->addChild(item);
                }
            }
        }
        _isFunc = true;
    }
    else if(decl == "field" || decl == "global" || decl == "const" || decl == "local" || decl == "param") {
        int i = line.indexOf("=");
        if(i)
            line = line.left(i).trimmed();
        i = line.indexOf(":");
        if(i) {
            _identType = line.mid(i+1);
            line = line.left(i);
            //templates: Map <K, V>
            i = _identType.indexOf("<");
            if(i > 0) {
                int i2 = _identType.indexOf(">");
                if(i2) {
                    QString templ = _identType.mid(i+1,i2-i-1);
                    templ = templ.trimmed();
                    QStringList l = templ.split(",");
                    foreach (QString s, l) {
                        s = s.trimmed();
                        _templWords.append(s);
                    }
                }
            }
        }
        _ident = line;
        _descrAsItem = _ident+":"+_identType;
        _isField = true;
    }
    else if(decl == "keyword") {
        _ident = line;
        if( line=="Include"||line=="Import"||line=="Module"||line=="Extern"||
                line=="New"||line=="Eachin"||
                line=="Extends"||/*topic=="Abstract"||topic=="Final"||*/line=="Native"||line=="Select"||line=="Case"||
                line=="Const"||line=="Local"||line=="Global"||line=="Field"||line=="Method"||line=="Function"||line=="Class"||line=="Interface"||line=="Implements"||
                line=="And"||line=="Or"||
                line=="Until"||line=="For"||line=="To"||line=="Step"||
                line=="Catch"||line=="Print" ) {
            _identForInsert = _ident+ " ";
        }
        _isKeyword = true;
    }
    else {
        _ident = line;
    }
    if(_identForInsert == "")
        _identForInsert = _ident;
    if(_module == "")
        _module = _filepath;
    else
        _module = _module.replace("/",".");
    //store codeitem as block user data
    BlockData *d = BlockData::data(block,true);
    d->setCode(this);
}

CodeItem::~CodeItem() {
    _parent = 0;
    if(hasChildren()) {
        foreach (CodeItem *i, children()) {
            delete i;
        }
        children().clear();
    }
}

QString CodeItem::descrAsItem() {
    return _descrAsItem;
}

void CodeItem::addChild(CodeItem *item) {
    _children.append(item);
    item->setParent(this);
}

int CodeItem::lineNumber() {
    if(isMonkey())
        return _lineNumber+1;
    else if(_block.isValid())
        return _block.blockNumber()+1;
}

CodeItem* CodeItem::child(const QString &decl, const QString &ident) {
    foreach (CodeItem *item, children()) {
        if((decl == "" || item->decl()==decl) && item->ident()==ident)
            return item;
    }
    return 0;
}

void CodeItem::addUnique(QList<CodeItem*> &list, CodeItem *item) {
    /*bool found = false;
    foreach (CodeItem *i, list) {
        if(i->fullDescr() == item->fullDescr()) {
            found = true;
            break;
        }
    }
    if(!found)*/
        list.push_back(item);
}

void CodeItem::fillByChildren(const QString &decl, const QString &identStarts, QList<CodeItem*> &list) {
    if((decl == "" || this->decl()==decl) && this->ident().startsWith(identStarts,Qt::CaseInsensitive)) {
        CodeItem::addUnique(list,this);
    }
    foreach (CodeItem *item, children()) {
        if((decl == "" || item->decl()==decl) && item->ident().startsWith(identStarts,Qt::CaseInsensitive)) {
            CodeItem::addUnique(list,item);
        }
        if(item->hasChildren())
            item->fillByChildren(decl,identStarts,list);
    }
}

void CodeItem::setFoldable(bool value) {
    _foldable = value;
    QTextBlock b = block();
    BlockData *d = BlockData::data(b);
    if(d) {
        d->setFoldable(value);
        d->setCode(this);
    }
}

void CodeItem::setBlockEnd(const QTextBlock &block) {
    _blockEnd = block;
    QTextBlock b = this->block();
    BlockData *d = BlockData::data(b);
    if(d)
        d->setBlockEnd(block);
}

QString CodeItem::identWithParamsBraces(int &cursorDelta) {
    QString line = identForInsert();
    int d = 0;
    if(isFunc()) {
        int len = params().length();
        if( len > 0 ) {
            line += "(";
            for( int k = 0 ; k < len-1 ; ++k ) {
                line += ",";
                --d;
            }
            line += ")";
            --d;
        }
        else {
            line += "()";
        }
    }
    cursorDelta = d;
    return line;
}

QString CodeItem::fullDescr() {
    QString s = _decl +" "+ _ident +":"+ _identType;
    if(_isFunc) {
        s += "(";
        int k = 0;
        foreach (CodeItem *item, children()) {
            if(item->_isParam) {
                if(k > 0)
                    s += ",";
                s += item->descrAsItem();
                ++k;
            }
        }
        s += ")";
    }
    return s;
}

QString CodeItem::identTypeCleared() {
    return clearType(_identType);
}

QString CodeItem::clearType(QString &s) {
    int i = s.indexOf("<");
    if(i > 0)
        return s.left(i);
    i = s.indexOf("[");
    if(i > 0)
        return s.left(i);
    return s;
}
