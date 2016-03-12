/*
Jentos IDE.

Copyright 2014, Evgeniy Goroshkin.

See LICENSE.TXT for licensing terms.
*/

#include "codeanalyzer.h"
#include "listwidgetcomplete.h"
#include "codeeditor.h"
#include "prefs.h"

QHash<QString, ImportedFiles*> CodeAnalyzer::_imports;
QStringList CodeAnalyzer::_userFiles;
QStringList CodeAnalyzer::_storedUserFiles;
QString CodeAnalyzer::_curFilePath;
QString CodeAnalyzer::_storedFilePath;
bool CodeAnalyzer::_isShowVariables;
bool CodeAnalyzer::_isShowInherited;
bool CodeAnalyzer::_isSortByName;
bool CodeAnalyzer::_disabled;
bool CodeAnalyzer::_isFillAucompWithInheritance;
//
bool CodeAnalyzer::_tabUseSpaces;
QString CodeAnalyzer::_tab;
QString CodeAnalyzer::_tabAsSpaces;
QString CodeAnalyzer::_tabAsTab;
int CodeAnalyzer::_tabSize;
bool CodeAnalyzer::_doAutoformat;
//
QTreeView* CodeAnalyzer::_treeView;
QListView* CodeAnalyzer::_listView;
//
QHash<QString,CodeItem*> CodeAnalyzer::_codeItemLinks;
QHash<QString,QStandardItem*> CodeAnalyzer::_standardItemLinks;
QList<CodeItem*> CodeAnalyzer::_listUserItems;
QList<CodeItem*> CodeAnalyzer::_listForRefreshIdentTypes;
QList<int> CodeAnalyzer::_listFoldTypes;
//
QHash<QString, QTextDocument *> CodeAnalyzer::_docs;


QHash<QTreeWidgetItem*,UsagesResult*> UsagesResult::usages;


CodeAnalyzer::CodeAnalyzer(QObject *parent) :
    QObject(parent)
{
}

void CodeAnalyzer::init() {
    _disabled = false;
    _treeView = 0;
    _listView = 0;
    Prefs *p = Prefs::prefs();
    //set defaults
    if(!p->contains("codeShowVariables"))
        p->setValue("codeShowVariables",true);
    if(!p->contains("codeShowInherited"))
        p->setValue("codeShowInherited",false);
    if(!p->contains("codeSort"))
        p->setValue("codeSort",true);
    if(!p->contains("codeAutoformat"))
        p->setValue("codeAutoformat",true);
    if(!p->contains("codeFillAucompInherit"))
        p->setValue("codeFillAucompInherit",true);
    if(!p->contains("tabSize"))
        p->setValue("tabSize",4);
    if(!p->contains("codeTabUseSpaces"))
        p->setValue("codeTabUseSpaces",false);
    //read current
    _isShowVariables = p->getBool("codeShowVariables");
    _isShowInherited = p->getBool("codeShowInherited");
    _isSortByName = p->getBool("codeSort");
    _doAutoformat = p->getBool("codeAutoformat");
    _isFillAucompWithInheritance = p->getBool("codeFillAucompInherit");
    _tabSize = p->getInt("tabSize");
    _tabUseSpaces = p->getBool("codeTabUseSpaces");
    //_tabUseSpaces = true;
    setTabSize(_tabSize, _tabUseSpaces);
    //_isFillAucompWithInheritance = false;
}

void CodeAnalyzer::setTabSize(int size, bool useSpaces) {
    _tabUseSpaces = useSpaces;
    _tabSize = size;
    _tabAsSpaces = " ";
    _tabAsSpaces = _tabAsSpaces.repeated(size);
    _tabAsTab = "	";
    _tab = (_tabUseSpaces ? _tabAsSpaces : _tabAsTab);
    //qDebug()<<"tab:"<<_tab;
}

CodeAnalyzer* CodeAnalyzer::instance() {
    static CodeAnalyzer *_instance = 0;
    if(_instance == 0)
        _instance = new CodeAnalyzer;
    return _instance;
}

void CodeAnalyzer::onPrefsChanged( const QString &name ) {
    if(name != "tabSize" && !name.startsWith("code"))
        return;
    Prefs *p = Prefs::prefs();
    if(name == "codeShowVariables") {
        _isShowVariables = p->getBool(name);
    }
    else if(name == "codeShowInherited") {
        _isShowInherited = p->getBool(name);
    }
    else if(name == "codeSort") {
        _isSortByName = p->getBool(name);
    }
    else if(name == "codeAutoformat") {
        _doAutoformat = p->getBool(name);
    }
    else if(name == "tabSize") {
        _tabSize = p->getInt(name);
        setTabSize(_tabSize, _tabUseSpaces);
    }
    else if(name == "codeTabUseSpaces") {
        bool b = p->getBool(name);
        setTabSize(_tabSize, b);
    }
    else if(name == "codeFillAucompInherit") {
        _isFillAucompWithInheritance = p->getBool("codeFillAucompInherit");
    }
    //qDebug()<<"CodeAnalyzer::onPrefsChanged"<<name;
}

void CodeAnalyzer::finalize() {
    foreach (FileInfo *i, userFilesModified()->values()) {
        delete i;
    }
    userFilesModified()->clear();
    delete userFilesModified();
    //
    mapTemplates()->clear();
    delete mapTemplates();
    //
    foreach (CodeItem *i, mapMonkey()->values()) {
        delete i;
    }
    mapMonkey()->clear();
    delete mapMonkey();
    //
    foreach (CodeItem *i, mapUser()->values()) {
        delete i;
    }
    mapUser()->clear();
    delete mapUser();
    //
    foreach (CodeItem *i, mapKeywords()->values()) {
        delete i;
    }
    mapKeywords()->clear();
    delete mapKeywords();
    //
    foreach (CodeItem *i, mapRem()->values()) {
        delete i;
    }
    mapRem()->clear();
    delete mapRem();
    //
    foreach (CodeItem *i, mapFolds()->values()) {
        delete i;
    }
    mapFolds()->clear();
    delete mapFolds();
    //
    UsagesResult::clear();

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
    return mapTemplates()->value( key.toLower() );
}

QString CodeAnalyzer::keyword( const QString &key ) {
    CodeItem *item = mapKeywords()->value(key.toLower(), 0);
    if(item)
        return item->ident();
    return "";
}

void CodeAnalyzer::extractIdents(const QString &text, QStringList &list) {
    /*QString s = "";
    int i = 0, n = text.length();
    while( i < n ) {
        QChar c = text[i++];
        bool idnt = isIdent(c);
        if(idnt) {
            s += c;
        }
        else {
            if(s != "" && isAlpha(s[0]))
                list.append(s);
            s = "";
        }
    }
    if(s != "" && isAlpha(s[0]))
        list.append(s);
    qDebug()<<"extractIdents";
    foreach (s, list) {
        qDebug()<<"idnt:"<<s;
    }*/
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
                //qDebug()<<"add template: "+s+" > "+s2;
            }
            else {
                i2 = i1;
            }
        }
    }
}

void CodeAnalyzer::loadKeywords( const QString &path ) {
    //qDebug() << "loadKeywords";
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
            "Print;Error;Alias;Rem";
    }
    QStringList lines = text.split(';');
    QTextBlock block;
    foreach(QString s, lines){
        //qDebug() << "kw: "+s;
        CodeItem *item = new CodeItem("keyword",s,0,block,"");
        mapKeywords()->insert(item->ident().toLower(), item);
    }
}

QHash<QString, QString> *CodeAnalyzer::mapTemplates() {
    static QHash<QString,QString> *m = 0;
    if( !m )
        m = new QHash<QString,QString>;
    return m;
}

QMap<QString, CodeItem *>* CodeAnalyzer::mapMonkey() {
    static QMap<QString,CodeItem*> *m = 0;
    if( !m )
        m = new QMap<QString,CodeItem*>;
    return m;
}

QHash<QString, CodeItem *>* CodeAnalyzer::mapUser() {
    static QHash<QString,CodeItem*> *m = 0;
    if( !m )
        m = new QHash<QString,CodeItem*>;
    return m;
}

QHash<QString, CodeItem *>* CodeAnalyzer::mapKeywords() {
    static QHash<QString,CodeItem*> *m = 0;
    if( !m )
        m = new QHash<QString,CodeItem*>;
    return m;
}

QHash<QString, CodeItem *>* CodeAnalyzer::mapRem() {
    static QHash<QString,CodeItem*> *m = 0;
    if( !m )
        m = new QHash<QString,CodeItem*>;
    return m;
}

QHash<QString, CodeItem *> *CodeAnalyzer::mapFolds() {
    static QHash<QString,CodeItem*> *m = 0;
    if( !m )
        m = new QHash<QString,CodeItem*>;
    return m;
}

QStandardItemModel* CodeAnalyzer::treeItemModel() {
    static QStandardItemModel* m = 0;
    if( !m )
        m = new QStandardItemModel;
    return m;
}

QStandardItemModel* CodeAnalyzer::listItemModel() {
    static QStandardItemModel* m = 0;
    if( !m )
        m = new QStandardItemModel;
    return m;
}


QString CodeAnalyzer::fullStandardItemPath(QStandardItem *si) {
    QString s = si->text();
    while(si->parent()) {
        si = si->parent();
        s = si->text()+"$"+s;
    }
    return s;
}

void CodeAnalyzer::linkCodeItemWithStandardItem(CodeItem *i, QStandardItem *si) {
    QString s = fullStandardItemPath(si);
    //qDebug()<<"linkItemWithData:"<<s;
    _codeItemLinks.insert(s,i);
    _standardItemLinks.insert(s,si);
}

CodeItem* CodeAnalyzer::getCodeItemFromStandardItem(QStandardItem *si) {
    if(!si)
        return 0;
    QString s = fullStandardItemPath(si);
    //qDebug()<<"getItemFromData:"<<s;
    return _codeItemLinks.value(s,0);
}

QStandardItem* CodeAnalyzer::getStandardItem(QString path) {
    return _standardItemLinks.value(path,0);
}

void CodeAnalyzer::fillListInheritance(const QTextBlock &block, QListWidget *l) {
    CodeItem *c = scopeAt(block, true);
    if(!c || !c->hasBaseClasses())
        return;

    QList<CodeItem*> list, list2, listRes;
    list = c->children();
    foreach (CodeItem *i, list) {
        if(i->decl() == "method") {
            listRes.append(i);
            //qDebug()<<"append"<<i->descrAsItem();
        }
    }
    list.clear();
    allClasses("",false,true,list,c);
    //qDebug()<<"list.size:"<<list.size();
    foreach (CodeItem *i, list) {
        //qDebug()<<"base:"<<i->descrAsItem();
        list2 = i->children();
        foreach (CodeItem *i, list2) {
            if(i->decl() == "method" && !CodeItem::isListContains(listRes,i)) {
                QIcon icon = identIcon(i->decl());
                ListWidgetCompleteItem *lwi = new ListWidgetCompleteItem(icon, i->descrAsItem(), i, l);
                lwi->setToolTip( i->toString() );
                l->addItem( lwi );
                //qDebug()<<"to list:"<<i->descrAsItem();
            }
        }

    }
}

CodeItem *CodeAnalyzer::findInScope(const QTextBlock &block, int pos, QListWidget *l, bool findLastIdent, const QString &blockText) {
    //qDebug()<<"cur:"<<_curFilePath;
    QStringList list;
    QString text = blockText;
    if(text == "")
        text = block.text();
    identsList(text, pos, list);
    if(list.isEmpty()) {
        //qDebug()<<"list is empty. try to fill common";
        if(l)
            fillListFromCommon(l, "", block);
        return 0;
    }
    QString ident = list.first();
    //qDebug()<<"ident:"<<ident;
    if(ident.isEmpty()) {//is empty if there is a number before a dot
        //qDebug()<<"dot before number, retirn";
        return 0;
    }
    bool isself = (ident.toLower()=="self");
    bool issuper = (ident.toLower()=="super");
    if(!isself && !issuper && containsKeyword(ident)) {
        //qDebug()<<"ident is keyword: "+ident;
        return 0;
    }
    //qDebug()<<"list.size:"<<list.size()<<"first:"<<ident;
    bool classOnly = (isself || issuper);
    CodeItem *scope = scopeAt(block, classOnly, true);
    CodeItem *item=0;
    if(classOnly) {
        item = scope;
        //qDebug()<<"item = scope; (classonly)";
    }
    else if(scope) {
        //qDebug()<<"inscope: "+scope->descrAsItem();
        item = scope->child(ident);
        if(item) {
            //qDebug()<<"found 1:"<<item->descrAsItem();
            //return item;
        }
        else {
            while(scope->parent() != 0) {
                scope = scope->parent();
                //qDebug()<<"inscope 2:"<<scope->descrAsItem();
                if(scope->isClassOrInterface()) {
                    QList<CodeItem*> classes;
                    allClasses("",true,true,classes,scope);
                    foreach (CodeItem *c, classes) {
                        item = c->child(ident);
                        if(item) {
                            //qDebug()<<"found 2: "+item->descrAsItem()+", class: "+c->descrAsItem();
                            break;
                        }
                    }
                }
                else {
                    item = scope->child(ident);
                }
                if(item)
                    break;
            }
        }
    }
    /*else {
        qDebug()<<"scope not found for: "+ident;
    }*/
    if(!item)
        item = itemUser(ident);
    if(!item)
        item = itemMonkey(ident);
    if(!item) {
        //qDebug() << "ident not found: "+ident+". try to fill common";
        if(l && list.size() == 1)
            fillListFromCommon(l, ident, block);
        return 0;
    }
    //qDebug()<<"found2: "+item->descrAsItem();
    //search sub-items if exists
    CodeScope sc;
    sc.item = item;
    //CodeItem *res = item;
    for( int k = 1, n = list.size()-1; k < n; ++k) {
        QString type = item->identTypeCleared();

        CodeItem *c = itemUser(type);
        if(!c)
            c = itemMonkey(type);
        if(c) {
            item = c->child(list.at(k),true);
            if(item) {
                //qDebug()<<"found sub-item: "+list.at(k);
                sc.item = item;
                sc.isSelf = (list.at(k-1).toLower()=="self");
                sc.isSuper = (list.at(k-1).toLower()=="super");
            }
            else {
                break;
            }
        }
    }
    if(sc.item) {
        //qDebug()<<"scope: "+sc.item->descrAsItem();
        if(l) {
            fillListFromScope(l,list.last(),sc);
        }
        if(findLastIdent && list.size() > 1) {
            ident = list.last();
            if(!ident.isEmpty()) {
                //qDebug()<<"search:"<<ident<<"in"<<sc.item->ident();
                QString type = sc.item->identTypeCleared();
                CodeItem *c = CodeAnalyzer::itemUser(type);
                if(!c)
                    c = CodeAnalyzer::itemMonkey(type);
                sc.item = (c ? c->child(ident, true) : 0);
            }
        }
    }
    return sc.item;
}

void CodeAnalyzer::identsList(const QString &text, int cursorPos, QStringList &list) {
    if(text.trimmed().isEmpty())
        return;
    int i = cursorPos;
    int end = i;
    QString s = "";
    bool skip = false;
    while( (--i) >= 0 ) {
        QChar c = text[i];
        if(!skip) {
            bool idnt = isIdent(c);
            bool b = (c == '.' || c == '[' || c == ']' || idnt);
            if( !b ) {
                ++i;
                break;
            }
            if(idnt) {
                s = c+s;
            }
            else if(c == ']'){
                skip = true;
            }
            else if( c == '.' && s != "") {
                if(!isAlpha(s[0])) {
                    list.insert(0, "");
                    return;
                }
                list.insert(0, s);
                //qDebug() << "insert1: "+s;
                s = "";
            }
        }
        else if(c == '[') {
            skip = false;
        }
    }
    if( s != "" ) {
        if(!isAlpha(s[0])) {
            list.insert(0, "");
            return;
        }
        list.insert(0, s);
        //qDebug() << "insert2: "+s;

    }
    if(text[text.size()-1] == '.') {
        list.append("");
        //qDebug() << "append: empty char";
    }
    /*qDebug()<<"findidents: "+text.mid(i, end-i);
    foreach (s, list) {
        qDebug() << "idnt: "+s;
    }*/
}

CodeItem* CodeAnalyzer::scopeAt(const QTextBlock &block, bool classOnly, bool checkCurFile) {
    //qDebug()<<"scopeAt._curFilePath:"<<_curFilePath;
    QList<CodeItem*> list = mapUser()->values();
    CodeItem *item, *res=0;
    //qDebug()<<"1.1";
    int num = block.blockNumber();
    //qDebug()<<"1.2";
    foreach (item, list) {
        //qDebug()<<"scopeAt.item:"<<item->descrAsItem()<<item->filepath();
        /*if(item->block().document()->)
            qDebug()<<"scopeAt.has document";
        else
            qDebug()<<"scopeAt.has no document";*/
        //qDebug()<<"scopeAt.block.number:"<<item->blockNumber();
        if(checkCurFile && item->filepath() != _curFilePath)
            continue;
        bool eq = (num == item->blockNumber());
        if( (eq || num > item->blockNumber() && num < item->blockEndNumber()) && item->isMultiLine() ) {
            res = item;
            if(eq) {
                break;
            }
            if(classOnly && item->isClassOrInterface()) {
                break;
            }
            //search sub-containers: func, if, for...
            bool search = true;
            while(search && !item->children().isEmpty()) {
                search = false;
                QList<CodeItem*> list2 = item->children();
                foreach (item, list2) {
                    if(item->isMultiLine()) {//is container
                        eq = (num == item->blockNumber());
                        if( (eq || num > item->blockNumber() && num < item->blockEndNumber()) ) {
                            res = item;
                            search = !eq;
                            break;
                        }
                    }
                }
                if(eq) {
                    //qDebug()<<"eq";
                    break;
                }
            }
            break;
        }
    }
    //qDebug()<<"1.3";
    return res;
}

CodeItem* CodeAnalyzer::remAt(const QTextBlock &block) {
    QList<CodeItem*> list = mapRem()->values();
    CodeItem *item = 0;
    int num = block.blockNumber();
    foreach (item, list) {
        if(item->filepath() == _curFilePath) {
            //qDebug()<<_curFilePath<<block.text();
            if(num >= item->blockNumber() && num <= item->blockEndNumber()) {
                return item;
            }
        }
    }
    return 0;
}

CodeItem* CodeAnalyzer::foldAt(const QTextBlock &block) {
    QList<CodeItem*> list = mapFolds()->values();
    CodeItem *item = 0;
    int num = block.blockNumber();
    foreach (item, list) {
        if(item->filepath() == _curFilePath && num >= item->blockNumber() && num <= item->blockEndNumber()) {
            return item;
        }
    }
    return 0;
}

CodeItem* CodeAnalyzer::isBlockHasClassOrFunc(const QTextBlock &block) {
    BlockData *d = BlockData::data(block);
    if(d) {
        foreach (CodeItem *i, d->items()) {
            if(i->isClassOrInterface() || i->isFunc()) {
                return i;
            }
        }
    }
    return 0;
}
//Modify this code to stop the autobracing
QStringList CodeAnalyzer::extractParams(const QString &text) {
    QStringList list;
    int st1=0, st2=0, st3=0, pos = 0;
    bool quo = false;
    QChar ch;
    QString s;
    int len = text.length();
    for(int k = 0; k < len; ++k) {
        ch = text.at(k);
        if(ch == '\"') {
            quo = !quo;
        }
        if(quo)
            continue;
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
        else if(ch == '(')
            ++st2;
        else if(ch == ')')
            --st2;
        else if(ch == '<')
            ++st3;
        else if(ch == '>')
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

bool CodeAnalyzer::analyzeFile( const QString &path, int kind ) {
    QFile file(path);
    if( file.open( QIODevice::ReadOnly ) ) {
        //qDebug()<<"parse file:"<<path;
        QTextStream stream( &file );
        stream.setCodec( "UTF-8" );
        QString text = stream.readAll();
        file.close();
        QTextDocument *doc = new QTextDocument(text);
        bool b = parse(doc, path, kind);
        if(kind == KIND_MONKEY)
            delete doc;
        return b;
    }
    return false;
}

/*bool CodeAnalyzer::analyzeScope( const QTextBlock &path, int kind ) {
    QFile file(path);
    if( file.open( QIODevice::ReadOnly ) ) {
        //qDebug()<<"parse file:"<<path;
        QTextStream stream( &file );
        stream.setCodec( "UTF-8" );
        QString text = stream.readAll();
        file.close();
        QTextDocument *doc = new QTextDocument(text);
        bool b = parse(doc, path, kind);
        if(kind == KIND_MONKEY)
            delete doc;
        return b;
    }
    return false;
}*/


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
        //qDebug()<<"USER:"<<codeItem->descrAsItem();
        _listUserItems.append(codeItem);
    }
}

void CodeAnalyzer::remove(CodeItem *item) {
    if(item) {
        mapUser()->remove(item->ident());
    }
}

void CodeAnalyzer::removeByPath(const QString &path) {
    QList<CodeItem*> list = _listUserItems;//mapUser()->values();
    foreach (CodeItem *i, list) {
        if(i->filepath() == path) {
            //qDebug()<<"remove:"<<i->descrAsItem();
            mapUser()->remove(i->ident());
            _listUserItems.removeOne(i);
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
    QStringList listForRemove, listInUse;
    ImportedFiles *f = _imports.value(path);
    if(f) {
        listForRemove << f->files;
    }
    listInUse << _userFiles;
    listInUse.removeOne(path);
    foreach (QString key, _imports.keys()) {
        if(key == path)
            continue;
        ImportedFiles *f = _imports.value(key);
        if(f) {
            listInUse << f->files;
        }
    }
    foreach (QString s, listForRemove) {
        if(!listInUse.contains(s)) {
            removeByPath(s);
            userFilesModified()->remove(s);
            _userFiles.removeOne(s);
            _docs.remove(s);
        }
    }
    if(!_userFiles.isEmpty()) {
        removeByPath(path);
        userFilesModified()->remove(path);
        _userFiles.removeOne(path);
        _docs.remove(path);
        if(listInUse.contains(path)) {
            analyzeFile(path, KIND_USER);
        }
        else {
            _imports.remove(path);
        }
    }
}

void CodeAnalyzer::begin() {
    //_storedUserFiles << _userFiles;
}

void CodeAnalyzer::end() {
    /*foreach (QString s, _storedUserFiles) {
        if(!_userFiles.)
    }*/
}

bool CodeAnalyzer::parse(QTextDocument *doc, const QString &path , int kind, QList<QTextBlock> *blocks) {

    if(_disabled) {
        return false;
    }


    if(!blocks && kind == KIND_USER) {

        FileInfo *fi = userFilesModified()->value(path,0);
        if(fi) {
            if(!fi->isModified()) {
                //qDebug()<<"not modified:"<<path;
                return false;
            }
            //remove previous items
            removeByPath(path);
        }
        else {
            fi = new FileInfo(path);
            userFilesModified()->insert(path,fi);
            _userFiles.append(path);
        }
        //
        _docs.insert(path, doc);

    }

    QString dir = extractDir(path)+"/";

    CodeItem *container=0, *remBlock = 0, *foldBlock = 0;
    bool isPrivateInFile = false;
    bool isPrivateInClass = false;
    QStack<CodeItem*> stack;

    QTextBlock block = (blocks ? blocks->at(0) : doc->firstBlock());
    QTextBlock block2 = (blocks ? blocks->at(1) : doc->lastBlock());

    while(block.isValid()) {

        if(block.previous() == block2) {
            break;
        }

        if(kind == KIND_USER) {
            BlockData::flush(block);
            BlockData *d = BlockData::data(block,true);
            d->foldType |= ( container||foldBlock ? 1 : 0 );
        }

        QString line = trimmedRight( block.text() );
        int len = line.length();
        int indent = 0;
        while( indent < len && line.at(indent) <= ' ' )
            ++indent;
        if(indent > 0)
            line = line.mid(indent);

        //imports
        if(kind == KIND_USER && line.startsWith("Import ")) {
            QString s = dir + line.mid(7).replace(".","/") + ".monkey";
            bool b = analyzeFile(s, KIND_USER);
            if(b) {
                ImportedFiles *f = _imports.value(path);
                if( !f ) {
                    f = new ImportedFiles;
                    _imports.insert(path, f);
                }
                f->append(s);
            }
        }

        //comments
        if(line.startsWith("'")) {
            if(kind == KIND_USER) {
                QString s1 = "'/**", s2 = "'**/";
                if(line.startsWith(s1)) {
                    CodeItem *item = new CodeItem("","fold"+QString::number(block.blockNumber())+path,0,block,path,0);
                    item->setFoldable(true);
                    foldBlock = item;
                    if(kind == KIND_USER) {
                        BlockData *d = BlockData::data(block,true);
                        d->foldType |= 1;
                    }
                }
                else if(foldBlock && line.startsWith(s2)) {
                    foldBlock->setBlockEnd(block);
                    //_listFoldTypes.replace(_listFoldTypes.size()-1,2);
                    BlockData *d = BlockData::data(block,true);
                    d->foldType |= 2;
                    if(!container)
                        d->foldType = 2;
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
            //line = lower;

            if(lower.startsWith("#rem")) {
                if(kind == KIND_USER) {
                    CodeItem *item = new CodeItem("","rem"+QString::number(block.blockNumber())+path,0,block,path,0);
                    item->setFoldable(true);
                    remBlock = item;
                    mapRem()->insert(item->ident(),item);
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
                                //_listFoldTypes.replace(_listFoldTypes.size()-1,2);
                                if(kind == KIND_USER) {
                                    BlockData *d = BlockData::data(block,true);
                                    d->foldType |= 2;
                                    if(container || foldBlock)
                                        d->foldType |= 1;
                                }
                                remBlock = 0;
                            }
                            break;
                        }
                        else {
                            --st;
                        }
                    }
                    if(kind == KIND_USER) {
                        BlockData *d = BlockData::data(block,true);
                        d->foldType |= 1;
                    }
                    //_listFoldTypes.append( remBlock ? 1 : 0 );
                    block = block.next();
                }
                block = block.next();
                continue;
            }
            //
            if(container && (lower == "end" || lower.startsWith("end ") || lower == "endif" || lower == "next" || lower == "wend")) {
                while(container && container->indent() > indent) {
                    container = (stack.isEmpty() ? 0 : stack.pop());
                }
                if(container && indent <= container->indent()) {
                //if(indent == container->indent()) {
                    container->setFoldable(true);
                    container->setBlockEnd(block);
                    if(container->isClassOrInterface())
                        isPrivateInClass = false;
                    container = (stack.isEmpty() ? 0 : stack.pop());
                    if(kind == KIND_USER) {
                        //_listFoldTypes.replace(_listFoldTypes.size()-1,2);
                        BlockData *d = BlockData::data(block,true);
                        d->foldType |= 2;
                        if(!container && !foldBlock)
                            d->foldType = 2;
                    }
                    block = block.next();
                    continue;
                }
            }
            else if(lower == "private") {
                if(container && container->isClassOrInterface())
                    isPrivateInClass = true;
                else
                    isPrivateInFile = true;
                block = block.next();
                continue;
            }
            else if(lower == "public") {
                if(container && container->isClassOrInterface())
                    isPrivateInClass = false;
                else
                    isPrivateInFile = false;
                block = block.next();
                continue;
            }
        }
        if(isPrivateInFile /*|| isPrivateInClass*/) {
            block = block.next();
            continue;
        }
        int rem = line.indexOf("'");
        if(rem)
            line = line.left(rem);
        QString line0 = line;
        //qDebug()<<"line0:"<<line0;
        QString decl = line.left(i).toLower();
        line = line.mid(i+1).trimmed();

        if(kind == KIND_MONKEY) {
            autoFormat(line);
        }
        if(decl == "class" || decl == "interface") {
            CodeItem *item = new CodeItem(decl, line, indent, block, path, 0);
            item->setKind(kind);
            item->setFoldable(true);
            insertItem(item->ident(), item, kind);
            if(container) {
                QTextBlock b = block.previous();
                container->setBlockEnd(b);
                if(kind == KIND_USER) {
                    BlockData *d = BlockData::data(b,true);
                    d->foldType |= 2;
                    if(!foldBlock)
                        d->foldType = 2;
                }
            }
            container = item;
            if(kind == KIND_USER) {
                BlockData *d = BlockData::data(block,true);
                d->foldType |= 1;
            }
        }
        else if(decl == "field" || decl == "global" || decl == "const") {
            if(container && container->indent() >= indent) {
                QTextBlock b = block.previous();
                container->setBlockEnd(b);
                if(b != container->block())
                    container->setFoldable(true);
                if(kind == KIND_USER) {
                    BlockData *d = BlockData::data(b,true);
                    d->foldType |= 2;
                }
                container = (stack.isEmpty() ? 0 : stack.pop());
            }
            QStringList l = extractParams(line);
            foreach (QString s, l) {
                s = s.trimmed();
                CodeItem *item = new CodeItem(decl, s, indent, block, path, 0);
                item->setKind(kind);
                if (isPrivateInClass)
                    item->markAsPrivate();
                if(container) {
                    container->addChild(item);
                }
                else {
                    insertItem(item->ident(), item, kind);
                }
            }
        }
        else if(decl == "method" || decl == "function") {

            if (container && container->indent() >= indent) {
                QTextBlock b = block.previous();
                container->setBlockEnd(b);
                if (b != container->block())
                    container->setFoldable(true);
                if (kind == KIND_USER) {
                    BlockData *d = BlockData::data(b,true);
                    d->foldType |= 2;
                }
                container = (stack.isEmpty() ? 0 : stack.pop());
            }
            /*if(container && container->isFunc()) {
                container->setBlockEnd(block);
                container->setFoldable(false);
                container = (stack.isEmpty() ? 0 : stack.pop());
            }*/

            bool isProp = line.endsWith("Property");

            bool singleLine = false;
            int ii = line.lastIndexOf(')');
            if(!isProp && ii && ii != line.length()-1) {
                line = line.left(ii+1);
                singleLine = true;
            }

            CodeItem *item = new CodeItem(decl, line, indent, block, path, 0);
            item->setKind(kind);
            if (isProp)
                item->markAsProperty();

            if (container) {

                bool needAdd = true;
                if (isProp) {
                    CodeItem *i = container->child( item->ident() );
                    needAdd = (i == 0);
                }

                if (needAdd)
                    container->addChild(item);
                //else
                //    qDebug() << "already added:"<<item->ident();
            } else {
                insertItem(item->ident(), item, kind);
            }
            if (container && container->isInterface())
                singleLine = true;
            else if (line.endsWith(" Abstract"))
                singleLine = true;
            if (!singleLine) {
                if (container)
                    stack.push(container);
                container = item;
            }
            if (kind == KIND_USER) {
                BlockData *d = BlockData::data(block,true);
                d->foldType |= 1;
            }

        }
        else if(container && decl == "local") {
            QStringList l = extractParams(line);
            foreach (QString s, l) {
                s = s.trimmed();
                CodeItem *item = new CodeItem(decl, s, indent, block, path, 0);
                item->setKind(kind);
                container->addChild(item);
            }
        }
        else if(container) {

            bool bif = (line0.startsWith("If") && !line0.contains("Then "));
            bool bfor = line0.startsWith("For ");
            bool bwhile = line0.startsWith("While");
            bool bselect = line0.startsWith("Select");

            if(bif) {
                QTextBlock b = block.next();
                if(b.isValid()) {
                    int indent2 = 0;
                    QString s = b.text();
                    int len = s.length();
                    while( indent2 < len && s.at(indent2) <= ' ' )
                        ++indent2;
                    if(indent2 <= indent) {//next line outside of 'if-end'
                        bif = false;
                    }
                }
            }

            if(bif || bfor || bwhile || bselect) {
                QString decl="";
                if(bif) {
                    decl = "if";
                }
                else if(bfor) {
                    decl = "for";
                }
                else if(bwhile) {
                    decl = "while";
                }
                else if(bselect) {
                    decl = "select";
                }
                CodeItem *item = new CodeItem(decl, line0, indent, block, path, 0);
                item->setKind(kind);
                item->setFoldable(true);
                container->addChild(item);
                stack.push(container);
                container = item;
                if(bfor && line.startsWith("Local")) {
                    line = line.mid(6);
                    //qDebug()<<"line:"<<line;
                    item = new CodeItem("local", line, indent, block, path, 0);
                    container->addChild(item);
                }
                //qDebug()<<"if-for-while container";
            }
        }
        block = block.next();
    }
    /*if(kind == KIND_USER && !files.isEmpty()) {
        //files.size() > _userFiles.size()
        //qDebug()<<"sizes:"<<files.size();//<<_userFiles.size();
        ImportedFile *f = _imports.value(path);
        foreach (QString s, files) {
            if(!f || !f->files.contains(s)) {
                qDebug()<<"remove:"<<s;
                removeByPath(s);
            }
        }
    }*/
    refreshIdentTypes();
    /*if(kind == KIND_USER) {
        qDebug()<<"parse4:"<<path;
    }*/
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

CodeItem* CodeAnalyzer::itemKeyword(const QString &ident ) {
    return mapKeywords()->value(ident.toLower(), 0);
}

CodeItem* CodeAnalyzer::itemMonkey(const QString &ident ) {
    return mapMonkey()->value(ident, 0);
}

CodeItem* CodeAnalyzer::itemUser(const QString &ident, bool withChildren ) {
    QList<CodeItem*> items = mapUser()->values();
    foreach(CodeItem *i, items){
        if(i->ident() == ident /*&& i->filepath() == _curFilePath*/) {
            return i;
        }
        if(withChildren && i->hasChildren()) {
            QList<CodeItem*> items2 = i->children();
            foreach(CodeItem *i2, items2){
                if(i2->ident() == ident /*&& i->filepath() == _curFilePath*/)
                    return i2;
            }
        }
    }
    return 0;
}

void CodeAnalyzer::fillListFromCommon( QListWidget *l, const QString &ident, const QTextBlock &block ) {

    CodeItem *item;
    ListWidgetCompleteItem *lwi;
    QIcon icon;
    QList<CodeItem*> list = mapKeywords()->values();

    QList<CodeItem*> listRes;

    foreach (item, list) {
        if( ident == "" || item->ident().startsWith(ident, Qt::CaseInsensitive) ) {
            //CodeItem::addUnique(listRes,item);
            listRes.append(item);
        }
    }

    CodeItem *scope = scopeAt(block);
    if(scope) {
        list = scope->children();
        foreach (item, list) {
            if( ident == "" || item->ident().startsWith(ident, Qt::CaseInsensitive) ) {
                //CodeItem::addUnique(listRes,item);
                listRes.append(item);
            }
        }
        if(scope->parent()) {
            QList<CodeItem*> classes;
            allClasses("",true,true,classes,scope->parent());
            foreach (CodeItem *c, classes) {
                list = c->children();
                foreach (item, list) {
                    if( ident == "" || item->ident().startsWith(ident, Qt::CaseInsensitive) ) {
                        //CodeItem::addUnique(listRes,item);
                        CodeItem *same = CodeItem::findTheSame(listRes,item);
                        if(same)
                            same->setIsInherited();
                        else
                            listRes.append(item);
                    }
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
        tryToAddItemToList(item, scope, l);
    }

}

void CodeAnalyzer::tryToAddItemToList(CodeItem *item, CodeItem *scopeItem, QListWidget *list) {
    if (item->isPrivate()) {
        bool ok = checkScopeForPrivate(item, scopeItem);
        if (!ok)
            return;
    }
    QString descr = item->descrAsItem();

    QStringList templField = scopeItem->templWords();
    bool hasTempl = !templField.isEmpty();
    // replacement for template <T> values, i.e. list.Add(value:T) -> list.Add(value:MyClass)
    if (hasTempl) {
        QStringList templClass;
        CodeItem *par = item->parent();
        if (par != 0) {
            templClass = par->templWords();
            hasTempl = (!templField.isEmpty() && templClass.size() == templField.size());
            if (hasTempl) {
                for (int k = 0, size = templField.size(); k < size; ++k) {
                    descr = descr.replace(":"+templClass.at(k), ":"+templField.at(k));
                }
            }
        }
    }
    QIcon icon = identIcon(item->decl());
    ListWidgetCompleteItem *lwi = new ListWidgetCompleteItem(icon, descr, item, list);
    lwi->setToolTip( item->toString() );
    list->addItem( lwi );
}

bool CodeAnalyzer::checkScopeForPrivate(CodeItem *item, CodeItem *scopeItem) {
    CodeItem *class1 = item->parentClass();
    CodeItem *class2 = scopeItem->parentClass();
    // private members are available only for the same class where they declared
    return (class1 == class2);
}

void CodeAnalyzer::fillListFromScope( QListWidget *l, const QString &ident, CodeScope scope ) {
    //qDebug() << "fillListByChildren: "+ident;
    CodeItem *item, *item0;
    ListWidgetCompleteItem *lwi;
    QIcon icon;
    item = item0 = scope.item;
    bool global = (item->isClassOrInterface() && !scope.isSelf && !scope.isSuper);
    QList<CodeItem*> list, listRes;
    list.clear();
    allClasses(item->identTypeCleared(), !scope.isSuper, true/*!scope.isSelf*/, list);
    QStringList templField = item->templWords();
    bool hasTempl = !templField.isEmpty();
    foreach (item, list) {
        //qDebug() << "base: "+ item->ident();
        QList<CodeItem*> listChildren = item->children();
        foreach (CodeItem *child, listChildren) {
            //qDebug() << "child: "+ child->ident();
            if(global && (child->decl() != "function" && child->decl() != "global" && child->decl() != "const")) {
                continue;
            }
            if( (ident == "" || child->ident().startsWith(ident,Qt::CaseInsensitive)) && child->ident() != "Self" ) {
                //CodeItem::addUnique(listRes, child);
                CodeItem *same = CodeItem::findTheSame(listRes,child);
                if(same)
                    same->setIsInherited();
                else
                    listRes.append(child);
            }
        }
    }
    //qDebug() << "work";
    foreach (CodeItem *child, listRes) {

        tryToAddItemToList(child, scope.item, l);

    }
}

void CodeAnalyzer::allClasses(QString identType, bool addSelf, bool addBase, QList<CodeItem*> &list, CodeItem *item) {
    CodeItem *i = item;
    if(!i)
        i = itemUser(identType);
    if(!i)
        i = itemMonkey(identType);
    if(!i) {
        return;
    }
    if(addSelf) {
        list.append(i);
    }
    if(addBase) {
        QStringList baseClasses = i->baseClasses();
        foreach (QString s, baseClasses) {
            allClasses(CodeItem::clearType(s), true, true, list);
        }
    }
}

void CodeAnalyzer::allClasses(QList<CodeItem*> &targetList) {
    QList<CodeItem*> items;
    items = mapUser()->values();
    foreach (CodeItem *i, items){
        if (i->isClassOrInterface()) {
            targetList.append(i);
        }
    }
    items = mapMonkey()->values();
    foreach (CodeItem *i, items){
        if (i->isClassOrInterface()) {
            targetList.append(i);
        }
    }
}


void CodeAnalyzer::fillTree(/*QStandardItemModel *im*/) {

    _codeItemLinks.clear();
    _standardItemLinks.clear();

//    return;

    QStandardItemModel *im = treeItemModel();
    im->clear();
    QStandardItem *root = im->invisibleRootItem();
    QStandardItem *parent;
    QList<CodeItem*> list = _listUserItems;//mapUser()->values();

    CodeItem *item, *childItem;
    foreach (QString file, _userFiles) {
        //qDebug()<<"user file:"<<file;
        parent = root;
        ItemWithData *treeItem = new ItemWithData;
        treeItem->setText(stripDir(file));
        treeItem->setToolTip(file);
        QFont f = treeItem->font();
        f.setBold(true);
        treeItem->setFont(f);
        treeItem->setCode(0);
        parent->appendRow(treeItem);
        parent = treeItem;
        foreach (item, list) {
            if(item->filepath() != file)
                continue;
            //qDebug()<<"tree:"<<item->descrAsItem();
            ItemWithData *treeItem = new ItemWithData(identIcon(item->decl()), item->descrAsItem());
            //treeItem->setCode(item);
            //item->setItemWithData(item->ident(), treeItem);
            parent->appendRow(treeItem);
            linkCodeItemWithStandardItem(item,treeItem);
            treeItem->setToolTip(item->toolTip());
            QList<CodeItem*> listForAdd;
            QString name = item->ident();
            if(item->hasChildren()) {
                QList<CodeItem*> children = item->children();
                foreach (childItem, children) {
                    if(!_isShowVariables && childItem->isVar())
                        continue;
                    //CodeItem::addUnique(listForAdd, childItem);
                    listForAdd.append(childItem);
                }
            }
            if(_isShowInherited) {
                QList<CodeItem*> classes;
                allClasses("",false,true,classes,item);
                foreach (item, classes) {
                    if(item->hasChildren()) {
                        QList<CodeItem*> children = item->children();
                        foreach (childItem, children) {
                            if(!_isShowVariables && childItem->isVar())
                                continue;
                            CodeItem *same = CodeItem::findTheSame(listForAdd, childItem);
                            if(same)
                                same->setIsInherited();
                            else
                                listForAdd.append(childItem);
                        }
                    }
                }
            }
            foreach (item, listForAdd) {
                //item = new CodeItem(childItem);
                ItemWithData *itch = new ItemWithData(identIcon(item->decl()), item->descrAsItem());
                //itch->setCode(item);
                treeItem->appendRow(itch);
                linkCodeItemWithStandardItem(item,itch);
                QString tip = ""+item->toolTip()+"";
                if(item->isInherited()) {
                    tip += "<br><br>  overridden item";
                }
                else {
                    CodeItem *p = item->parent();
                    if(p) {
                        QString n = p->ident();
                        if(n != name)
                            tip += "<br><br>  inherited from: "+n+" ("+p->decl()+")";
                        //item->setItemWithData(n, itch);
                        //linkCodeItemWithStandardItem(item,itch);
                    }
                }
                itch->setToolTip("<p style='white-space:pre'>"+tip+"</p>");
            }
        }
    }
    if(_isSortByName)
        im->sort(0);
    if(_treeView) {
        QList<QStandardItem*> list = im->findItems( stripDir(_curFilePath) );
        if(!list.isEmpty())
            _treeView->expand(list.first()->index());
    }
}

QStandardItem* CodeAnalyzer::itemInTree( const QModelIndex &index ) {
    return CodeAnalyzer::treeItemModel()->itemFromIndex( index );
}

ItemWithData* CodeAnalyzer::itemInList( const QModelIndex &index ) {
    return dynamic_cast<ItemWithData*>(CodeAnalyzer::listItemModel()->itemFromIndex( index ));
}

ItemWithData* CodeAnalyzer::itemInList( int row ) {
    return dynamic_cast<ItemWithData*>(CodeAnalyzer::listItemModel()->item(row));
}

QString CodeAnalyzer::toolTip(CodeItem *item) {
    if(!item)
        return "";
    QString s="";
    if(item->isKeyword()) {
        s = "(keyword) <b>"+item->ident()+"</b>";
        if(item->ident() == "Import")
            s += "<br><br>Click to open file in editor.";
    }
    else {
        s = "("+item->decl()+") <b>"+item->descrAsItem()+"</b><br>";
        if(item->parent() && item->parent()->isClassOrInterface())
            s += "<u>"+item->parent()->toolTip()+"</u>";
        s += "<br><i>Declared in:</i> "+item->module()+"<br><i>at line "+QString::number(item->blockNumber()+1)+"</i>";

    }
    return s;
}

bool CodeAnalyzer::autoFormat( QString &s, bool force ) {
    if(!force && !_doAutoformat) {
        return false;
    }
    //replace tabs at beginning of string with 4 spaces
    int i = 0;
    bool repl = false;
    int len = s.length();
    while( i < len && s[i] <= ' ' ) {
        /*if(s[i] == 9) {
            s2 += _tab;
            repl = true;
        }
        else {
            s2 += s[i];
        }*/
        ++i;
    }
    if(i > 0) {
        QString s2 = s.left(i), s3 = s2;
        if(_tabUseSpaces)
            s3.replace(_tabAsTab, _tabAsSpaces);
        else
            s3.replace(_tabAsSpaces, _tabAsTab);
        repl = (s3.length() != s2.length());
        s = s3+s.mid(i);
        //qDebug()<<"indent:"<<i<<s2<<s3<<repl;
    }
    QString trimmed = s.trimmed();
    if(trimmed.length() == 0)
        return repl;
    static QStringList *sFrom = 0, *sTo = 0;
    static int cnt = 0;

    QString snippetmojox;
    snippetmojox += "Import mojo\n";
    snippetmojox += "\n";
    snippetmojox += "Class {{Xname}} Extends App\n";
    snippetmojox += "\n";
    snippetmojox += "<---->Method OnCreate()\n";
    snippetmojox += "<----....>SetUpdateRate 30\n";
    snippetmojox += "<---->End\n";
    snippetmojox += "\n";
    snippetmojox += "<---->Method OnUpdate()\n";
    snippetmojox += "\n";
    snippetmojox += "<---->End\n";
    snippetmojox += "\n";
    snippetmojox += "<---->Method OnRender()\n";
    snippetmojox += "<----....>Cls\n";
    snippetmojox += "<----....>DrawText(\"Welcome Monkey\",302,240)\n";
    snippetmojox += "<---->End\n";
    snippetmojox += "\n";
    snippetmojox += "End\n";
    snippetmojox += "\n";
    snippetmojox += "Function Main()\n";
    snippetmojox += "<---->New {{Xname}}()\n";
    snippetmojox += "End\n";


    QString snippetmojoxstrict;
    snippetmojoxstrict += "Strict\n";
    snippetmojoxstrict += "\n";
    snippetmojoxstrict += "Import mojo\n";
    snippetmojoxstrict += "\n";
    snippetmojoxstrict += "Class {{Xname}} Extends App\n";
    snippetmojoxstrict += "\n";
    snippetmojoxstrict += "<---->Method OnCreate:Int()\n";
    snippetmojoxstrict += "<----....>SetUpdateRate 30\n";
    snippetmojoxstrict += "<----....>Return 0\n";
    snippetmojoxstrict += "<---->End\n";
    snippetmojoxstrict += "\n";
    snippetmojoxstrict += "<---->Method OnUpdate:Int()\n";
    snippetmojoxstrict += "\n";
    snippetmojoxstrict += "<----....>Return 0\n";
    snippetmojoxstrict += "<---->End\n";
    snippetmojoxstrict += "\n";
    snippetmojoxstrict += "<---->Method OnRender:Int()\n";
    snippetmojoxstrict += "<----....>Cls\n";
    snippetmojoxstrict += "<----....>DrawText(\"Welcome Monkey\",302,240)\n";
    snippetmojoxstrict += "<----....>Return 0\n";
    snippetmojoxstrict += "<---->End\n";
    snippetmojoxstrict += "\n";
    snippetmojoxstrict += "End\n";
    snippetmojoxstrict += "\n";
    snippetmojoxstrict += "Function Main:Int()\n";
    snippetmojoxstrict += "<---->New {{Xname}}()\n";
    snippetmojoxstrict += "<---->Return 0\n";
    snippetmojoxstrict += "End\n";


    QString snippetmethodname;
    snippetmethodname += "<---->Method {{Xname}}\n";
    snippetmethodname += "\n";
    snippetmethodname += "<---->End";

    QString snippetmethodnamestrictVoid;
    snippetmethodnamestrictVoid += "<---->Method {{Xname}}:Void()\n";
    snippetmethodnamestrictVoid += "\n";
    snippetmethodnamestrictVoid += "<---->End";

    QString snippetmethodnamestrictInt;
    snippetmethodnamestrictInt += "<---->Method {{Xname}}:Int()\n";
    snippetmethodnamestrictInt += "\n";
    snippetmethodnamestrictInt += "<----....>Return 0\n";
    snippetmethodnamestrictInt += "<---->End";

    QString snippetmethodnamestrictField;
    snippetmethodnamestrictField += "<---->Method {{Xname}}:Float()\n";
    snippetmethodnamestrictField += "\n";
    snippetmethodnamestrictField += "<----....>Return 0\n";
    snippetmethodnamestrictField += "<---->End";

    QString snippetmethodnamestrictString;
    snippetmethodnamestrictString += "<---->Method {{Xname}}:String()\n";
    snippetmethodnamestrictString += "\n";
    snippetmethodnamestrictString += "<----....>Return \"\"\n";
    snippetmethodnamestrictString += "<---->End";

    QString snippetmethodnamestrictBool;
    snippetmethodnamestrictBool += "<---->Method {{Xname}}:Bool()\n";
    snippetmethodnamestrictBool += "\n";
    snippetmethodnamestrictBool += "<----....>Return False\n";
    snippetmethodnamestrictBool += "<---->End";

    QString snippetclasename;
    snippetclasename += "Class {{Xname}}\n";
    snippetclasename += "\n";
    snippetclasename += "End\n";

    //key down
    QString snippetkeydowndireccional;
    snippetkeydowndireccional += "<----....>If KeyDown(KEY_LEFT)\n";
    snippetkeydowndireccional += "\n";
    snippetkeydowndireccional += "<----....>End\n";
    snippetkeydowndireccional += "\n";
    snippetkeydowndireccional += "<----....>If KeyDown(KEY_RIGHT)\n";
    snippetkeydowndireccional += "\n";
    snippetkeydowndireccional += "<----....>End\n";
    snippetkeydowndireccional += "\n";
    snippetkeydowndireccional += "<----....>If KeyDown(KEY_UP)\n";
    snippetkeydowndireccional += "\n";
    snippetkeydowndireccional += "<----....>End\n";
    snippetkeydowndireccional += "\n";
    snippetkeydowndireccional += "<----....>If KeyDown(KEY_DOWN)\n";
    snippetkeydowndireccional += "\n";
    snippetkeydowndireccional += "<----....>End\n";

    QString snippetkeydownkey;
    snippetkeydownkey += "<----....>If KeyDown(KEY_{{Xname}})\n";
    snippetkeydownkey += "\n";
    snippetkeydownkey += "<----....>End\n";

    // key hit
    QString snippetkeyhitdireccional;
    snippetkeyhitdireccional += "<----....>If KeyHit(KEY_LEFT)\n";
    snippetkeyhitdireccional += "\n";
    snippetkeyhitdireccional += "<----....>End\n";
    snippetkeyhitdireccional += "\n";
    snippetkeyhitdireccional += "<----....>If KeyHit(KEY_RIGHT)\n";
    snippetkeyhitdireccional += "\n";
    snippetkeyhitdireccional += "<----....>End\n";
    snippetkeyhitdireccional += "\n";
    snippetkeyhitdireccional += "<----....>If KeyHit(KEY_UP)\n";
    snippetkeyhitdireccional += "\n";
    snippetkeyhitdireccional += "<----....>End\n";
    snippetkeyhitdireccional += "\n";
    snippetkeyhitdireccional += "<----....>If KeyHit(KEY_DOWN)\n";
    snippetkeyhitdireccional += "\n";
    snippetkeyhitdireccional += "<----....>End\n";

    QString snippetkeyhitkey;
    snippetkeyhitkey += "<----....>If KeyHit(KEY_{{Xname}})\n";
    snippetkeyhitkey += "\n";
    snippetkeyhitkey += "<----....>End\n";

    QString snippetfieldlist;
    snippetfieldlist += "<---->Field {{Xnameminuscula}}s:List<{{Xname}}> = New List<{{Xname}}>\n";

    QString snippetlocallist;
    snippetlocallist += "<---->Local {{Xnameminuscula}}s:List<{{Xname}}> = New List<{{Xname}}>\n";

    QString snippetForEachin;
    snippetForEachin += "<---->For Local {{Xnameminuscula}}:Type = Eachin {{Xname}}\n";
    snippetForEachin += "<----....>\'Statements\n";
    snippetForEachin += "<---->Next\n";

    QString snippetlvec2df;
    snippetlvec2df += "Class Vec2Df\n";
    snippetlvec2df += "<---->Field x:Float\n";
    snippetlvec2df += "<---->Field y:Float\n";
    snippetlvec2df += "\n";
    snippetlvec2df += "<---->Method New(x:Float = 0, y:Float = 0)\n";
    snippetlvec2df += "<----....>Set(x, y)\n";
    snippetlvec2df += "<---->End\n";
    snippetlvec2df += "<---->Method Set:Void(x:Float, y:Float)\n";
    snippetlvec2df += "<----....>Self.x = x\n";
    snippetlvec2df += "<----....>Self.y = y\n";
    snippetlvec2df += "<---->End\n";
    snippetlvec2df += "End";

    QString snippetlvec2di;
    snippetlvec2di += "Class Vec2Di\n";
    snippetlvec2di += "<---->Field x:Int\n";
    snippetlvec2di += "<---->Field y:Int\n";
    snippetlvec2di += "\n";
    snippetlvec2di += "<---->Method New(x:Int = 0, y:Int = 0)\n";
    snippetlvec2di += "<----....>Set(x, y)\n";
    snippetlvec2di += "<---->End\n";
    snippetlvec2di += "<---->Method Set:Void(x:Int, y:Int)\n";
    snippetlvec2di += "<----....>Self.x = x\n";
    snippetlvec2di += "<----....>Self.y = y\n";
    snippetlvec2di += "<---->End\n";
    snippetlvec2di += "End";


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
        sFrom->append("@mojoapp");
        sTo->append(snippetmojox);
        sFrom->append("@m");
        sTo->append(snippetmethodname);
        sFrom->append("@cc");
        sTo->append(snippetclasename);
        sFrom->append("@kdd");
        sTo->append(snippetkeydowndireccional);
        sFrom->append("@kd-");
        sTo->append(snippetkeydownkey);
        sFrom->append("@khd");
        sTo->append(snippetkeyhitdireccional);
        sFrom->append("@kh-");
        sTo->append(snippetkeyhitkey);
        sFrom->append("@:mojoapp");
        sTo->append(snippetmojoxstrict);
        sFrom->append("@vm");
        sTo->append(snippetmethodnamestrictVoid);
        sFrom->append("@im");
        sTo->append(snippetmethodnamestrictInt);
        sFrom->append("@fm");
        sTo->append(snippetmethodnamestrictField);
        sFrom->append("@sm");
        sTo->append(snippetmethodnamestrictString);
        sFrom->append("@bm");
        sTo->append(snippetmethodnamestrictBool);
        sFrom->append("@fl");
        sTo->append(snippetfieldlist);
        sFrom->append("@ll");
        sTo->append(snippetlocallist);
        sFrom->append("@Fore");
        sTo->append(snippetForEachin);
        sFrom->append("@vec2df");
        sTo->append(snippetlvec2df);
        sFrom->append("@vec2di");
        sTo->append(snippetlvec2di);
        cnt = 24;
    }
    int pos = -1;
    bool checkQuotes = (s.indexOf("\"") > 0);
    if(trimmed.indexOf("'") == 0) {//skip comments
        return repl;
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

            // create new method init
            QString mncreate= sFrom->at(i);
            if(mncreate == QString("@m")){
                QString snippetcreatemethod = sTo->at(i);
                QString reemplazenameX = s.mid((sTo->at(i)).length()); // name

                reemplazenameX.replace(QString(" "), QString(""));

                if(reemplazenameX == QString("")){
                    s = "'require name method";
                }
                if(reemplazenameX != QString("")){
                    snippetcreatemethod.replace(QString("{{Xname}}"), QString(reemplazenameX)+"()");
                    s = snippetcreatemethod;
                }

            }
            if(mncreate == QString("@vm")){
                QString snippetcreatemethod = sTo->at(i);
                QString reemplazenameX = s.mid((sTo->at(i)).length()); // name

                reemplazenameX.replace(QString(" "), QString(""));

                if(reemplazenameX == QString("")){
                    s = "'require name method";
                }
                if(reemplazenameX != QString("")){
                    snippetcreatemethod.replace(QString("{{Xname}}"), QString(reemplazenameX));
                    s = snippetcreatemethod;
                }
            }
            if(mncreate == QString("@im")){
                QString snippetcreatemethod = sTo->at(i);
                QString reemplazenameX = s.mid((sTo->at(i)).length()); // name

                reemplazenameX.replace(QString(" "), QString(""));

                if(reemplazenameX == QString("")){
                    s = "'require name method";
                }
                if(reemplazenameX != QString("")){
                    snippetcreatemethod.replace(QString("{{Xname}}"), QString(reemplazenameX));
                    s = snippetcreatemethod;
                }
            }
            if(mncreate == QString("@fm")){
                QString snippetcreatemethod = sTo->at(i);
                QString reemplazenameX = s.mid((sTo->at(i)).length()); // name

                reemplazenameX.replace(QString(" "), QString(""));

                if(reemplazenameX == QString("")){
                    s = "'require name method";
                }
                if(reemplazenameX != QString("")){
                    snippetcreatemethod.replace(QString("{{Xname}}"), QString(reemplazenameX));
                    s = snippetcreatemethod;
                }
            }
            if(mncreate == QString("@sm")){
                QString snippetcreatemethod = sTo->at(i);
                QString reemplazenameX = s.mid((sTo->at(i)).length()); // name

                reemplazenameX.replace(QString(" "), QString(""));

                if(reemplazenameX == QString("")){
                    s = "'require name method";
                }
                if(reemplazenameX != QString("")){
                    snippetcreatemethod.replace(QString("{{Xname}}"), QString(reemplazenameX));
                    s = snippetcreatemethod;
                }
            }
            if(mncreate == QString("@bm")){
                QString snippetcreatemethod = sTo->at(i);
                QString reemplazenameX = s.mid((sTo->at(i)).length()); // name

                reemplazenameX.replace(QString(" "), QString(""));

                if(reemplazenameX == QString("")){
                    s = "'require name method";
                }
                if(reemplazenameX != QString("")){
                    snippetcreatemethod.replace(QString("{{Xname}}"), QString(reemplazenameX));
                    s = snippetcreatemethod;
                }
            }
            if(mncreate == QString("@cc")){
                QString snippetcreatemethod = sTo->at(i);
                QString reemplazenameX = s.mid((sTo->at(i)).length()); // name

                reemplazenameX.replace(QString(" "), QString(""));

                if(reemplazenameX == QString("")){
                    s = "'require name class";
                }
                if(reemplazenameX != QString("")){
                    snippetcreatemethod.replace(QString("{{Xname}}"), QString(reemplazenameX));
                    s = snippetcreatemethod;
                }
            }
            if(mncreate == QString("@mojoapp")){
                QString snippetcreatemethod = sTo->at(i); // all
                QString reemplazenameX = s.mid((sTo->at(i)).length()); // name

                reemplazenameX.replace(QString(" "), QString(""));

                if(reemplazenameX == QString("")){
                    snippetcreatemethod.replace(QString("{{Xname}}"), QString("Game"));
                }
                if(reemplazenameX != QString("")){
                    snippetcreatemethod.replace(QString("{{Xname}}"), QString(reemplazenameX));
                }

                //qDebug() << reemplazenameX;
                s = snippetcreatemethod;
            }
            if(mncreate == QString("@:mojoapp")){
                QString snippetcreatemethod = sTo->at(i); // all
                QString reemplazenameX = s.mid((sTo->at(i)).length()); // name

                reemplazenameX.replace(QString(" "), QString(""));

                if(reemplazenameX == QString("")){
                    snippetcreatemethod.replace(QString("{{Xname}}"), QString("Game"));
                }
                if(reemplazenameX != QString("")){
                    snippetcreatemethod.replace(QString("{{Xname}}"), QString(reemplazenameX));
                }

                //qDebug() << reemplazenameX;
                s = snippetcreatemethod;
            }
            if(mncreate == QString("@kd-")){
                QString snippetcreatemethod = sTo->at(i); // all
                QString reemplazenameX = s.mid((sTo->at(i)).length()); // name

                reemplazenameX.replace(QString(" "), QString(""));

                if(reemplazenameX == QString("")){
                    s = "'require key down";
                }
                if(reemplazenameX != QString("")){
                    reemplazenameX = reemplazenameX.toUpper();
                    snippetcreatemethod.replace(QString("{{Xname}}"), QString(reemplazenameX));
                    s = snippetcreatemethod;
                }
            }

            if(mncreate == QString("@kh-")){
                QString snippetcreatemethod = sTo->at(i); // all
                QString reemplazenameX = s.mid((sTo->at(i)).length()); // name

                reemplazenameX.replace(QString(" "), QString(""));
                if(reemplazenameX == QString("")){
                    s = "'require key hit";
                }
                if(reemplazenameX != QString("")){
                    reemplazenameX = reemplazenameX.toUpper();
                    snippetcreatemethod.replace(QString("{{Xname}}"), QString(reemplazenameX));
                    s = snippetcreatemethod;
                }
            }
            if(mncreate == QString("@fl")){
                QString snippetcreatemethod = sTo->at(i); // all
                QString reemplazenameX = s.mid((sTo->at(i)).length()); // name

                reemplazenameX.replace(QString(" "), QString(""));

                if(reemplazenameX == QString("")){
                    s = "'require name List";
                }
                if(reemplazenameX != QString("")){
                    snippetcreatemethod.replace(QString("{{Xname}}"), QString(reemplazenameX));
                    snippetcreatemethod.replace(QString("{{Xnameminuscula}}"), QString(reemplazenameX.toLower()));
                    s = snippetcreatemethod;
                }
            }
            if(mncreate == QString("@ll")){
                QString snippetcreatemethod = sTo->at(i); // all
                QString reemplazenameX = s.mid((sTo->at(i)).length()); // name

                reemplazenameX.replace(QString(" "), QString(""));

                if(reemplazenameX == QString("")){
                    s = "'require name List";
                }
                if(reemplazenameX != QString("")){
                    snippetcreatemethod.replace(QString("{{Xname}}"), QString(reemplazenameX));
                    snippetcreatemethod.replace(QString("{{Xnameminuscula}}"), QString(reemplazenameX.toLower()));
                    s = snippetcreatemethod;
                }
            }
            if(mncreate == QString("@Fore")){
                QString snippetcreatemethod = sTo->at(i); // all
                QString reemplazenameX = s.mid((sTo->at(i)).length()); // name

                reemplazenameX.replace(QString(" "), QString(""));

                if(reemplazenameX == QString("")){
                    s = "'require name collection";
                }
                if(reemplazenameX != QString("")){
                    snippetcreatemethod.replace(QString("{{Xname}}"), QString(reemplazenameX));
                    snippetcreatemethod.replace(QString("{{Xnameminuscula}}"), QString(  (reemplazenameX.toLower()).left(1) ));
                    s = snippetcreatemethod;
                }
            }
            // create new method end
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

    bool iff = (trimmed.startsWith("if") || trimmed.startsWith("If") || trimmed.startsWith("while") || trimmed.startsWith("While"));
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
            res += c;//(c == 9 ? '	' : ' ');
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
    //--------add space method and others init
    s.replace(QString("<---->"), QString("    "));
    s.replace(QString("<----....>"), QString("        "));
    //--------add space method and others end
    return repl;
}

QString CodeAnalyzer::trimmedRight(const QString &s) {
    int pos, n = s.length();
    for( pos = n-1 ; pos >= 0 && s.at(pos) <= ' ' ; --pos ) {}

    if( pos != n-1 && pos >= 0 ) {
        return s.left(pos+1);
    }
    return s;
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

void CodeAnalyzer::refreshIdentTypes() {
    if(_listForRefreshIdentTypes.isEmpty())
        return;
    foreach (CodeItem *item, _listForRefreshIdentTypes) {
        QString type = item->tempIdentType();
        bool isEachin = (type[0] == '+');
        if(isEachin)
            type = type.mid(1);
        int i = type.indexOf("<");
        if(i > 0) {
            //qDebug()<<"TYPE1:"<<type;
        }
        else {
            i = type.indexOf('(');
            type = type.left(i);
            //qDebug()<<"TYPE2:"<<type;
            QTextBlock b = item->block();
            CodeItem *code = findInScope(b,type.length(),0,true,type);
            if(code) {
                type = code->identType();
                if(isEachin && !code->templWords().isEmpty())
                    type = code->templWords().first();
                //qDebug()<<"TYPE3:"<<type;
            }
        }
        item->setIdentType(type);
    }
    _listForRefreshIdentTypes.clear();
}

void CodeAnalyzer::flushFileModified(const QString &path) {
    FileInfo *fi = userFilesModified()->value(path,0);
    if(fi)
        fi->modified = 0;
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
    _isClass = _isFunc = _isField = _isKeyword = _isMonkey = _isUser = false;
    _isParam = _isVar = _isInherited = _isInterface = _isProperty = _isPrivate = false;
    _identForInsert = "";
    _filepath = path;
    _filename = stripDir(path);
    _module = "";
    _blockNumber = block.blockNumber();
    _blockEndNumber = _blockNumber;
    //if(decl == "keyword")
    //qDebug() << "item: "+decl+" : "+line;

    int i = path.indexOf("modules");
    if(i > 0) {
        _module = path.mid(i+8);
        _module = _module.left(_module.length()-7);// .monkey
    }

    _isClass = (decl == "class");
    _isInterface = (decl == "interface");
    if(_isClass || _isInterface) {
        //qDebug() << "if 1";
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
                //qDebug()<< "add base: "+s;
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

    }
    else if(decl == "method" || decl == "function") {
        //qDebug() << "if 2";
        int i1 = line.indexOf("(");
        int i2 = line.indexOf(":");
        if (i1 < i2 || i2 < 0) {
            _ident = line.left(i1);
        } else if (i2) {
            _ident = line.left(i2);
            _identType = line.mid(i2+1, i1-i2-1);
        }
        //
        if ( i1 ) {
            i2 = line.indexOf(")");
            if (!i2)
                i2 = line.length();
            _descr = _descrAsItem = line.left(i2+1);
            int len = i2-i1-1;
            if (len > 0) {
                line = line.mid(i1+1,len);
                QStringList p = CodeAnalyzer::extractParams(line);
                foreach (QString s, p) {
                    s = s.trimmed();
                    CodeItem *item = new CodeItem("param", s, indent, block, path, 0);
                    this->addChild(item);
                    _params.append(item);
                }
            }
        }
        _isFunc = true;
    }
    else if(decl == "field" || decl == "global" || decl == "const" || decl == "local" || decl == "param") {
        //qDebug()<<"line:"<<line;
        int i;
        i = line.indexOf(":=");
        QString prefix = "";
        if(i > 0) {
            QString s1, s2, s3, t="";
            s1 = line.left(i).trimmed();
            s2 = line.mid(i+2).trimmed();
            //qDebug()<<s1<<s2<<path;
            i = s2.indexOf(' ');
            s3 = (i > 0 ? s2.left(i) : s2);
            //qDebug()<<"s3_1:"<<s3;
            if(s3 == "New" || s3 == "Eachin") {
                if(s3 == "Eachin")
                    prefix = "+";
                s2 = s2.mid(i+1);
                i = s2.indexOf(' ');
                s3 = (i > 0 ? s2.left(i) : s2);
                //qDebug()<<"s3_2:"<<s3;
                /*i = 0;
                int n = s3.length();
                while(i < n && isIdent(s3[i])) {
                    ++i;
                }
                if(i < n-1) {
                    s3 = s3.left(i);
                    qDebug()<<"s3_3:"<<s3;
                }*/
            }
            if(s3.startsWith('\"') || s3 == "String" || s3.startsWith("String(")) {
                t = "String";
            }
            else if(s3 == "Int") {
                t = "Int";
            }
            else if(s3 == "Float") {
                t = "Float";
            }
            else if(s3 == "True" || s3 == "False" || s3 == "Bool") {
                t = "Bool";
            }
            else if(isDigit(s3[0])) {
                if(s3.contains('.'))
                    t = "Float";
            }
            else {
                //need to store for futher processing
                _tempIdentType = prefix+s3;
                CodeAnalyzer::addToListForRefreshIdentTypes(this);
                //qDebug()<<"_tempIdentType:"<<_tempIdentType;
            }
            if(t != "") {
                //qDebug()<<"type:"<<t;
                setIdentType(t);
            }
            line = s1;
        }
        else {
            //qDebug() << "if 3";
            i = line.indexOf("=");
            if(i)
                line = line.left(i).trimmed();
            i = line.indexOf(":");
            if(i) {
                QString type = line.mid(i+1);
                line = line.left(i);
                setIdentType( type );
            }
        }
        _ident = line;
        _descrAsItem = _ident+":"+_identType;
        _isVar = true;
        _isField = (decl == "field" || decl == "global" || decl == "const");
        _isParam = (decl == "param");
    }
    else if(decl == "keyword") {
        //qDebug() << "if 4";
        _ident = line;
        //qDebug() << "keyword: "+line;
        if( line=="Include"||line=="Import"||line=="Module"||line=="Extern"||
                line=="New"||line=="Eachin"||
                line=="Extends"||/*topic=="Abstract"||topic=="Final"||*/line=="Native"||line=="Select"||line=="Case"||
                line=="Const"||line=="Local"||line=="Global"||line=="Field"||line=="Method"||line=="Function"||line=="Class"||line=="Interface"||line=="Implements"||
                line=="And"||line=="Or"||
                line=="Until"||line=="For"||line=="To"||line=="Step"||
                line=="Catch"||line=="Print" ) {
            _identForInsert = _ident+ " ";
            //qDebug() << "_identForInsert: "+_identForInsert;
        }
        _isKeyword = true;
    }
    else {
        //qDebug() << "if 5";
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
    d->addItem(this);
}

CodeItem::~CodeItem() {
    _parent = 0;
    //qDebug()<<"item.delete:"<<descrAsItem();
    if(hasChildren()) {
        foreach (CodeItem *i, children()) {
            delete i;
        }
        children().clear();
    }
}

void CodeItem::setIdentType(const QString &type) {
    _identType = type;
    //templates: Map <K, V>
    int i = _identType.indexOf("<");
    if(i > 0) {
        int i2 = _identType.indexOf(">");
        if(i2) {
            QString templ = _identType.mid(i+1,i2-i-1);
            templ = templ.trimmed();
            QStringList l = templ.split(",");
            foreach (QString s, l) {
                s = s.trimmed();
                _templWords.append(s);
                //qDebug() << "var templ: "+s;
            }
        }
    }
    _descrAsItem = _ident+":"+_identType;
}

QString CodeItem::descrAsItem() {
    return _descrAsItem;
}

void CodeItem::addChild(CodeItem *item) {
    _children.append(item);
    item->setParent(this);
}

int CodeItem::blockNumber() {
    if(isMonkey() || !_block.isValid())
        return _blockNumber;
    else //if(_block.isValid())
        return _block.blockNumber();
}

int CodeItem::blockEndNumber() {
    if(isMonkey() || !_blockEnd.isValid())
        return _blockEndNumber;
    else //if(_blockEnd.isValid())
        return _blockEnd.blockNumber();
}

CodeItem* CodeItem::child(const QString &ident, bool withBaseClasses) {
    if(withBaseClasses) {
        QList<CodeItem*> list;
        CodeAnalyzer::allClasses("",true,true,list,this);
        foreach (CodeItem *c, list) {
            QList<CodeItem*> list2 = c->children();
            foreach (CodeItem *item, list2) {
                if(item->ident() == ident)
                    return item;
            }
        }
    }
    else {
        foreach (CodeItem *item, children()) {
            if(item->ident() == ident)
                return item;
        }
    }
    return 0;
}

void CodeItem::addUnique(QList<CodeItem*> &list, CodeItem *item) {
    if(!isListContains(list, item))
        list.append(item);

}

bool CodeItem::isListContains(QList<CodeItem*> &list, CodeItem *item) {
    foreach (CodeItem *i, list) {
        if(CodeItem::equals(i, item)) {
            return true;
        }
    }
    return false;
}

CodeItem* CodeItem::findTheSame(QList<CodeItem*> &list, CodeItem *item) {
    foreach (CodeItem *i, list) {
        if(CodeItem::equals(i, item)) {
            return i;
        }
    }
    return 0;
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
    _blockEndNumber = block.blockNumber();
    QTextBlock b = this->block();
    BlockData *d = BlockData::data(b);
    if(d) {
        d->setBlockEnd(block);
    }
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

QString CodeItem::identTypeCleared() {
    return clearType(_identType);
}

CodeItem *CodeItem::parentClass() const
{
    CodeItem *p = _parent;
    while (p != 0) {
        if (p->isClass())
            break;
        p = p->parent();
    }
    return p;
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

QStringList CodeItem::baseClasses() {
    return _baseClasses;
}

bool CodeItem::equals(CodeItem *i1, CodeItem *i2) {
    if(i1 == i2)
        return true;
    if(i1->ident() != i2->ident())
        return false;
    if(i1->identTypeCleared() != i2->identTypeCleared())
        return false;
    QList<CodeItem*> params1 = i1->params();
    QList<CodeItem*> params2 = i2->params();
    if(params1.size() != params2.size())
        return false;
    for(int k = 0, size = params1.size(); k < size; ++k) {
        if(params1.at(k)->identType() != params2.at(k)->identType())
            return false;
    }
    return true;
}

bool CodeItem::isField() {
    return _isField;
    /*if(_parent != 0)
        qDebug()<<"has parent: "+_parent->descrAsItem();
    return (_isVar && _parent != 0 && _parent->isClass());*/
}

QString CodeItem::toolTip() {
    return "("+_decl+") "+_descrAsItem;
}

void CodeItem::setItemWithData(const QString &name, ItemWithData *iwd) {
    qDebug()<<"setItemWithData:"<<this->descrAsItem()<<name<<iwd->text();
    _itemsWithData.insert(name, iwd);
    if(_itemsWithData.size() > 1) {
        foreach(ItemWithData *i, _itemsWithData) {
            qDebug()<<"iwd: "<<i->text();
        }
    }
}

void CodeItem::setKind(int kind) {
    setIsMonkey(kind == CodeAnalyzer::KIND_MONKEY);
    setIsUser(kind == CodeAnalyzer::KIND_USER);
}

QString CodeItem::summary() {
    int fnc=0, fld=0, mtd=0, cnst=0, glbl=0;
    QList<CodeItem*> list = children();
    foreach (CodeItem *i, list) {
        QString d = i->decl();
        if(d == "function")
            ++fnc;
        else if(d == "method")
            ++mtd;
        else if(d == "const")
            ++cnst;
        else if(d == "global")
            ++glbl;
        else if(d == "field")
            ++fld;
    }
    QString s = "<u><b>"+decl()+" "+descrAsItem()+":</b></u>";
    s += "<table widt2h='222' cellspacing='5'>";
    s += "<tr><td align='right'>fields:</td><td>"+QString::number(fld)+"</td></tr>";
    s += "<tr><td align='right'>globals:</td><td>"+QString::number(glbl)+"</td></tr>";
    s += "<tr><td align='right'>consts:</td><td>"+QString::number(cnst)+"</td></tr>";
    s += "<tr><td align='right'>methods:</td><td>"+QString::number(mtd)+"</td></tr>";
    s += "<tr><td align='right'>functions:</td><td>"+QString::number(fnc)+"</td></tr>";
    s += "</table><br><br>";


    QString basec="", basei="";
    QList<CodeItem*> list2, list3;
    list.clear();
    CodeAnalyzer::allClasses("",true,true,list,this);
    fnc=0; fld=0; mtd=0; cnst=0; glbl=0;
    int inhmtd=0;
    bool hasInher = (list.size() > 1);
    if(hasInher) {
        foreach (CodeItem *i, list) {
            if(i->isClass()) {
                if(basec != "")
                    basec += ", ";
                basec += i->descrAsItem();
            }
            else if(i->isInterface()) {
                if(basei != "")
                    basei += ", ";
                basei += i->descrAsItem();
            }
            list2 = i->children();
            foreach (CodeItem *i, list2) {
                if(isListContains(list3,i)) {
                    ++inhmtd;
                    continue;
                }
                list3.append(i);
                QString d = i->decl();
                if(d == "function")
                    ++fnc;
                else if(d == "method")
                    ++mtd;
                else if(d == "const")
                    ++cnst;
                else if(d == "global")
                    ++glbl;
                else if(d == "field")
                    ++fld;
            }
        }
        if(basei == "")
            basei = "-";
        s += "<u>with inheritance:</u>";
        s += "<table width2='222' cellspacing='5'>";
        s += "<tr><td align='right'>classes:</td><td>"+basec+"</td></tr>";
        s += "<tr><td align='right'>interfaces:</td><td>"+basei+"</td></tr>";
        s += "<tr><td align='right'>fields:</td><td>"+QString::number(fld)+"</td></tr>";
        s += "<tr><td align='right'>globals:</td><td>"+QString::number(glbl)+"</td></tr>";
        s += "<tr><td align='right'>consts:</td><td>"+QString::number(cnst)+"</td></tr>";
        s += "<tr><td align='right'>methods:</td><td>"+QString::number(mtd)+"</td></tr>";
        s += "<tr><td align='right'>functions:</td><td>"+QString::number(fnc)+"</td></tr>";
        s += "<tr><td align='right'>overridden:</td><td>"+QString::number(inhmtd)+"</td></tr>";
        s += "</table>";
    }
    return s;
}

QString CodeItem::fullItemPath() {
    QString s = descrAsItem();
    CodeItem *i = this;
    while(i->parent()) {
        i = i->parent();
        s = i->descrAsItem()+"$"+s;
    }
    return _filename+"$"+s;
}
