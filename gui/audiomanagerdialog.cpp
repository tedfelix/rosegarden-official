/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
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

#include <klocale.h>
#include <kmessagebox.h>

#include <qhbox.h>
#include <qvbuttongroup.h>
#include <qlistbox.h>
#include <qpainter.h>
#include <qpixmap.h>

#include "audiomanagerdialog.h"

namespace Rosegarden
{

AudioManagerDialog::AudioManagerDialog(QWidget *parent,
                                       AudioFileManager *aFM):
    KDialogBase(parent, "", false, i18n("Audio Manager Dialog"), Close),
    m_audioFileManager(aFM)
{
    QHBox *h = makeHBoxMainWidget();
    QVButtonGroup *v = new QVButtonGroup(h);

    if (m_audioFileManager == 0)
    {
        KMessageBox::sorry(this,
                           i18n("No Audio File Manager - internal error"));
        delete this;
    }

    m_addButton    = new QPushButton(i18n("Add"), v);
    m_deleteButton = new QPushButton(i18n("Delete"), v);
    m_playButton   = new QPushButton(i18n("Play"), v);
    m_fileList     = new QListBox(h);

    // connect Delete button
    connect(m_deleteButton, SIGNAL(released()), SLOT(slotDeleteSelected()));

    populateFileList();
}

// Scan the AudioFileManager and populate the m_fileList
//
void
AudioManagerDialog::populateFileList()
{
    std::vector<AudioFile*>::const_iterator it;
    QListBoxPixmap *listBoxPixmap;

    // create pixmap of given size
    QPixmap *audioPixmap = new QPixmap(80, 20);

    m_fileList->clear();

    if (m_audioFileManager->begin() == m_audioFileManager->end())
    {
        // turn off selection and report empty list
        m_fileList->insertItem(i18n("<no audio files>"));
        m_fileList->setSelectionMode(QListBox::NoSelection);
        return;
    }

    for (it = m_audioFileManager->begin();
         it != m_audioFileManager->end();
         it++)
    {
        // clear and paint on it
        audioPixmap->fill(Qt::blue);
        QPainter audioPainter(audioPixmap);

        // this inserts the list item at the same time as creating
        listBoxPixmap = new QListBoxPixmap(m_fileList,
                                           *audioPixmap,
                                           QString((*it)->getName().c_str()));
    }
}

AudioFile*
AudioManagerDialog::getCurrentSelection()
{
    // if nothing selected
    if (m_fileList->currentItem() == -1)
        return 0;

    int count = 0;

    std::vector<AudioFile*>::const_iterator it;
    for (it = m_audioFileManager->begin();
         it != m_audioFileManager->end();
         it++)
    {
        if (count++ == m_fileList->currentItem())
            return (*it);
    }

    return 0;
}

void
AudioManagerDialog::slotDeleteSelected()
{
    AudioFile *audioFile = getCurrentSelection();

    if (audioFile == 0)
        return;

    m_audioFileManager->removeFile(audioFile->getId());
    populateFileList();
}

void
AudioManagerDialog::slotPlayPreview()
{
}

void
AudioManagerDialog::slotAdd()
{
}


void
AudioManagerDialog::closeEvent(QCloseEvent *e)
{
    e->accept();
}

}

