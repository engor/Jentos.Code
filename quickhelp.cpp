/*
Jentos IDE.

Copyright 2014, Evgeniy Goroshkin.

See LICENSE.TXT for licensing terms.
*/

#include "quickhelp.h"

QuickHelp::QuickHelp(QObject *parent) :
    QObject(parent)
{
    isKeyword = isGlobal = false;
    topic = ltopic = url = kind = module = descr = classs = descrForInsert = "";
}

QuickHelp::~QuickHelp(){
}


bool QuickHelp::init( QString &monkeyPath ) {

    map()->clear();
    map2()->clear();

    QString text, line, topic, url, kind, module, descr, classs, tmp, paramsLine;
    QStringList lines;
    QuickHelp *h;

    QString path = QApplication::applicationDirPath()+"/";
    if( monkeyPath.isEmpty() )
        monkeyPath = path;

    QFile file0( path+"keywords.txt" );
    if( file0.open( QIODevice::ReadOnly ) ) {
        QTextStream stream( &file0 );
        stream.setCodec( "UTF-8" );
        text = stream.readAll();
        file0.close();
    }
    else {
        text = "Void;Strict;Public;Private;Property;"
            "Bool;Int;Float;String;Array;Object;Mod;Continue;Exit;"
            "Include;Import;Module;Extern;"
            "New;Self;Super;EachIn;True;False;Null;Not;"
            "Extends;Abstract;Final;Native;Select;Case;Default;"
            "Const;Local;Global;Field;Method;Function;Class;Interface;Implements;"
            "And;Or;Shl;Shr;End;If;Then;Else;ElseIf;EndIf;While;Wend;Repeat;Until;Forever;For;To;Step;Next;Return;Inline;"
            "Try;Catch;Throw;Throwable;"
            "Print;Error;Alias";
    }

    lines = text.split(';');

    for( int i = 0 ; i < lines.size() ; ++i ){
        topic = lines.at(i);
        h = new QuickHelp();
        h->topic = topic;
        h->descr = topic;
        h->descrForInsert = topic;
        h->kind = "keyword";
        h->isKeyword = true;
        if( topic=="Include"||topic=="Import"||topic=="Module"||topic=="Extern"||
                topic=="New"||topic=="EachIn"||
                topic=="Extends"||/*topic=="Abstract"||topic=="Final"||*/topic=="Native"||topic=="Select"||topic=="Case"||
                topic=="Const"||topic=="Local"||topic=="Global"||topic=="Field"||topic=="Method"||topic=="Function"||topic=="Class"||topic=="Interface"||topic=="Implements"||
                topic=="And"||topic=="Or"||
                topic=="Until"||topic=="For"||topic=="To"||topic=="Step"||
                topic=="Catch"||topic=="Print" ) {
            h->descrForInsert += " ";
        }
        insert( h );
    }

    QFile file( monkeyPath+"/docs/html/index.txt" );
    if( file.open( QIODevice::ReadOnly ) ) {
        QTextStream stream( &file );
        stream.setCodec( "UTF-8" );
        text = stream.readAll();
        file.close();
        lines = text.split('\n');

        //global values for highlight
        for( int i = 0 ; i < lines.count() ; ++i ) {
            line = lines.at( i ).trimmed();
            int j = line.lastIndexOf( ':' );
            if( j == -1 )
                continue;
            topic = line.left(j);
            url = "file:///"+monkeyPath+"/docs/html/"+line.mid(j+1);
            h = help( topic );
            if( !h ) {
                h = new QuickHelp();
            }
            else if( h->isKeyword ) {
                h->url = url;
                continue;
            }
            h = new QuickHelp();
            h->topic = topic;
            h->url = url;
            h->isGlobal = false;
            insert( h );
        }
    }

    //all values with detailed description
    QFile file2( monkeyPath+"/docs/html/decls.txt" );
    if( !file2.open( QIODevice::ReadOnly ) )
        return false;
    QTextStream stream2( &file2 );
    stream2.setCodec( "UTF-8" );
    text = stream2.readAll();
    file2.close();
    lines = text.split('\n');

    for( int i = 0 ; i < lines.count() ; ++i ) {
        line = lines.at( i ).trimmed();
        line = line.left( line.length()-1 );
        int j = line.lastIndexOf( ';' );
        if( j == -1 )
            continue;

        topic = url = descr = kind = module = classs = paramsLine = "";

        descr = line.left(j);
        url = line.mid(j+1);

        h = help( topic );
        if( !h ) {
            h = new QuickHelp();
        }
        else if( h->isKeyword ) {
            continue;
        }

        //kind is {function, method, const, ...}
        j = descr.indexOf( ' ' );
        if( j > 0) {
            kind = descr.left(j);
            descr = descr.mid(j+1);
        }

        if( kind == "Module") {
            j = topic.lastIndexOf('.');
            if(j > 0) {
                module = topic.left(j);
                topic = topic.mid(j+1);
            }
            else {
                module = topic = descr;
            }
        }
        else {
            j = descr.indexOf( ':' );
            int j2 = descr.indexOf( '(' );
            if(j < 0 || (j2 > 0 && j2 < j))
                j = j2;
            if(j > 0) {
                topic = descr.left(j);
            }
            else {
                topic = descr;
            }
            j = topic.lastIndexOf( '.' );
            if(j > 0) {
                tmp = topic.left(j);
                topic = topic.mid(j+1);
                descr = descr.mid(j+1);
                j = tmp.lastIndexOf( '.' );
                if( j > 0 ) {
                    classs = tmp.mid(j+1);
                    if( url.indexOf( "_"+classs ) > 0) {
                        module = tmp.left(j);
                    }
                    else {
                        classs = "";
                        module = tmp;
                    }
                }
            }
            //params if exists
            j2 = descr.indexOf( '(' );
            if( j2 > 0) { // (
                paramsLine = descr.mid(j2+1);
                j = paramsLine.lastIndexOf( ')' );
                if( j >= 0 ) {
                    paramsLine = paramsLine.left(j);
                    if( paramsLine == "" )
                        paramsLine = "*"; //has empty brackets
                }
            }
        }

        h->topic = topic;
        h->url = "file:///"+monkeyPath+"/docs/html/"+url;
        h->descr = descr;
        h->kind = kind.toLower();
        h->module = module;
        h->classs = classs;
        h->descrForInsert = topic;
        if( paramsLine != "" ) {
            h->params = paramsLine.split( ',' );
        }
        h->isGlobal = (h->kind=="function" || h->kind=="global" || h->kind=="const");

        insert( h );

    }

    return true;
}

void QuickHelp::insert(QuickHelp *help ) {
    help->ltopic = help->topic.toLower();
    map()->insert( help->ltopic, help );
    map2()->insert( help->descr, help );
}

QuickHelp* QuickHelp::help( const QString &topic ) {
    return map()->value( topic.toLower(), 0 );
}

QuickHelp* QuickHelp::help2( const QString &descr ) {
    return map2()->value( descr, 0 );
}

QMap<QString,QuickHelp*>* QuickHelp::map() {
    static QMap<QString, QuickHelp*> *m = 0;
    if( !m )
        m = new QMap<QString,QuickHelp*>;
    return m;
}

QMap<QString,QuickHelp*>* QuickHelp::map2() {
    static QMap<QString, QuickHelp*> *m = 0;
    if( !m )
        m = new QMap<QString,QuickHelp*>;
    return m;
}

QString QuickHelp::quick() {
    QString mod = (module.isEmpty() || topic == module ? "" : module+" > ");
    QString cl = (classs.isEmpty() ? "" : classs+" > ");
    return mod + cl + "(" + kind + ") " + descr;
}

QString QuickHelp::hint() {
    QString mod = (module.isEmpty() || topic == module ? "" : module);
    QString cl = (classs.isEmpty() ? "" : classs);
    QString s = "(" + kind + ") <b>" + descr+"</b>";
    if( mod != "" )
        s += "\n<i>Declared in:</i> "+mod;
    if( cl != "" )
        s += " > "+cl;
    return s;
}
