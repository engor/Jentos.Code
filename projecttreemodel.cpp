/*
Ted, a simple text editor/IDE.

Copyright 2012, Blitz Research Ltd.

See LICENSE.TXT for licensing terms.
*/

#include "projecttreemodel.h"

ProjectTreeModel::ProjectTreeModel( QObject *parent ):QFileSystemModel( parent ),_current(-1){
}

bool ProjectTreeModel::addProject( const QString &dir ){

    if( dir.isEmpty() ) return false;

    QString sdir=dir.endsWith( '/' ) ? dir : dir+'/';
    for( int i=0;i<_dirs.size();++i ){
        if( _dirs[i].startsWith( sdir  ) ) return false;
        QString idir=_dirs[i].endsWith( '/' ) ? _dirs[i] : _dirs[i]+'/';
        if( dir.startsWith( idir ) ) return false;
    }

    QFileSystemModel::setRootPath( "" );

    QModelIndex index=QFileSystemModel::index( dir );
    if( !index.isValid() ) return false;

    QFileSystemModel::beginInsertRows( QModelIndex(),_projs.size(),_projs.size() );

    _dirs.push_back( dir );
    _projs.push_back( index );

    QFileSystemModel::endInsertRows();

//    _current=_projs.size()-1;

    return true;
}

void ProjectTreeModel::removeProject( const QString &dir ){

    for( int i=0;i<_dirs.size();++i ){
        if( dir==_dirs[i] ){
            QFileSystemModel::beginRemoveRows( QModelIndex(),_dirs.size()-1,_dirs.size()-1 );
            _dirs.remove( i );
            _projs.remove( i );
            QFileSystemModel::endRemoveRows();
            return;
        }
    }
}

bool ProjectTreeModel::isProject( const QModelIndex &index ){
    for( int i=0;i<_projs.size();++i ){
        if( index==_projs[i] ) return true;
    }
    return false;
}

bool ProjectTreeModel::hasChildren ( const QModelIndex &parent ) const{

    if( !parent.isValid() ) return _projs.size()>0;

    return QFileSystemModel::hasChildren( parent );
}

int	ProjectTreeModel::rowCount( const QModelIndex &parent )const{

    if( !parent.isValid() ) return _projs.size();

    return QFileSystemModel::rowCount( parent );
}

QModelIndex	ProjectTreeModel::index ( int row,int column,const QModelIndex &parent ) const{

    if( !parent.isValid() ){
        if( row>=0 && row<_projs.size() ) return _projs[row];
        return QModelIndex();
    }

    return QFileSystemModel::index( row,column,parent );
}

QVariant ProjectTreeModel::data( const QModelIndex &index,int role)const{

    if( role==Qt::FontRole && _current!=-1 && index==_projs[_current]){
        QFont font=QFileSystemModel::data( index,role ).value<QFont>();
        font.setBold( true );
        return font;
    }
    if (role == Qt::DecorationRole)
    {
        QString txtfilename = fileName(index);
        txtfilename = txtfilename.right(5);

        //qDebug() << txtfilename;
        //qDebug() << type(index);

        if(type(index).endsWith("Folder") && !txtfilename.endsWith(".data"))
        {
            return QPixmap(":/icons/circle/tree/folder.png");
        }
        if(type(index).endsWith("monkey File"))
        {
            return QPixmap(":/icons/circle/tree/monkey.png");
        }
        if(type(index).endsWith("png File"))
        {
            return QPixmap(":/icons/circle/tree/image.png");
        }
        if(type(index).endsWith("fnt File"))
        {
            return QPixmap(":/icons/circle/tree/font.png");
        }
        if(type(index).endsWith("txt File"))
        {
            return QPixmap(":/icons/circle/tree/text.png");
        }
        if(txtfilename.endsWith(".data") && !type(index).endsWith("data File"))
        {
            //qDebug() << type(index);
            return QPixmap(":/icons/circle/tree/data.png");
        }

    }
    return QFileSystemModel::data( index,role );
}


