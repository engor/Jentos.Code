/*
Jentos IDE.

Copyright 2014, Evgeniy Goroshkin.

See LICENSE.TXT for licensing terms.
*/

#ifndef CODEANALYZER_H
#define CODEANALYZER_H

#include "std.h"

class CodeItem;
class ItemWithData;
class CodeScope;
class FileInfo;
class ImportedFiles;
class ListWidgetCompleteItem;

class CodeAnalyzer : public QObject {
    Q_OBJECT

public:
    CodeAnalyzer(QObject *parent = 0);
    static void init();
    static void finalize();
    static void loadTemplates( const QString &path );
    static void loadKeywords( const QString &path );
    static QString templateWord( const QString &key );
    static QString keyword( const QString &key );
    static void analyzeDir(const QString &path , const QStringList &exclude);
    static bool analyzeFile(const QString &path , int kind=KIND_MONKEY);
    static bool needCloseWithEnd( const QString &line );
    static QIcon identIcon( const QString &ident );
    //static void parseCodeItems(QString line , const QTextBlock &block);
    static bool parse(QTextDocument *doc, const QString &path, int kind=0, QList<QTextBlock> *blocks=0); //parse all document
    static void insertItem(QString ident, CodeItem *codeItem/*, const QString &path*/, int kind=0);
    static bool autoFormat(QString &s , bool force=false);
    static QString trimmedRight(const QString &s );
    static QString clearSpaces( QString &s );
    static CodeItem* itemKeyword(const QString &ident );
    static CodeItem* itemMonkey(const QString &ident );
    static CodeItem* itemUser(const QString &ident , bool withChildren=false);
    static bool containsMonkey(const QString &ident );
    static bool containsUser(const QString &ident );
    static bool containsKeyword(const QString &ident );
    static void clearMonkey();
    static void printKeywords();
    static QString toolTip(const QString &ident, const QTextBlock &block);
    static QString toolTip(CodeItem *itemAtBlock);
    static void removeUserFile(const QString &path);
    static void remove(CodeItem *item);
    static void removeByPath(const QString &path);
    //
    static void fillListInheritance(const QTextBlock &block, QListWidget *l);
    static void fillListFromCommon(QListWidget *l, const QString &ident , const QTextBlock &block);
    static void fillTree();
    static void fillListFromScope(QListWidget *l, const QString &ident, CodeScope scope );
    static void allClasses(QString ident, bool addSelf, bool addBase, QList<CodeItem *> &list, CodeItem *item=0);
    static void allClasses(QList<CodeItem*> &targetList);
    static bool checkScopeForPrivate(CodeItem *item, CodeItem *scopeItem);
    static ListWidgetCompleteItem *tryToAddItemToList(CodeItem *item, CodeItem *scopeItem, QListWidget *list);
    //
    static void setShowVariables(bool value);
    static void setSortByName(bool value);
    static void setCurFilePath(const QString &filepath);
    //
    static QStringList extractParams(const QString &text);
    static CodeItem* findInScope(const QTextBlock &block, int pos, QListWidget *l=0, bool findLastIdent=false, const QString &blockText="");
    //static CodeItem* findInScope(const QString &ident, const QTextBlock &block, bool outsideScope=true);
    static CodeItem* scopeAt(const QTextBlock &block, bool classOnly=false, bool checkCurFile=false);
    static CodeItem* findScopeForIdent(const QString &ident, const QTextCursor &cursor, CodeScope &scope);
    static CodeItem* remAt(const QTextBlock &block);
    static CodeItem* foldAt(const QTextBlock &block);
    static void identsList(const QString &text, int cursorPos, QStringList &list);
    static CodeItem *isBlockHasClassOrFunc(const QTextBlock &block);
    //
    static void disable() { _disabled = true; }
    static void enable() { _disabled = false; }
    static bool isBetweenQuotes(QString text, int pos);
    //
    static QString tab() { return _tab; }
    static void setTabSize(int size, bool useSpaces);

    static const int KIND_USER = 0;
    static const int KIND_MONKEY = 1;
    static CodeAnalyzer* instance();

    static bool isSortByName() { return _isSortByName; }
    static bool isShowVariables() { return _isShowVariables; }
    static bool isShowInherited() { return _isShowInherited; }
    static bool isFillAucompWithInheritance() { return _isFillAucompWithInheritance; }

    static void setViews(QTreeView *tree, QListView *list) {
        if(!_treeView) {
            _treeView = tree;
            _listView = list;
            tree->setModel( treeItemModel() );
            list->setModel( listItemModel() );
        }
    }
    static QTreeView* treeView() { return _treeView; }
    static QListView* listView() { return _listView; }
    static QStandardItemModel* treeItemModel();
    static QStandardItemModel* listItemModel();
    static QStandardItem* itemInTree( const QModelIndex &index );
    static ItemWithData* itemInList( const QModelIndex &index );
    static ItemWithData* itemInList(int row);

    static void linkCodeItemWithStandardItem(CodeItem *i, QStandardItem *si);
    static CodeItem* getCodeItemFromStandardItem(QStandardItem *si);
    static QString fullStandardItemPath(QStandardItem *si);
    static QStandardItem* getStandardItem(QString path);
    static QList<CodeItem*> listForRefreshIdentTypes() { return _listForRefreshIdentTypes; }
    static void addToListForRefreshIdentTypes(CodeItem *i) { _listForRefreshIdentTypes.append(i); }
    static void refreshIdentTypes();
    static QList<int> listFoldTypes() { return _listFoldTypes; }
    static void extractIdents(const QString &text, QStringList &list);
    static void flushFileModified(const QString &path);
    static QHash<QString,QTextDocument*> docs() {
        return _docs;
    }
    static void storeCurFilePath() {
        _storedFilePath = _curFilePath;
    }
    static void restoreCurFilePath() {
        _curFilePath = _storedFilePath;
    }
    static void begin();
    static void end();

    static CodeItem *getClassOrKeyword(const QString &ident);

private:
    static QList<int> _listFoldTypes;

    static QList<CodeItem*> _listForRefreshIdentTypes;
    static QList<CodeItem*> _listUserItems;
    static QHash<QString,CodeItem*> _codeItemLinks;
    static QHash<QString,QStandardItem*> _standardItemLinks;

    static QTreeView *_treeView;
    static QListView* _listView;

    static QHash<QString, QString>* mapTemplates();
    static QMap<QString,CodeItem*>* mapMonkey();
    static QHash<QString, CodeItem *> *mapUser();
    static QHash<QString, CodeItem *> *mapKeywords();
    static QHash<QString, CodeItem *> *mapRem();
    static QHash<QString, CodeItem *>* mapFolds();
    static QHash<QString, QTextDocument *> _docs;

    static QHash<QString, ImportedFiles*> _imports;/*() {
        static QHash<QString, QList<QString> > *h = 0;
        if(h == 0)
            h = new QHash<QString, QList<QString> >;
        return h;
    }*/

    static QStringList _userFiles, _storedUserFiles;
    static QHash<QString,FileInfo*>* userFilesModified() {
        static QHash<QString,FileInfo*> *h = 0;
        if(h == 0)
            h = new QHash<QString,FileInfo*>;
        return h;
    }

    static QString _curFilePath, _storedFilePath;
    static bool _isShowVariables, _isSortByName, _isShowInherited, _isFillAucompWithInheritance;
    static bool _disabled;
    //
    static QString _tab, _tabAsTab, _tabAsSpaces;
    static bool _tabUseSpaces, _doAutoformat;
    static int _tabSize;

public slots:
    void onPrefsChanged( const QString &name );

};


//----------- CODE ITEM -----------------------------
class CodeItem : public QObject {
    Q_OBJECT

public:
    CodeItem(QString decl, QString line, int indent, QTextBlock &block, const QString &path, CodeItem *parent=0);
    ~CodeItem();


    static void addUnique(QList<CodeItem*> &list, CodeItem *item);
    static QString clearType(QString &s);
    static bool equals(CodeItem *i1, CodeItem *i2);
    static bool isListContains(QList<CodeItem*> &list, CodeItem *item);
    static CodeItem* findTheSame(QList<CodeItem*> &list, CodeItem *item);

    QString summary();
    void setIdentType(const QString &type);

    QString ident() { return _ident; }
    QString decl() const { return _decl; }
    QString descr() { return _descr; }
    QString descrAsItem();
    QString identForInsert() { return _identForInsert; }
    QString identWithParamsBraces(int &cursorDelta);
    QString identType() { return _identType; }
    QString identTypeCleared();
    QString filepath() { return _filepath; }
    QString filename() { return _filename; }
    QString module() { return _module; }
    QTextBlock block() { return _block; }
    QTextBlock blockEnd() { return _blockEnd; }
    QList<CodeItem*> params() const { return _params; }
    CodeItem* parent() const { return _parent; }
    CodeItem* parentClass() const;
    QList<CodeItem*> children() { return _children; }
    QString toString() { return "("+_decl+") "+_descr; }
    QString infoShort() { return "("+_decl+") "+_ident; }
    void addChild(CodeItem *item);
    void setParent(CodeItem *item){ _parent = item; }
    int indent(){ return _indent; }
    int blockNumber();
    int blockEndNumber();
    void setFoldable(bool value);
    bool isFoldable(){ return _foldable; }

    bool isProperty(){ return _isProperty; }
    void markAsProperty(){
        _isProperty = true;
        if (_params.size() > 0) {
            CodeItem *i = _params.at(0);
            _identType = i->identType();
        }
        //_descr = _descrAsItem = "<b>"+_ident+"</b> : <i>"+_identType+"</i>";
        _descr = _descrAsItem = _ident+" : "+_identType;
        _decl = "property";
    }

    bool isPrivate(){ return _isPrivate; }
    void markAsPrivate(){ _isPrivate = true; }

    bool isArray(){ return _isArray; }
    void markAsArray(){ _isArray = true; }

    bool hasChildren(){ return !_children.isEmpty(); }
    bool isClassOrInterface(){ return _isClass||_isInterface; }
    bool isClass(){ return _isClass; }
    bool isInterface(){ return _isInterface; }
    bool isFunc(){ return _isFunc; }
    bool isField();
    bool isVar(){ return _isVar; }
    bool isParam(){ return _isParam; }
    bool isKeyword(){ return _isKeyword; }
    bool isInnerItem(){ return _isInnerItem; }
    bool isMonkey(){ return _isMonkey; }
    bool isUser(){ return _isUser; }
    void setIsMonkey(bool value){ _isMonkey = value; }
    void setIsUser(bool value){ _isUser = value; }
    CodeItem* child(const QString &ident, bool withBaseClasses=false);
    void setBlockEnd(const QTextBlock &block);
    void setItemWithData(const QString &name, ItemWithData *iwd);
    ItemWithData* itemWithData(const QString &name){ return _itemsWithData.value(name,0); }
    bool hasBaseClasses(){ return !_baseClasses.isEmpty(); }
    QStringList baseClasses();
    QStringList templWords(){ return _templWords; }
    void setKind(int kind);
    QString toolTip();
    void setIsInherited() { _isInherited = true; }
    bool isInherited() { return _isInherited; }
    QString fullItemPath();
    QString tempIdentType() { return _tempIdentType; }
    bool isMultiLine() { return _blockNumber != _blockEndNumber; }


private:
    QString _decl, _ident, _identType, _descr, _descrAsItem, _identForInsert, _filepath, _filename, _module;
    QList<CodeItem*> _params;
    CodeItem *_parent;
    QTextBlock _block, _blockEnd;
    QList<CodeItem*> _children;
    int _indent, _blockNumber, _blockEndNumber;
    bool _foldable;
    bool _isClass, _isFunc, _isField, _isVar, _isKeyword, _isParam, _isInherited, _isInterface, _isProperty, _isPrivate, _isArray, _isInnerItem;
    bool _isMonkey, _isUser;
    QHash<QString,ItemWithData*> _itemsWithData;
    QStringList _baseClasses, _templWords;
    QString _tempIdentType;
    void updateDescrAsItem();
};


class CodeScope {
public:
    CodeScope() {
        item = 0;
        isSelf = isSuper = false;
        ident = "";
    }
    ~CodeScope(){}
    void set(CodeItem *i, bool self=false, bool super=false) {
        item = i;
        isSelf = self;
        isSuper = super;
    }
    void flush() {
        item = 0;
        isSelf = isSuper = false;
        ident = toolTip = "";
    }

    CodeItem *item;
    bool isSelf, isSuper;
    QString ident, toolTip;
};

class FileInfo {
public:
    FileInfo(const QString &path) {
        this->path = path;
        QFileInfo info(path);
        modified = info.lastModified().toMSecsSinceEpoch();
    }
    QString path;
    uint modified;
    bool isModified() {
        QFileInfo info(path);
        uint time = info.lastModified().toMSecsSinceEpoch();
        if(time != modified) {
            modified = time;
            return true;
        }
        return false;
    }
};

class ImportedFiles {
public:
    void append(const QString &path) {
        if(!files.contains(path))
            files.append(path);
    }

    QStringList files;
};


class UsagesResult {
public:
    UsagesResult(QTreeWidgetItem *treeitem, const QString &path, const QString &ident, int block, int start, int end) {
        this->path = path;
        this->ident = ident;
        blockNumber = block;
        positionStart = start;
        positionEnd = end;
    }
    static UsagesResult* add(QTreeWidgetItem *treeitem, const QString &path, const QString &ident, int block, int start, int end) {
        UsagesResult *u = new UsagesResult(treeitem, path, ident, block, start, end);
        usages.insert(treeitem,u);
        return u;
    }
    static UsagesResult* item(QTreeWidgetItem *treeitem) {
        return usages.value(treeitem);
    }
    static void clear() {
        foreach (UsagesResult *i, usages.values()) {
            delete i;
        }
        usages.clear();
    }

    QString path, ident;
    int blockNumber, positionStart, positionEnd;
    static QHash<QTreeWidgetItem*,UsagesResult*> usages;

};


#endif // CODEANALYZER_H
