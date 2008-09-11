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


#include "PlayList.h"
#include "PlayListView.h"
#include "PlayListViewItem.h"
#include "document/ConfigGroups.h"
#include <QLayout>

#include <klocale.h>
#include <QSettings>
#include <kfiledialog.h>
#include <kglobal.h>
#include <kurl.h>
#include <QFrame>
#include <QPushButton>
#include <QString>
#include <QStringList>
#include <QStringList>
#include <QWidget>
#include <QVBoxLayout>
#include <qdragobject.h>


namespace Rosegarden
{

PlayList::PlayList(QWidget *parent, const char *name)
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
    vLayout->addWidget(m_barLayout);
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

    connect(m_listView, SIGNAL(currentChanged(QListWidgetItem*)),
            SLOT(slotCurrentItemChanged(QListWidgetItem*)));

    connect(m_listView, SIGNAL(dropped(QDropEvent*, QListWidgetItem*)),
            SLOT(slotDropped(QDropEvent*, QListWidgetItem*)));

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

    enableButtons(m_listView->currentIndex());
}

void
PlayList::slotDropped(QDropEvent *event, QListWidgetItem* after)
{
    QStrList uri;

    // see if we can decode a URI.. if not, just ignore it
    if (QUriDrag::decode(event, uri)) {

        // okay, we have a URI.. process it
        // weed out non-rg files
        //
        for (QString url = uri.first(); url; url = uri.next()) {
            if (url.right(3).toLower() == ".rg")
                new PlayListViewItem(m_listView, after, KURL(url));

        }
    }

    enableButtons(m_listView->currentIndex());
}

void PlayList::slotPlay()
{
    PlayListViewItem *currentIndex = dynamic_cast<PlayListViewItem*>(m_listView->currentIndex());

    if (currentIndex)
        emit play(currentIndex->getURL().url());
}

void PlayList::slotMoveUp()
{
    QListWidgetItem *currentIndex = m_listView->currentIndex();
    QListWidgetItem *previousItem = m_listView->previousSibling(currentIndex);

    if (previousItem)
        previousItem->moveItem(currentIndex);

    enableButtons(currentIndex);
}

void PlayList::slotMoveDown()
{
    QListWidgetItem *currentIndex = m_listView->currentIndex();
    QListWidgetItem *nextItem = currentIndex->nextSibling();

    if (nextItem)
        currentIndex->moveItem(nextItem);

    enableButtons(currentIndex);
}

void PlayList::slotClear()
{
    m_listView->clear();
    enableButtons(0);
}

void PlayList::slotDeleteCurrent()
{
    QListWidgetItem* currentIndex = m_listView->currentIndex();
    if (currentIndex)
        delete currentIndex;
}

void PlayList::slotCurrentItemChanged(QListWidgetItem* currentIndex)
{
    enableButtons(currentIndex);
}

void PlayList::enableButtons(QListWidgetItem* currentIndex)
{
    bool enable = (currentIndex != 0);

    m_playButton->setEnabled(enable);
    m_deleteButton->setEnabled(enable);

    if (currentIndex) {
        m_moveUpButton->setEnabled(currentIndex != m_listView->firstChild());
        m_moveDownButton->setEnabled(currentIndex != m_listView->lastItem());
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

    QSettings kc ; // was: KGlobal::config()
    KConfigGroupSaver cs(kc, PlayListConfigGroup);
    kc.setValue("Playlist Files", urlList);

    getListView()->saveLayout(kc, PlayListConfigGroup);
}

void PlayList::restore()
{
    QSettings kc ; // was: KGlobal::config()
    getListView()->restoreLayout(kc, PlayListConfigGroup);

    KConfigGroupSaver cs(kc, PlayListConfigGroup);
    QStringList urlList = kc->readListEntry("Playlist Files");

    for (QStringList::Iterator it = urlList.begin();
            it != urlList.end(); ++it) {
        new PlayListViewItem(getListView(), KURL(*it));
    }
}

}
#include "PlayList.moc"
