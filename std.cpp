/*
Ted, a simple text editor/IDE.

Copyright 2012, Blitz Research Ltd.

See LICENSE.TXT for licensing terms.
*/

#include "std.h"

QString stripDir( const QString &path ){
    int i=path.lastIndexOf( '/' );
    if( i==-1 ) return path;
    return path.mid( i+1 );
}

QString extractDir( const QString &path ){
    int i=path.lastIndexOf( '/' );
    if( i==-1 ) return "";
#ifdef Q_OS_WIN32
    if( i && path[i-1]==':' ) return "";
#endif
    return path.left( i );
}

QString extractExt( const QString &path ){
    int i=path.lastIndexOf( '.' )+1;
    return i && path.indexOf( '/',i )==-1 ? path.mid( i ) : "";
}

// Converts \ to /, removes trailing /s and prefixes drive if necessary.
//
QString fixPath( QString path ){
    if( path.isEmpty() ) return path;

    if( isUrl( path ) ) return path;

    path=path.replace( '\\','/' );
    path=QDir::cleanPath( path );

#ifdef Q_OS_WIN32
    if( path.startsWith( "//" ) ) return path;
    if( path.startsWith( '/' ) ) path=QDir::rootPath()+path.mid( 1 );
    if( path.endsWith( '/' ) && !path.endsWith( ":/" ) ) path=path.left( path.length()-1 );
#else
    if( path.endsWith( '/' ) && path!="/" ) path=path.left( path.length()-1 );
#endif

    return path;
}

bool removeDir( const QString &path ){

    bool result=true;
    QDir dir( path );

    if( dir.exists( path ) ){
        Q_FOREACH( QFileInfo info,dir.entryInfoList( QDir::NoDotAndDotDot|QDir::System|QDir::Hidden|QDir::AllDirs|QDir::Files,QDir::DirsFirst ) ){
            if( info.isDir() ){
                result=removeDir( info.absoluteFilePath() );
            }else{
                result=QFile::remove( info.absoluteFilePath() );
            }
            if( !result ) return result;
        }
        result=dir.rmdir( path );
    }

    return result;
}

bool isUrl( const QString &path ){
    return path.startsWith( "file:" ) || path.startsWith( "http:" ) || path.startsWith( "https:" ) || path.startsWith( "qrc:" );
}


