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
#include <kfiledialog.h>

#include <qhbox.h>
#include <qvbuttongroup.h>
#include <qlistbox.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qinputdialog.h>

#include "WAVAudioFile.h"
#include "audiomanagerdialog.h"

using std::cout;
using std::cerr;
using std::endl;

namespace Rosegarden
{

static const int maxPreviewWidth = 100;
static const int previewHeight = 40;

AudioManagerDialog::AudioManagerDialog(QWidget *parent,
                                       AudioFileManager *aFM):
    KDialogBase(parent, "", false,
                i18n("Rosegarden Audio File Manager"), Close),
    m_audioFileManager(aFM)
{
    QHBox *h = makeHBoxMainWidget();
    QVButtonGroup *v = new QVButtonGroup(i18n("Audio File actions"), h);

    if (m_audioFileManager == 0)
    {
        KMessageBox::sorry(this,
                           i18n("No Audio File Manager - internal error"));
        delete this;
    }

    // create widgets
    m_addButton    = new QPushButton(i18n("Add File"), v);
    m_deleteButton = new QPushButton(i18n("Delete File"), v);
    m_playButton   = new QPushButton(i18n("Play File"), v);
    m_renameButton = new QPushButton(i18n("Rename File"), v);
    m_fileList     = new QListBox(h);

    // a minimum width for the list box
    m_fileList->setMinimumWidth(300);

    // connect buttons
    connect(m_deleteButton, SIGNAL(released()), SLOT(slotDeleteSelected()));
    connect(m_addButton, SIGNAL(released()), SLOT(slotAdd()));
    connect(m_playButton, SIGNAL(released()), SLOT(slotPlayPreview()));
    connect(m_renameButton, SIGNAL(released()), SLOT(slotRenameSelected()));

    // connect selection mechanism
    connect(m_fileList, SIGNAL(selectionChanged()), SLOT(slotEnableButtons()));

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
    QPixmap *audioPixmap = new QPixmap(maxPreviewWidth, previewHeight);

    // clear file list and disable associated action buttons
    m_fileList->clear();
    m_deleteButton->setDisabled(true);
    m_playButton->setDisabled(true);
    m_renameButton->setDisabled(true);

    if (m_audioFileManager->begin() == m_audioFileManager->end())
    {
        // turn off selection and report empty list
        m_fileList->insertItem(i18n("<no audio files>"));
        m_fileList->setSelectionMode(QListBox::NoSelection);
        return;
    }

    // enable selection
    m_fileList->setSelectionMode(QListBox::Single);

    // for the sample file length
    QString usecs;
    RealTime length;

    for (it = m_audioFileManager->begin();
         it != m_audioFileManager->end();
         it++)
    {
        generateEnvelopePixmap(audioPixmap, *it);

        length = (*it)->getLength();
        usecs.sprintf("%6d", length.usec);

        QString label = QString((*it)->getShortFilename().c_str()) + " (" +
                        //QString((*it)->getName().c_str()) + ") - " + 
                        QString("%1.%2 s").arg(length.sec)
                                          .arg(usecs) + ")";

             
        // this inserts the list item at the same time as creating
        listBoxPixmap = new QListBoxPixmap(m_fileList,
                                           *audioPixmap,
                                           label);
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

    QString question = i18n("Really delete \"") +
                       QString(audioFile->getFilename().c_str()) +
                       QString("\" ?");

    // Ask the question
    int reply = KMessageBox::questionYesNo(this, question);

    if (reply != KMessageBox::Yes)
        return;

    unsigned int id = audioFile->getId();
    m_audioFileManager->removeFile(id);
    populateFileList();

    // tell the sequencer
    emit deleteAudioFile(id);
}

void
AudioManagerDialog::slotPlayPreview()
{
    AudioFile *audioFile = getCurrentSelection();

    if (audioFile == 0)
        return;

    // tell the sequencer
    emit playAudioFile(audioFile->getId());
}

// Add a file to the audio file manager - allow previews and
// enforce file types.
//
void
AudioManagerDialog::slotAdd()
{
    unsigned int id = 0;

    KFileDialog *fileDialog =
        new KFileDialog(QString(m_audioFileManager->getLastAddPath().c_str()),
                        QString(i18n("WAV files (*.wav)")),
                        this, i18n("Select an Audio File"), true);

    if (fileDialog->exec() == QDialog::Accepted)
    {
        QString newFilePath = fileDialog->selectedFile().data();
        unsigned int id;

        // Now set the "last add" path so that next time we use "file add"
        // we start looking in the same place.
        //
        std::string newLastAddPath =
            m_audioFileManager->getDirectory(std::string(newFilePath.data()));
        m_audioFileManager->setLastAddPath(newLastAddPath);

        try
        {
            id = m_audioFileManager->addFile(std::string(newFilePath.data()));
        }
        catch(std::string e)
        {
            QString errorString =
                i18n("Can't add File.  WAV file body invalid.\n\"") +
                                  QString(e.c_str()) + "\"";
            KMessageBox::sorry(this, errorString);
        }

        populateFileList();

    }

    delete fileDialog;

    // tell the sequencer
    emit addAudioFile(id);
}

// Enable these action buttons
void
AudioManagerDialog::slotEnableButtons()
{
    m_deleteButton->setDisabled(false);
    m_playButton->setDisabled(false);
    m_renameButton->setDisabled(false);
}

void
AudioManagerDialog::slotRenameSelected()
{
    AudioFile *audioFile = getCurrentSelection();

    if (audioFile == 0)
        return;

    bool ok = false;
#ifdef RGKDE3
    QString newText = QInputDialog::getText(
                                 QString("Change Audio File label"),
                                 QString("Enter new label"),
                                 QLineEdit::Normal,
                                 QString(audioFile->getName().c_str()),
                                 &ok,
                                 this);
#else
    QString newText = QInputDialog::getText(
                                 QString("Change Audio File label"),
                                 QString("Enter new label"),
                                 QString(audioFile->getName().c_str()),
                                 &ok,
                                 this);
#endif

    if ( ok && !newText.isEmpty() )
        audioFile->setName(std::string(newText));

    populateFileList();
}

void
AudioManagerDialog::closeEvent(QCloseEvent *e)
{
    e->accept();
}

void
AudioManagerDialog::generateEnvelopePixmap(QPixmap *pixmap, AudioFile *aF)
{
    //std::cout << "SAMPLE LENGTH = " << aF->getLength() << std::endl;
    // clear and paint on it
    /*
    pixmap->fill(Qt::blue);
    QPainter audioPainter(pixmap);
    */

    /*
    std::vector<float> values = m_audioFileManager->getPreview(
            dynamic_cast<Rosegarden::WAVAudioFile*>(aF)->getId(),
            RealTime(0, 0),
            dynamic_cast<Rosegarden::WAVAudioFile*>(aF)->getLength(),
            previewWidth);
            */

    // Find the max length of all the sample files 
    //
    if (m_maxLength == RealTime(0, 0))
    {
        std::vector<AudioFile*>::const_iterator it;
        for (it = m_audioFileManager->begin();
             it != m_audioFileManager->end();
             it++)
        {
            if ((*it)->getLength() > m_maxLength)
                m_maxLength = (*it)->getLength();
        }
    }

    // Resize the pixmap according
    //
    pixmap->resize(int(double(maxPreviewWidth) *
                       (aF->getLength() / m_maxLength)),
                   pixmap->height());

    m_audioFileManager->
        drawPreview(dynamic_cast<Rosegarden::WAVAudioFile*>(aF)->getId(),
                    RealTime(0, 0),
                    dynamic_cast<Rosegarden::WAVAudioFile*>(aF)->getLength(),
                    pixmap);

}


}

