/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[PlayList]"

#include "misc/Debug.h"
#include "PlayList.h"
#include "PlayListView.h"
#include "PlayListViewItem.h"
#include "misc/ConfigGroups.h"
#include "gui/widgets/FileDialog.h"

#include <QLayout>
#include <QSettings>
#include <QFrame>
#include <QPushButton>
#include <QString>
#include <QStringList>
#include <QWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QUrl>
#include <QMimeData> 


namespace Rosegarden
{

PlayList::PlayList(QWidget *parent) : QWidget(parent),
        m_listView(new PlayListView(this)),
        m_buttonBar(new QFrame(this)),
        m_barLayout(new QHBoxLayout(m_buttonBar)),
        m_playButton(0),
        m_moveUpButton(0),
        m_moveDownButton(0),
        m_deleteButton(0),
        m_clearButton(0)
{
    QVBoxLayout *vLayout = new QVBoxLayout;
    vLayout->addWidget(m_listView);
    vLayout->addWidget(m_buttonBar);
    setLayout(vLayout);

    m_openButton = new QPushButton(m_buttonBar);
    m_playButton = new QPushButton(m_buttonBar);
    m_moveUpButton = new QPushButton(m_buttonBar);
    m_moveDownButton = new QPushButton(m_buttonBar);
    m_deleteButton = new QPushButton(m_buttonBar);
    m_clearButton = new QPushButton(m_buttonBar);
    m_barLayout->addWidget(m_openButton);
    m_barLayout->addWidget(m_playButton);
    m_barLayout->addWidget(m_moveUpButton);
    m_barLayout->addWidget(m_moveDownButton);
    m_barLayout->addWidget(m_deleteButton);
    m_barLayout->addWidget(m_clearButton);
    m_barLayout->addStretch();


    m_openButton ->setText(tr("Add..."));
    m_playButton ->setText(tr("Play"));
    m_moveUpButton ->setText(tr("Move Up"));
    m_moveDownButton->setText(tr("Move Down"));
    m_deleteButton ->setText(tr("Remove"));
    m_clearButton ->setText(tr("Clear whole List"));

    connect(m_openButton, SIGNAL(clicked()), SLOT(slotOpenFiles()));
    connect(m_playButton, SIGNAL(clicked()),SLOT(slotPlay()));
    connect(m_deleteButton, SIGNAL(clicked()), SLOT(slotDeleteCurrent()));
    connect(m_clearButton, SIGNAL(clicked()), SLOT(slotClear()));
    connect(m_moveUpButton, SIGNAL(clicked()), SLOT(slotMoveUp()));
    connect(m_moveDownButton, SIGNAL(clicked()), SLOT(slotMoveDown()));
    
    connect(m_listView, SIGNAL(droppedURIs(QDropEvent*, QTreeWidget*, QStringList)),
            SLOT(slotDroppedURIs(QDropEvent*, QTreeWidget*, QStringList)));
    
    connect(m_listView, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)),
                this, SLOT(slotCurrentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)));
    
//     connect(m_listView, SIGNAL(currentChanged(QTreeWidgetItem*)),
//             SLOT(slotCurrentItemChanged(QTreeWidgetItem*)));
//     connect(m_listView, SIGNAL(dropped(QDropEvent*, QTreeWidgetItem*)),
//             SLOT(slotDropped(QDropEvent*, QTreeWidgetItem*)));

    restore();

    enableButtons(0);

}

PlayList::~PlayList()
{
    save();
}

void PlayList::slotOpenFiles()
{
    QStringList files = FileDialog::getOpenFileNames( this, tr("Select one or more Rosegarden files"), QDir::currentPath(),
                        tr("Rosegarden files") + " (*.rg *.RG)" + ";;" +
                        tr("MIDI files") + " (*.mid *.midi *.MID *.MIDI)" + ";;" +
                        tr("X11 Rosegarden files") + " (*.rose)" + ";;" +
                        tr("All files") + " (*)", 0, 0 );
    
    QString fname;
    
    for( int i=0; i < files.size(); i++ ){
        fname = files.at( i );
        new PlayListViewItem(m_listView, QUrl(fname) );
    }
    
    enableButtons( m_listView->currentItem() );
}


void PlayList::slotDroppedURIs(QDropEvent* /* ev */, QTreeWidget* /* twid */, QStringList sl){
    // new
    for( int i=0; i<sl.count(); i++ ){
        new PlayListViewItem( m_listView, QUrl(sl[i]) );
        //new PlayListViewItem( m_listView, after, QUrl(sl[i]) );
    }
}


/*
void PlayList::slotDropped(QDropEvent *event, QTreeWidgetItem* after)
{
    // old. funct. - delete me later
    
    QStringList uri;
    QMimeData md;
    
    // see if we can decode a URI.. if not, just ignore it
//     if (QUriDrag::decode(event, uri)) {
    
//     if( ! md.formats().isEmpty() ){        //list of formats supported
    if( md.hasUrls() ){
        
        // okay, we have a URI.. process it
        // weed out non-rg files
        //
        QList<QUrl> urls = md.urls();
        QUrl url;
        
        for( int i=0; i< urls.count(); i++ ){
            url = urls.at( i );
            if (url.toString().right(3).toLower() == ".rg")
                new PlayListViewItem( m_listView, after, url );

        }
    }
    
    enableButtons( m_listView->currentItem() );
}
    */


void PlayList::slotPlay()
{
    QString fname;
    PlayListViewItem *item;
    item = dynamic_cast<PlayListViewItem*>( m_listView->currentItem() );
    
    RG_DEBUG << "PlayList::slotPlay() - called. ";
    if (item){
        fname = item->text(1); // 1==column1==filename
        emit play(fname);
//         emit play( item->getURL().toString() );
    }else{
        RG_DEBUG << "PlayList::slotPlay() - No item selected. ";
    }
}

void PlayList::slotMoveUp()
{
//     QTreeWidgetItem *currentIndex = m_listView->currentItem();
//     QTreeWidgetItem *previousItem = m_listView->previousSibling( currentIndex );
    
    QTreeWidgetItem *currentItem = m_listView->currentItem();
    QTreeWidgetItem *previousItem = m_listView->itemAbove( currentItem );
    QTreeWidgetItem *ti;
    int ix = m_listView->indexOfTopLevelItem( currentItem );
    
    if (previousItem){
//         previousItem->moveItem(currentIndex);
        ti = m_listView->takeTopLevelItem( ix );
        m_listView->insertTopLevelItem( ix -1, ti );
        
        m_listView->clearSelection();   // update selection
        m_listView->setCurrentItem( ti );
//         ti->setSelected( true );
    }
    
    enableButtons(currentItem);
}

void PlayList::slotMoveDown()
{
    /*
    QTreeWidgetItem *currentIndex = m_listView->currentItem();
    QTreeWidgetItem *nextItem = currentIndex->nextSibling();

    if (nextItem)
        currentIndex->moveItem(nextItem);
    */
    
    QTreeWidgetItem *currentItem = m_listView->currentItem();
    QTreeWidgetItem *nextItem = m_listView->itemBelow( currentItem );
    QTreeWidgetItem *ti;
    int ix = m_listView->indexOfTopLevelItem( currentItem );
    
    if (nextItem){
//         previousItem->moveItem(currentIndex);
        ti = m_listView->takeTopLevelItem( ix );
        m_listView->insertTopLevelItem( ix +1, ti );
        
        m_listView->clearSelection();   // update selection
        m_listView->setCurrentItem( ti );
//         ti->setSelected( true );
        
    }

    enableButtons(currentItem);
}

void PlayList::slotClear()
{
    m_listView->clear();
    enableButtons(0);
}

void PlayList::slotDeleteCurrent()
{
    QTreeWidgetItem* currentIndex = m_listView->currentItem();
    if (currentIndex)
        delete currentIndex;
}

void PlayList::slotCurrentItemChanged(QTreeWidgetItem* currentItem, QTreeWidgetItem* /* prevItem */)
{
//     RG_DEBUG << "PlayList::slotCurrentItemChanged() - selection Changed. ";
    enableButtons(currentItem);
}

void PlayList::enableButtons(QTreeWidgetItem* currentIndex)
{
    bool enable = (currentIndex != 0);

    m_playButton->setEnabled(enable);
    m_deleteButton->setEnabled(enable);
    
    
    int cAll = m_listView->topLevelItemCount();
//     int cCurr = m_listView->indexOfTopLevelItem( m_listView.currentItem() );
    int cCurr = m_listView->indexOfTopLevelItem( currentIndex );
    
    if (currentIndex) {
//         m_moveUpButton->setEnabled(currentIndex != m_listView->firstChild());
//         m_moveDownButton->setEnabled(currentIndex != m_listView->lastItem());
         m_moveUpButton->setEnabled( cCurr != 0 );
         m_moveDownButton->setEnabled( cCurr != cAll );
    } else {
        m_moveUpButton->setEnabled(false);
        m_moveDownButton->setEnabled(false);
    }

//     m_clearButton->setEnabled( m_listView->childCount() > 0 );
    m_clearButton->setEnabled( cAll > 0 );
}

void PlayList::save()
{
    QStringList urlList;
    
    PlayListViewItem* item = dynamic_cast<PlayListViewItem*>( getListView()->topLevelItem(0) );

    while (item) {
        urlList << item->getURL().toString();
//         item = dynamic_cast<PlayListViewItem*>( item->nextSibling() );
        item = dynamic_cast<PlayListViewItem*>( getListView()->itemBelow( item ) );
    }

    QSettings kc ; // was: KGlobal::config()
    
//     KConfigGroupSaver cs(kc, PlayListConfigGroup);    //&&&
    kc.beginGroup( PlayListConfigGroup );
    kc.setValue("Playlist Files", urlList);

//     getListView()->saveLayout(kc, PlayListConfigGroup);    //&&&
    kc.endGroup();
}

void PlayList::restore()
{
    QSettings kc ; // was: KGlobal::config()
//     getListView()->restoreLayout(kc, PlayListConfigGroup);    //&&&

//     KConfigGroupSaver cs(kc, PlayListConfigGroup);
    kc.beginGroup(PlayListConfigGroup);
    QStringList urlList = kc.value("Playlist Files").toStringList();
    QString ss;
    
    for( int i=0; i< urlList.count(); i ++ ){
        ss = urlList.at( i );
        new PlayListViewItem( getListView(), QUrl(ss) );
    }
    /*
    for (QStringList::Iterator it = urlList.begin();
            it != urlList.end(); ++it) {
        new PlayListViewItem(getListView(), QUrl(*it));
    }
    */
    kc.endGroup();
}


}
