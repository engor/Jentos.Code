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
    static void analyzeFile( const QString &path );
    static bool needCloseWithEnd( const QString &line );
    static QIcon identIcon( const QString &ident );
    //static void parseCodeItems(QString line , const QTextBlock &block);
    static bool parse(QTextBlock block, const QString &path, int kind=0); //parse all document
    static void insertItem(QString ident, CodeItem *codeItem/*, const QString &path*/, int kind=0);
    static CodeItem *classItem( const QString &ident );
    static bool autoFormat( QString &s );
    static QString clearSpaces( QString &s );
    static CodeItem* itemMonkey(const QString &ident );
    static CodeItem* itemUser(const QString &ident , bool withChildren=false);
    static bool containsMonkey(const QString &ident );
    static bool containsUser(const QString &ident );
    static bool containsKeyword(const QString &ident );
    static void clearMonkey();
    static QString toolTip(const QString &ident, const QTextBlock &block);
    static QString toolTip(CodeItem *itemAtBlock);
    static CodeItem* itemAtBlock(const QString &ident, const QTextBlock &block);
    static CodeItem* findItem(CodeItem *parent, const QString &ident);
    static void removeUserFile(const QString &path);
    static void remove(CodeItem *item);
    static void removeByPath(const QString &path);
    //
    static void fillListCommon(QListWidget *l, const QString &ident , const QTextBlock &block);
    static void fillListVarType( QListWidget *l, const QString &vartype );
    static void fillTree(QStandardItemModel *im);
    static void fillListByChildren(QListWidget *l, const QString &ident, CodeScope scope );
    //
    static void setShowVariables(bool value);
    static void setSortByName(bool value);
    static void setCurFilePath(const QString &filepath);
    //
    static QStringList extractParams(const QString &text);
    static CodeItem* findInScope(const QString &ident, const QTextBlock &block);
    static CodeItem* findScopeForBlock(const QTextBlock &block, bool classOnly=false);
    static CodeItem* findScopeForIdent(const QString &ident, const QTextCursor &cursor, CodeScope &scope);
    static CodeItem* findRemForBlock(const QTextBlock &block);
    static CodeItem* findFoldForBlock(const QTextBlock &block);
    //
    static void disable() { _disabled = true; }
    static void enable() { _disabled = false; }
    static bool isBetweenQuotes(QString text, int pos);

    static const int KIND_USER = 0;
    static const int KIND_MONKEY = 1;

private:
    static QMap<QString, QString>* mapTemplates();
    static QMap<QString,CodeItem*>* mapMonkey();
    static QMap<QString, CodeItem *>* mapUser();
    static QMap<QString, CodeItem *>* mapKeywords();
    static QMap<QString, CodeItem *>* mapRem();
    static QMap<QString, CodeItem *>* mapFolds();
    static QStringList _userFiles;
    static QString _lastToolTipIdent, _lastToolTipString, _curFilePath;
    static bool _isShowVariables, _isSortByName;
    static bool _disabled;
};


//----------- CODE ITEM -----------------------------
class CodeItem : public QObject {
    Q_OBJECT

public:
    CodeItem(QString decl, QString line, int indent, QTextBlock &block, const QString &path, CodeItem *parent=0);
    ~CodeItem();

    static void addUnique(QList<CodeItem*> &list, CodeItem *item);
    static QString clearType(QString &s);

    QString ident() { return _ident; }
    QString decl() const { return _decl; }
    QString descr() { return _descr; }
    QString descrAsItem();
    QString identForInsert() { return _identForInsert; }
    QString identWithParamsBraces(int &cursorDelta);
    QString identType() { return _identType; }
    QString identTypeCleared();
    QString filepath() { return _filepath; }
    QString module() { return _module; }
    QTextBlock block() { return _block; }
    QTextBlock blockEnd() { return _blockEnd; }
    QStringList params() const { return _params; }
    CodeItem* parent() const { return _parent; }
    QList<CodeItem*> children() { return _children; }
    QString toString() { return "("+_decl+") "+_descr; }
    QString infoShort() { return "("+_decl+") "+_ident; }
    void addChild(CodeItem *item);
    void setParent(CodeItem *item){ _parent = item; }
    int indent(){ return _indent; }
    int lineNumber();
    void setFoldable(bool value);
    bool isFoldable(){ return _foldable; }
    bool hasChildren(){ return !_children.isEmpty(); }
    bool isClass(){ return _isClass; }
    bool isFunc(){ return _isFunc; }
    bool isField(){ return _isField; }
    bool isKeyword(){ return _isKeyword; }
    bool isMonkey(){ return _isMonkey; }
    bool isUser(){ return _isUser; }
    void setIsMonkey(bool value){ _isMonkey = value; }
    void setIsUser(bool value){ _isUser = value; }
    CodeItem* child(const QString &decl, const QString &ident);
    void fillByChildren(const QString &decl, const QString &identStarts, QList<CodeItem *> &list);
    void setBlockEnd(const QTextBlock &block);
    void setItemWithData(ItemWithData *iwd){ _itemWithData = iwd; }
    ItemWithData* itemWithData(){ return _itemWithData; }
    bool hasBaseClasses(){ return !_baseClasses.isEmpty(); }
    QStringList baseClasses(){ return _baseClasses; }
    QStringList templWords(){ return _templWords; }
    QString fullDescr();

private:
    QString _decl, _ident, _identType, _descr, _descrAsItem, _identForInsert, _filepath, /*_helpUrl,*/ _module;
    QStringList _params;
    CodeItem *_parent;
    QTextBlock _block, _blockEnd;
    QList<CodeItem*> _children;
    int _indent, _lineNumber;
    bool _foldable;
    bool _isClass, _isFunc, _isField, _isKeyword, _isParam;
    bool _isMonkey, _isUser;
    ItemWithData *_itemWithData;
    QStringList _baseClasses, _templWords;

};


class CodeScope {
public:
    CodeScope() {
        item = 0;
        isSelf = isSuper = false;
    }
    ~CodeScope(){}
    void set(CodeItem *i, bool self=false, bool super=false) {
        item = i;
        isSelf = self;
        isSuper = super;
    }
    CodeItem *item;
    bool isSelf, isSuper;
};

#endif // CODEANALYZER_H
