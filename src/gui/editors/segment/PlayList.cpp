/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include <klocale.h>
// #include <kglobal.h>
// #include <kurl.h>

//#include <Q3DragObject>

#include "PlayList.h"
#include "PlayListView.h"
#include "PlayListViewItem.h"
#include "document/ConfigGroups.h"

#include <QLayout>
#include <QSettings>
#include <QFileDialog>
#include <QFrame>
#include <QPushButton>
#include <QString>
#include <QStringList>
#include <QWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QUrl>
#include <QMimeData>	// replaces Q3UriDrag and Q3DragObject



namespace Rosegarden
{

PlayList::PlayList
		(
		QWidget *parent, const char *name
		)
        : QWidget(parent, name),
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
//     vLayout->addWidget(m_barLayout);
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


    m_openButton ->setText(i18n("Add..."));
    m_playButton ->setText(i18n("Play"));
    m_moveUpButton ->setText(i18n("Move Up"));
    m_moveDownButton->setText(i18n("Move Down"));
    m_deleteButton ->setText(i18n("Delete"));
    m_clearButton ->setText(i18n("Clear"));

    connect(m_openButton, SIGNAL(clicked()),
            SLOT(slotOpenFiles()));

    connect(m_playButton, SIGNAL(clicked()),
            SLOT(slotPlay()));

    connect(m_deleteButton, SIGNAL(clicked()),
            SLOT(slotDeleteCurrent()));

    connect(m_clearButton, SIGNAL(clicked()),
            SLOT(slotClear()));

    connect(m_moveUpButton, SIGNAL(clicked()),
            SLOT(slotMoveUp()));

    connect(m_moveDownButton, SIGNAL(clicked()),
            SLOT(slotMoveDown()));

    connect(m_listView, SIGNAL(currentChanged(QTreeWidgetItem*)),
            SLOT(slotCurrentItemChanged(QTreeWidgetItem*)));

    connect(m_listView, SIGNAL(dropped(QDropEvent*, QTreeWidgetItem*)),
            SLOT(slotDropped(QDropEvent*, QTreeWidgetItem*)));

    restore();

    enableButtons(0);

}

PlayList::~PlayList()
{
    save();
}

void PlayList::slotOpenFiles()
{
	/*
    QUrl::List kurlList =
        KFileDialog::getOpenURLs(":ROSEGARDEN",
                                 "audio/x-rosegarden audio/x-midi audio/x-rosegarden21",
                                 this,
                                 i18n("Select one or more Rosegarden files"));

    QUrl::List::iterator it;
	*/
	
	QStringList files = QFileDialog::getOpenFileNames( this, i18n("Select one or more Rosegarden files"), QDir::currentPath(), 0, 0);
	
	QString fname;
	
//    for (it = kurlList.begin(); it != kurlList.end(); ++it) {
	for( int i=0; i< files.size(); i++ ){
		fname = files.at( i );
		new PlayListViewItem(m_listView, QUrl(fname) );
    }
	
    enableButtons( m_listView->currentItem() );
}

void
PlayList::slotDropped(QDropEvent *event, QTreeWidgetItem* after)
{
    QStringList uri;
	QMimeData md;
	
    // see if we can decode a URI.. if not, just ignore it
// 	if (QUriDrag::decode(event, uri)) {
	
// 	if( ! md.formats().isEmpty() ){		//list of formats supported
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

void PlayList::slotPlay()
{
    PlayListViewItem *currentIndex = dynamic_cast<PlayListViewItem*>( m_listView->currentItem() );

    if (currentIndex)
        emit play( currentIndex->getURL().toString() );
}

void PlayList::slotMoveUp()
{
// 	QTreeWidgetItem *currentIndex = m_listView->currentItem();
// 	QTreeWidgetItem *previousItem = m_listView->previousSibling( currentIndex );
	
	QTreeWidgetItem *currentIndex = m_listView->currentItem();
	QTreeWidgetItem *previousItem = m_listView->itemAbove( currentIndex );
	QTreeWidgetItem *ti;
	int ix = m_listView->indexOfTopLevelItem( currentIndex );
	
    if (previousItem){
//         previousItem->moveItem(currentIndex);
		ti = m_listView->takeTopLevelItem( ix );
		m_listView->insertTopLevelItem( ix -1, ti );
	}

    enableButtons(currentIndex);
}

void PlayList::slotMoveDown()
{
	/*
    QTreeWidgetItem *currentIndex = m_listView->currentItem();
    QTreeWidgetItem *nextItem = currentIndex->nextSibling();

    if (nextItem)
        currentIndex->moveItem(nextItem);
	*/
	
	QTreeWidgetItem *currentIndex = m_listView->currentItem();
	QTreeWidgetItem *nextItem = m_listView->itemBelow( currentIndex );
	QTreeWidgetItem *ti;
	int ix = m_listView->indexOfTopLevelItem( currentIndex );
	
	if (nextItem){
//         previousItem->moveItem(currentIndex);
		ti = m_listView->takeTopLevelItem( ix );
		m_listView->insertTopLevelItem( ix +1, ti );
	}

    enableButtons(currentIndex);
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

void PlayList::slotCurrentItemChanged(QTreeWidgetItem* currentIndex)
{
    enableButtons(currentIndex);
}

void PlayList::enableButtons(QTreeWidgetItem* currentIndex)
{
    bool enable = (currentIndex != 0);

    m_playButton->setEnabled(enable);
    m_deleteButton->setEnabled(enable);
	
	
	int cAll = m_listView->topLevelItemCount();
// 	int cCurr = m_listView->indexOfTopLevelItem( m_listView.currentItem() );
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

// 	m_clearButton->setEnabled( m_listView->childCount() > 0 );
	m_clearButton->setEnabled( cAll > 0 );
}

void PlayList::save()
{
    QStringList urlList;
	
    PlayListViewItem* item = dynamic_cast<PlayListViewItem*>( getListView()->topLevelItem(0) );

    while (item) {
        urlList << item->getURL().toString();
// 		item = dynamic_cast<PlayListViewItem*>( item->nextSibling() );
		item = dynamic_cast<PlayListViewItem*>( getListView()->itemBelow( item ) );
	}

    QSettings kc ; // was: KGlobal::config()
	
//     KConfigGroupSaver cs(kc, PlayListConfigGroup);	//&&&
	kc.beginGroup( PlayListConfigGroup );
    kc.setValue("Playlist Files", urlList);

//     getListView()->saveLayout(kc, PlayListConfigGroup);	//&&&
	kc.endGroup();
}

void PlayList::restore()
{
    QSettings kc ; // was: KGlobal::config()
//     getListView()->restoreLayout(kc, PlayListConfigGroup);	//&&&

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
#include "PlayList.moc"
