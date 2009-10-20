/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2009 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "PlayListView.h"

#include "misc/Debug.h"


#include <QMimeData>	// replaces Q3DragObject and Q3UriDrag
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QDropEvent>
#include <QDrag>
#include <QUrl>


namespace Rosegarden {

PlayListView::PlayListView(QWidget *parent, const char *name)
    : QTreeWidget(parent)
{
	this->setObjectName( name );
	
//     addColumn(tr("Title"));
//     addColumn(tr("File name"));
	setColumnCount( 2 );
	setHeaderLabels( QStringList() << tr("Title") << tr("File name") );
    setAllColumnsShowFocus(true);
	
    setDropIndicatorShown(true);
    setDragEnabled(true);
    setAcceptDrops(true);
    //setDragDropMode( QAbstractItemView::NoDragDrop );
    //setDragDropMode(QAbstractItemView::InternalMove);
    
	
	/*
	setShowToolTips(true);		//&&& disabled a few property inits
	setShowSortIndicator(true);
	setItemsMovable(true);
    setSorting(-1);
	*/
}



/*
QMimeData* PlayListView::mimeData(const QList<QTreeWidgetItem *> items) const
{
    
    // Create a QByteArray to store the data for the drag.
    QByteArray ba;
    // Create a data stream that operates on the binary data
    QDataStream ds(&ba, QIODevice::WriteOnly);
    
    // Add each item's text for col 0 to the stream
    for (int i=0; i<items.size(); i++){
        ds << items.at(i)->text(0);
    }
    QMimeData *md = new QMimeData;
    // Set the data associated with the mime type foo/bar to ba 
    md->setData("text/uri-list", ba);
    return md;
}
*/


/*
void PlayListView::mousePressEvent ( QMouseEvent * event ){
    //
}
*/
void PlayListView::dragEnterEvent ( QDragEnterEvent * e ){
    
    if (e->provides("text/uri-list") || e->provides("text/plain")) {

        if (e->proposedAction() & Qt::CopyAction) {
            e->acceptProposedAction();
        } else {
            e->setDropAction(Qt::CopyAction);
            e->accept();
        }
    }
}


void PlayListView::mouseMoveEvent(QMouseEvent *event){

    // 
    
    // if not left button - return
     if (!(event->buttons() & Qt::LeftButton)) return;
    
    // if no item selected, return (else it would crash)
     if (currentItem() == NULL) return;
    
    QDrag *drag = new QDrag(this);
    QMimeData *mimeData = new QMimeData;
    
    // construct list of QUrls
    // other widgets accept this mime type, we can drop to them
    QList<QUrl> list;
    QString line;
    line = currentItem()->text(0); // 0 == first Column of QTreeWidgetItem
    list.append( QUrl(line) ); // only QUrl in list will be text of actual item
    
    // mime stuff
    mimeData->setUrls(list); 
    //mimeData->setData( line.toUtf8(), "text/uri-list" );
    drag->setMimeData(mimeData);
    
    RG_DEBUG << "Starting drag from PlayListView::mouseMoveEvent() with mime : " << mimeData->formats() << " - " << mimeData->urls()[0] << endl;
    
    // start drag
    drag->start(Qt::CopyAction | Qt::MoveAction);
    
    
}



QStringList PlayListView::mimeTypes() const{
    QStringList types;
    types << "text/uri-list";
    return types;
}



void PlayListView::dropEvent(QDropEvent* e)
{
    QStringList uriList;
    QString text;

    if (e->provides("text/uri-list") || e->provides("text/plain")) {

        if (e->proposedAction() & Qt::CopyAction) {
            e->acceptProposedAction();
        } else {
            e->setDropAction(Qt::CopyAction);
            e->accept();
        }

        if (e->provides("text/uri-list")) {
            uriList = QString::fromLocal8Bit(
                        e->encodedData("text/uri-list").data()
                    ).split( QRegExp("[\\r\\n]+"), QString::SkipEmptyParts );
        } else {
            text = QString::fromLocal8Bit(e->encodedData("text/plain").data());
        }
    } else {
        e->ignore();
        RG_DEBUG << "PlayListView::dropEvent: ignored dropEvent (invalid mime) " << endl;
        return;
    }

    if (uriList.empty() && text == "") {
        RG_DEBUG << "PlayListView::dropEvent: Nothing dropped" << endl;
        return;
    }
    if( text != "" ){
        uriList << text;
    }
    
    //### TODO test if QUrl is valid (?) valid extension, existent, supported file ?
    
    emit droppedURIs(e, dynamic_cast<QTreeWidget*>(this), uriList);
    
    // signal: dropped(QDropEvent*, QTreeWidgetItem*)
    // send to AudioManagerDialog::slotDropped()
}





QTreeWidgetItem* PlayListView::previousSibling(QTreeWidgetItem* item)
{
	return this->itemAbove( item );
	/*
    QTreeWidgetItem* prevSib = firstChild();

    while(prevSib && prevSib->nextSibling() != item)
        prevSib = prevSib->nextSibling();

    return prevSib;
	*/
}


}

#include "PlayListView.moc"