// -*- c-basic-offset: 4 -*-
/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2004
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "playlist.h"

#include <qlabel.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qdragobject.h>

#include <klistview.h>
#include <kdialogbase.h>
#include <kguiitem.h>
#include <klocale.h>
#include <kfiledialog.h>
#include <kdeversion.h>

class PlayListViewItem : public KListViewItem
{
 public:
    PlayListViewItem(KListView* parent, KURL);
    PlayListViewItem(KListView* parent, QListViewItem*, KURL);

    const KURL& getURL() { return m_kurl; }

 protected:
    KURL m_kurl;
};


PlayListViewItem::PlayListViewItem(KListView* parent, KURL kurl)
    : KListViewItem(parent, kurl.fileName(), kurl.prettyURL()),
      m_kurl(kurl)
{
}

PlayListViewItem::PlayListViewItem(KListView* parent, QListViewItem* after, KURL kurl)
    : KListViewItem(parent, after, kurl.fileName(), kurl.prettyURL()),
      m_kurl(kurl)
{
}



class PlayListView : public KListView
{
public:
    PlayListView(QWidget *parent=0, const char *name=0);

    QListViewItem* previousSibling(QListViewItem*);

protected:
//     virtual void dragEnterEvent(QDragEnterEvent *event);
//     virtual void dropEvent(QDropEvent*);

//     virtual void dragEnterEvent(QDragEnterEvent*);
    virtual bool acceptDrag(QDropEvent*) const;

    
};

PlayListView::PlayListView(QWidget *parent, const char *name)
    : KListView(parent, name)
{
    addColumn(i18n("Title"));
    addColumn(i18n("File name"));

    setDragEnabled(true);
    setAcceptDrops(true);
    setDropVisualizer(true);

    setShowToolTips(true);
    setShowSortIndicator(true);
    setAllColumnsShowFocus(true);
    setItemsMovable(true);
    setSorting(-1);
}

bool PlayListView::acceptDrag(QDropEvent* e) const
{
    return QUriDrag::canDecode(e) || KListView::acceptDrag(e);
}


QListViewItem* PlayListView::previousSibling(QListViewItem* item)
{
    QListViewItem* prevSib = firstChild();

    while(prevSib && prevSib->nextSibling() != item)
        prevSib = prevSib->nextSibling();

    return prevSib;
}


//////////////////////////////////////////////////////////////////////

PlayList::PlayList(QWidget *parent, const char *name)
    : QVBox(parent, name),
      m_listView(new PlayListView(this)),
      m_buttonBar(new QFrame(this)),
      m_barLayout(new QHBoxLayout(m_buttonBar)),
      m_playButton(0),
      m_moveUpButton(0),
      m_moveDownButton(0),
      m_deleteButton(0),
      m_clearButton(0)
{
    m_openButton     = new QPushButton(m_buttonBar);
    m_playButton     = new QPushButton(m_buttonBar);
    m_moveUpButton   = new QPushButton(m_buttonBar);
    m_moveDownButton = new QPushButton(m_buttonBar);
    m_deleteButton   = new QPushButton(m_buttonBar);
    m_clearButton    = new QPushButton(m_buttonBar);
    m_barLayout->addWidget(m_openButton);
    m_barLayout->addWidget(m_playButton);
    m_barLayout->addWidget(m_moveUpButton);
    m_barLayout->addWidget(m_moveDownButton);
    m_barLayout->addWidget(m_deleteButton);
    m_barLayout->addWidget(m_clearButton);
    m_barLayout->addStretch();


    m_openButton    ->setText(i18n("Add..."));
    m_playButton    ->setText(i18n("Play"));
    m_moveUpButton  ->setText(i18n("Move Up"));
    m_moveDownButton->setText(i18n("Move Down"));
    m_deleteButton  ->setText(i18n("Delete"));
    m_clearButton   ->setText(i18n("Clear"));

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

    connect(m_listView, SIGNAL(currentChanged(QListViewItem*)),
            SLOT(slotCurrentItemChanged(QListViewItem*)));

    connect(m_listView, SIGNAL(dropped(QDropEvent*, QListViewItem*)),
            SLOT(slotDropped(QDropEvent*, QListViewItem*)));

    restore();

    enableButtons(0);

}

PlayList::~PlayList()
{
    save();
}

void PlayList::slotOpenFiles()
{
    KURL::List kurlList =
        KFileDialog::getOpenURLs(":ROSEGARDEN",
                                 "audio/x-rosegarden audio/x-midi audio/x-rosegarden21",
                                 this,
                                 i18n("Select one or more Rosegarden files"));

    KURL::List::iterator it;

    for (it = kurlList.begin(); it != kurlList.end(); ++it) {
        new PlayListViewItem(m_listView, *it);
    }

    enableButtons(m_listView->currentItem());
}

void
PlayList::slotDropped(QDropEvent *event, QListViewItem* after)
{
    QStrList uri;

    // see if we can decode a URI.. if not, just ignore it
    if (QUriDrag::decode(event, uri)) {

        // okay, we have a URI.. process it
        // weed out non-rg files
        //
        for(QString url = uri.first(); url; url = uri.next()) {
            if (url.right(3).lower() == ".rg")
                new PlayListViewItem(m_listView, after, KURL(url));
            
        }
    }

    enableButtons(m_listView->currentItem());
}

void PlayList::slotPlay()
{
    PlayListViewItem *currentItem = dynamic_cast<PlayListViewItem*>(m_listView->currentItem());

    if (currentItem)
        emit play(currentItem->getURL().url());
}

void PlayList::slotMoveUp()
{
    QListViewItem *currentItem = m_listView->currentItem();
    QListViewItem *previousItem = m_listView->previousSibling(currentItem);

    if (previousItem)
        previousItem->moveItem(currentItem);

    enableButtons(currentItem);
}

void PlayList::slotMoveDown()
{
    QListViewItem *currentItem = m_listView->currentItem();
    QListViewItem *nextItem = currentItem->nextSibling();

    if (nextItem)
        currentItem->moveItem(nextItem);

    enableButtons(currentItem);
}

void PlayList::slotClear()
{
    m_listView->clear();
    enableButtons(0);
}

void PlayList::slotDeleteCurrent()
{
    QListViewItem* currentItem = m_listView->currentItem();
    if (currentItem) delete currentItem;
}

void PlayList::slotCurrentItemChanged(QListViewItem* currentItem)
{
    enableButtons(currentItem);
}

void PlayList::enableButtons(QListViewItem* currentItem)
{
    bool enable = (currentItem != 0);

    m_playButton->setEnabled(enable);
    m_deleteButton->setEnabled(enable);

    if (currentItem) {
        m_moveUpButton->setEnabled(currentItem != m_listView->firstChild());
        m_moveDownButton->setEnabled(currentItem != m_listView->lastItem());
    } else {
        m_moveUpButton->setEnabled(false);
        m_moveDownButton->setEnabled(false);
    }

    m_clearButton->setEnabled(m_listView->childCount() > 0);
}

void PlayList::save()
{
    QStringList urlList;
    PlayListViewItem* item = dynamic_cast<PlayListViewItem*>(getListView()->firstChild());

    while (item) {
        urlList << item->getURL().url();
        item = dynamic_cast<PlayListViewItem*>(item->nextSibling());
    }

    KConfig *kc = KGlobal::config();
    KConfigGroupSaver cs(kc, PlayListConfigGroup);
    kc->writeEntry("Playlist Files", urlList);

    getListView()->saveLayout(kc, PlayListConfigGroup);
}

void PlayList::restore()
{
    KConfig *kc = KGlobal::config();
    getListView()->restoreLayout(kc, PlayListConfigGroup);

    KConfigGroupSaver cs(kc, PlayListConfigGroup);
    QStringList urlList = kc->readListEntry("Playlist Files");

    for(QStringList::Iterator it = urlList.begin();
        it != urlList.end(); ++it) {
        new PlayListViewItem(getListView(), KURL(*it));
    }
}

const char* const PlayList::PlayListConfigGroup = "PLAY_LIST";

//////////////////////////////////////////////////////////////////////

PlayListDialog::PlayListDialog(QString caption,
                               QWidget* parent, const char* name)
    : KDialogBase(parent, name, false, caption,
                  KDialogBase::Close, // standard buttons
                  KDialogBase::Close, // default button
                  true),
      m_playList(new PlayList(this))
{
    setWFlags(WDestructiveClose);
    setMainWidget(m_playList);
    restore();
}

void PlayListDialog::save()
{
    saveDialogSize(PlayList::PlayListConfigGroup);
}

void PlayListDialog::restore()
{
    setInitialSize(configDialogSize(PlayList::PlayListConfigGroup));
}

void PlayListDialog::closeEvent(QCloseEvent *e)
{
    save();
    emit closing();
    KDialogBase::closeEvent(e);
}

void PlayListDialog::slotClose()
{
    save();
    emit closing();
    KDialogBase::slotClose();
}
