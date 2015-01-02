/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "PlayListViewItem.h"

#include <QFile>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QUrl>
#include <QMimeData>


namespace Rosegarden {

PlayListViewItem::PlayListViewItem(
                    QTreeWidget* parent, QUrl kurl
                    )
    : QTreeWidgetItem(
                    parent, QStringList() << QFile(kurl.toLocalFile()).fileName() << kurl.toString() 
                    ),
    m_kurl(kurl)
{
    // dont start drag on treeWidgetItem-Base, (but in TreeWidget)
//     setFlags( flags() & ! (Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled) );
}


PlayListViewItem::PlayListViewItem(QTreeWidget* parent, QTreeWidgetItem* after, QUrl kurl)
// 	: QTreeWidgetItem(parent, after, QStringList() << QFile(kurl.toLocalFile()).fileName() << kurl.toString() ),
//       m_kurl(kurl)
{
	QTreeWidgetItem *it = new QTreeWidgetItem( parent, QStringList() << QFile(kurl.toLocalFile()).fileName() << kurl.toString() );
	
	parent->insertTopLevelItem( parent->indexOfTopLevelItem(after)+1, it );
	
	this->m_kurl = kurl;
}


/*
void PlayListViewItem::mousePressEvent( QMouseEvent * event ){
    
}

void PlayListViewItem::mouseMoveEvent( QMouseEvent *event ){
    
}


QStringList PlayListViewItem::mimeTypes () const {
    QStringList types;
    types << "text/uri-list";
    return types;
}

QMimeData * PlayListViewItem::mimeData ( const QModelIndexList & indexes ) const{
    QMimeData *mime = new QMimeData;
    QList<QUrl> list;
    list << QUrl( text(0) );    // text of column 0
    mime->setUrls( list );
    return mime;
}
*/

}

