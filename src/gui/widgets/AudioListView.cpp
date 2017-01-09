/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[AudioListView]"

#include "AudioListView.h"

#include "misc/Debug.h"
#include "gui/widgets/AudioListItem.h"

#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QMimeData>
#include <QUrl>
#include <QDrag>
#include <QDropEvent>


namespace Rosegarden {


AudioListView::AudioListView(QWidget *parent, const char *name)
    : QTreeWidget(parent)
{
    if(name){
        setObjectName( name );
    }else{
        setObjectName( "AudioListView" );
    }
    setDragEnabled(false);  // start drag manually in mouseMoveEvent
    setAcceptDrops(true);
    setDropIndicatorShown(true);
    
}




void AudioListView::mouseMoveEvent(QMouseEvent *event){
    
    // 
    
    // if not left button - return
     if (!(event->buttons() & Qt::LeftButton)) return;
    
    // if no item selected, return (else it would crash)
     if (currentItem() == NULL) return;
     
     QTreeWidgetItem *item = currentItem();
     
     // we use the topLevelItems as drag-source. (they have the full file-path available)
     while( item && item->parent() ){
        item = dynamic_cast<QTreeWidgetItem*>( item->parent() );  // assign parent/topLevelItem
     }
     if( ! item ){
        RG_DEBUG << "AudioListView::mouseMoveEvent() - item is NULL (cast failed?) ";
        return;
     }
    
    QDrag *drag = new QDrag(this);
    QMimeData *mimeData = new QMimeData;
    
    // construct list of QUrls
    // other widgets accept this mime type, we can drop to them
    QList<QUrl> list;
    QString line;
    line = item->text(6);       // 6 == Column->filename / QTreeWidgetItem
    line = line.replace( "~", getenv( "HOME" ), Qt::CaseSensitive );
    QFileInfo finfo( line  );
    
    //TODO : allow multi-selection drags from AudioListView
    
    line = finfo.absoluteFilePath();
    // should we ?
//     if( ! line.startsWith( "file://" )){
//         line = line.insert( 0, "file://" );
//     }
    
    list.append( QUrl(line) ); // line is the filename.of the audio file
    
    // mime stuff
    mimeData->setUrls(list); 
    //mimeData->setData( line.toUtf8(), "text/uri-list" );
    
    
    // ----------------------------------------------------------------------
    // provide a plain:text type, for accellerated access, when draging internaly
    
    AudioListItem* AuItem = dynamic_cast<AudioListItem*>(currentItem());
    
    QString audioDatax;
    QTextStream ts(&audioDatax);
    ts << "AudioFileManager\n"
        << AuItem->getId() << '\n'
        << AuItem->getStartTime().sec << '\n'
        << AuItem->getStartTime().nsec << '\n'
        << AuItem->getDuration().sec << '\n'
        << AuItem->getDuration().nsec << '\n';
    ts.flush();
    
    RG_DEBUG << "AudioListView::dragObject - "
            << "file id = " << AuItem->getId()
            << ", start time = " << AuItem->getStartTime() << endl;
    
    mimeData->setText( audioDatax ); 
     // ----------------------------------------------------------------------
    
    
    drag->setMimeData(mimeData);
    
    RG_DEBUG << "Starting drag from AudioListView::mouseMoveEvent() with mime : " << mimeData->formats() << " - " << mimeData->urls()[0];
    
    // start drag
    drag->start(Qt::CopyAction | Qt::MoveAction);
    
    
}



QStringList AudioListView::mimeTypes() const{
    QStringList types;
    types << "text/uri-list";
    types << "text/plain";
    return types;
}



void AudioListView::dragEnterEvent(QDragEnterEvent *e){
    QStringList uriList;
    QString text;

    if (e->mimeData()->hasUrls() || e->mimeData()->hasText()) {

        if (uriList.empty() && text == "") {
            RG_DEBUG << "AudioListView::dragEnterEvent: Drop Empty ! ";
        }
        if (e->proposedAction() & Qt::CopyAction) {
            e->acceptProposedAction();
        } else {
            e->setDropAction(Qt::CopyAction);
            e->accept();
        }

    }

}


void AudioListView::dropEvent(QDropEvent* e)
{
    QList<QUrl> uList;
    QStringList uriList;
    QString text;

    if (e->mimeData()->hasUrls() || e->mimeData()->hasText()) {
        
        if( e->source() ){
            RG_DEBUG << "AudioListView::dropEvent() - objectName : " << e->source()->objectName();
        }
        
        // if (drag-source == this)  (or a child item) disallow drop
        if( e->source() && ((e->source() == this) || (e->source()->parent() && (e->source()->parent() == this )))){
            // don't accept dropped items inside the ListView
            // moving items not supported yet.
            return;
        }
        
        if (e->proposedAction() & Qt::CopyAction) {
            e->acceptProposedAction();
        } else {
            e->setDropAction(Qt::CopyAction);
            e->accept();
        }

        if (e->mimeData()->hasUrls()) {
            uList = e->mimeData()->urls();
            if (!uList.isEmpty()) {
                for (int i = 0; i < uList.size(); ++i)  {
                    uriList.append(QString::fromLocal8Bit(uList.value(i).toEncoded().data()));
               }
            }
        } else {  // text/plain
            text = e->mimeData()->text();
        }
    } else {
        e->ignore();
        RG_DEBUG << "AudioListView::dropEvent: ignored dropEvent (invalid mime) ";
        return;
    }

    if (uriList.empty() && text == "") {
        RG_DEBUG << "AudioListView::dropEvent: Nothing dropped";
        return;
    }
    
    RG_DEBUG << "AudioListView::dropEvent() - Dropped this: \n " << uriList;
    
    
    if( text != "" ){
        uriList << text;
    }
    
    emit dropped(e, dynamic_cast<QTreeWidget*>(this), uriList);
    // send to AudioManagerDialog::slotDropped()
    
    // signal: dropped(QDropEvent*, QTreeWidgetItem*)
}


}


