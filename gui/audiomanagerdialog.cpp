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
#include <qlistview.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qinputdialog.h>

#include "WAVAudioFile.h"
#include "audiomanagerdialog.h"
#include "widgets.h"
#include "Progress.h"
#include "multiviewcommandhistory.h"

using std::cout;
using std::cerr;
using std::endl;

namespace Rosegarden
{

static const int maxPreviewWidth = 100;
static const int previewHeight = 30;

AudioManagerDialog::AudioManagerDialog(QWidget *parent,
                                       RosegardenGUIDoc *doc):
    KDialogBase(parent, "", false,
                i18n("Rosegarden Audio File Manager"), Close),
    m_doc(doc)
{
    QHBox *h = makeHBoxMainWidget();
    QVButtonGroup *v = new QVButtonGroup(i18n("Audio File actions"), h);

    if (m_doc == 0)
    {
        KMessageBox::sorry(this,
                           i18n("No RosegardenGUIDoc - internal error"));
        delete this;
    }

    // create widgets
    m_addButton       = new QPushButton(i18n("Add"), v);
    m_deleteButton    = new QPushButton(i18n("Remove"), v);
    m_playButton      = new QPushButton(i18n("Play"), v);
    m_renameButton    = new QPushButton(i18n("Rename"), v);
    m_insertButton    = new QPushButton(i18n("Insert"), v);
    m_deleteAllButton = new QPushButton(i18n("Delete All"), v);
    m_fileList     = new QListView(h);

    // Set the column names
    //
    m_fileList->addColumn(i18n("Name"));
    m_fileList->addColumn(i18n("Duration"));
    m_fileList->addColumn(i18n("Envelope"));
    m_fileList->addColumn(i18n("File"));
    m_fileList->addColumn(i18n("Resolution"));
    m_fileList->addColumn(i18n("Channels"));

    m_fileList->setColumnAlignment(1, Qt::AlignHCenter);
    m_fileList->setColumnAlignment(2, Qt::AlignHCenter);
    m_fileList->setColumnAlignment(4, Qt::AlignHCenter);
    m_fileList->setColumnAlignment(5, Qt::AlignHCenter);

    // a minimum width for the list box
    //m_fileList->setMinimumWidth(300);

    // Show focus across all columns
    m_fileList->setAllColumnsShowFocus(true);


    // connect buttons
    connect(m_deleteButton, SIGNAL(released()), SLOT(slotDelete()));
    connect(m_addButton, SIGNAL(released()), SLOT(slotAdd()));
    connect(m_playButton, SIGNAL(released()), SLOT(slotPlayPreview()));
    connect(m_renameButton, SIGNAL(released()), SLOT(slotRename()));
    connect(m_insertButton, SIGNAL(released()), SLOT(slotInsert()));
    connect(m_deleteAllButton, SIGNAL(released()), SLOT(slotDeleteAll()));

    // connect selection mechanism
    connect(m_fileList, SIGNAL(selectionChanged(QListViewItem*)),
                        SLOT(slotSelectionChanged(QListViewItem*)));

    // setup local accelerators
    //
    m_accelerator = new QAccel(this);

    // delete
    //
    m_accelerator->connectItem(m_accelerator->insertItem(Key_Delete),
                               this,
                               SLOT(slotDelete()));

    slotPopulateFileList();

    // Connect command history for updates
    //
    connect(getCommandHistory(), SIGNAL(commandExecuted(KCommand *)),
                        this, SLOT(slotCommandExecuted(KCommand *)));
}

AudioManagerDialog::~AudioManagerDialog()
{
    delete m_accelerator;
}

// Scan the AudioFileManager and populate the m_fileList
//
void
AudioManagerDialog::slotPopulateFileList()
{

    // create pixmap of given size
    QPixmap *audioPixmap = new QPixmap(maxPreviewWidth, previewHeight);

    // clear file list and disable associated action buttons
    m_fileList->clear();

    m_deleteButton->setDisabled(true);
    m_playButton->setDisabled(true);
    m_renameButton->setDisabled(true);
    m_insertButton->setDisabled(true);
    m_deleteAllButton->setDisabled(true);

    if (m_doc->getAudioFileManager().begin() ==
            m_doc->getAudioFileManager().end())
    {
        // Turn off selection and report empty list
        //
        new AudioListItem(m_fileList, i18n("<no audio files>"), 0);
        m_fileList->setSelectionMode(QListView::NoSelection);
        m_fileList->setRootIsDecorated(false);

        return;
    }

    // we have at least one audio file
    m_deleteAllButton->setDisabled(false);

    // show tree hierarchy
    m_fileList->setRootIsDecorated(true);

    // enable selection
    m_fileList->setSelectionMode(QListView::Single);

    // for the sample file length
    QString msecs;
    RealTime length;

    // Create a vector of audio Segments only
    //
    std::vector<Rosegarden::Segment*> segments;
    std::vector<Rosegarden::Segment*>::const_iterator iit;

    for (Composition::iterator it = m_doc->getComposition().begin();
             it != m_doc->getComposition().end(); it++)
    {
        if ((*it)->getType() == Rosegarden::Segment::Audio)
            segments.push_back(*it);
    }

    // duration
    Rosegarden::RealTime segmentDuration;

    for (std::vector<AudioFile*>::const_iterator
             it = m_doc->getAudioFileManager().begin();
             it != m_doc->getAudioFileManager().end();
             it++)
    {
        m_doc->getAudioFileManager().
                drawPreview((*it)->getId(),
                            RealTime(0, 0),
                            (*it)->getLength(),
                            audioPixmap);


        QString label = QString((*it)->getShortFilename().c_str());
             
        // Set the label, duration, envelope pixmap and filename
        //
        AudioListItem *item = new AudioListItem(m_fileList, label,
                                                (*it)->getId());
        // Duration
        //
        length = (*it)->getLength();
        msecs.sprintf("%03ld", length.usec / 1000);
        item->setText(1, QString("%1.%2s").arg(length.sec).arg(msecs));

        // set start time and duration
        item->setStartTime(Rosegarden::RealTime(0, 0));
        item->setDuration(length);

        // Envelope pixmap
        //
        item->setPixmap(2, *audioPixmap);

        // File location
        //
        item->setText(3, QString(
                    m_doc->getAudioFileManager().
                        substituteHomeForTilde((*it)->getFilename()).c_str()));
                                       
        // Resolution
        //
        item->setText(4, QString("%1 bits").arg((*it)->getBitsPerSample()));

        // Channels
        //
        item->setText(5, QString("%1").arg((*it)->getChannels()));

        // Add children
        //
        for (iit = segments.begin(); iit != segments.end(); iit++)
        {
            if ((*iit)->getAudioFileId() == (*it)->getId())
            {
                AudioListItem *childItem =
                    new AudioListItem(item,
                                      QString((*iit)->getLabel().c_str()),
                                      (*it)->getId());
                segmentDuration = (*iit)->getAudioEndTime() -
                                  (*iit)->getAudioStartTime();

                // store the start time
                //
                childItem->setStartTime((*iit)->getAudioStartTime());
                childItem->setDuration(segmentDuration);

                // Write segment duration
                //
                msecs.sprintf("%03ld", segmentDuration.usec / 1000);
                childItem->setText(1, QString("%1.%2s")
                                          .arg(segmentDuration.sec)
                                          .arg(msecs));

                m_doc->getAudioFileManager().
                    drawHighlightedPreview((*it)->getId(),
                                           RealTime(0, 0),
                                           (*it)->getLength(),
                                           (*iit)->getAudioStartTime(),
                                           (*iit)->getAudioEndTime(),
                                           audioPixmap);

                // set pixmap
                //
                childItem->setPixmap(2, *audioPixmap);

                // set segment
                //
                childItem->setSegment(*iit);
            }
        }
    }


}

AudioFile*
AudioManagerDialog::getCurrentSelection()
{
    // try and get the selected item
    AudioListItem *item =
        dynamic_cast<AudioListItem*>(m_fileList->selectedItem());
    if (item == 0) return 0;

    std::vector<AudioFile*>::const_iterator it;

    for (it = m_doc->getAudioFileManager().begin();
         it != m_doc->getAudioFileManager().end();
         it++)
    {
        // If we match then return the valid AudioFile
        //
        if (item->getId() == (*it)->getId())
            return (*it);
    }

    return 0;
}

void
AudioManagerDialog::slotDelete()
{
    AudioFile *audioFile = getCurrentSelection();
    AudioListItem *item =
        dynamic_cast<AudioListItem*>(m_fileList->selectedItem());

    if (audioFile == 0 || item == 0)
        return;

    // If we're on a Segment then delete it at the Composition
    // and refresh the list.
    //
    if (item->getSegment())
    {
        // Get the next item to highlight
        //
        QListViewItem *newItem = item->itemBelow();
        
        // Or try above
        //
        if (newItem == 0) newItem = item->itemAbove();

        // Or the parent
        //
        if(newItem == 0) newItem = item->parent();

        // Get the id and segment of the next item so that we can
        // match against it
        //
        Rosegarden::AudioFileId id = 0;
        Rosegarden::Segment *segment = 0;
        AudioListItem *aItem = dynamic_cast<AudioListItem*>(newItem);
        
        if (aItem)
        {
            segment = aItem->getSegment();
            id = aItem->getId();
        }

        // Jump to new selection
        //
        if (newItem)
            setSelected(id, segment);

        // Do it - will force update
        //
        emit deleteSegment(item->getSegment());


        return;
    }

    QString question = i18n("Really delete \"") +
                       QString(audioFile->getFilename().c_str()) +
                       QString("\" ?");

    // Ask the question
    int reply = KMessageBox::questionYesNo(this, question);

    if (reply != KMessageBox::Yes)
        return;

    Rosegarden::AudioFileId id = audioFile->getId();
    m_doc->getAudioFileManager().removeFile(id);

    // tell the sequencer
    emit deleteAudioFile(id);
}

void
AudioManagerDialog::slotPlayPreview()
{
    AudioFile *audioFile = getCurrentSelection();
    AudioListItem *item =
            dynamic_cast<AudioListItem*>(m_fileList->selectedItem());

    if (item == 0 || audioFile == 0) return;

    // tell the sequencer
    emit playAudioFile(audioFile->getId(),
                       item->getStartTime(),
                       item->getDuration());
}

// Add a file to the audio file manager - allow previews and
// enforce file types.
//
void
AudioManagerDialog::slotAdd()
{
    Rosegarden::AudioFileId id = 0;

    KFileDialog *fileDialog =
        new KFileDialog(QString(m_doc->getAudioFileManager().getLastAddPath().c_str()),
                        QString(i18n("*.wav|WAV files (*.wav)")),
                        this, i18n("Select an Audio File"), true);

    if (fileDialog->exec() == QDialog::Accepted)
    {
        QString newFilePath = fileDialog->selectedFile().data();

        // Now set the "last add" path so that next time we use "file add"
        // we start looking in the same place.
        //
        std::string newLastAddPath =
            m_doc->getAudioFileManager().getDirectory(std::string(newFilePath.data()));
        m_doc->getAudioFileManager().setLastAddPath(newLastAddPath);

        RosegardenProgressDialog *progressDlg =
            new RosegardenProgressDialog(i18n("Generating audio preview..."),
                                         i18n("Cancel"),
                                         100,
                                         this);
        try
        {
            id = m_doc->getAudioFileManager().addFile(std::string(newFilePath.data()));
        }
        catch(std::string e)
        {
            // clear down progress dialog
            delete progressDlg;
            progressDlg = 0;

            QString errorString =
                i18n("Can't add File.  WAV file body invalid.\n\"") +
                                  QString(e.c_str()) + "\"";
            KMessageBox::sorry(this, errorString);
        }

        try
        {
            m_doc->getAudioFileManager().generatePreview(
                    dynamic_cast<Progress*>(progressDlg), id);
        }
        catch(std::string e)
        {
            delete progressDlg;
            progressDlg = 0;
            KMessageBox::error(this, QString(e.c_str()));
        }

        if (progressDlg) delete progressDlg;

        slotPopulateFileList();

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
    m_insertButton->setDisabled(false);
    m_deleteAllButton->setDisabled(false);
}

void
AudioManagerDialog::slotInsert()
{
    AudioFile *audioFile = getCurrentSelection();
    if (audioFile == 0)
        return;

    std::cout << "AudioManagerDialog::slotInsert" << std::endl;

    // Find an Audio Instrument and create a Track and insert
    // the audio file over given time parameters.

    Rosegarden::DeviceList *devices = m_doc->getStudio().getDevices();
    Rosegarden::DeviceListIterator it;
    Rosegarden::InstrumentList instruments;
    Rosegarden::InstrumentList::iterator iit;
    Rosegarden::Instrument *instr = 0;

    // Hmm, creative use of vectors and iterators there - *sigh*
    //
    for (it = devices->begin(); it != devices->end(); it++)
    {
        if ((*it)->getType() == Rosegarden::Device::Audio)
        {
            // Hmm, again..
            //
            instruments = (*it)->getAllInstruments();

            for (iit = instruments.begin(); iit != instruments.end(); iit++)
            {
                if (instr == 0)
                {
                    instr = (*iit);
                    break;
                }
                else
                    break;
            }
        }
    }

    // Ok, so we've got the first audio instrument
    //
    if (instr == 0)
        return;
    

    // find selected audio file and guess a track
    emit insertAudioSegment(audioFile->getId(),
                            0,
                            instr->getId(),
                            Rosegarden::RealTime(0, 0),
                            Rosegarden::RealTime(0, 0));
}

void
AudioManagerDialog::slotDeleteAll()
{
    std::cout << "AudioManagerDialog::slotDeleteAll" << std::endl;
}

void
AudioManagerDialog::slotRename()
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

    slotPopulateFileList();
}

void
AudioManagerDialog::closeEvent(QCloseEvent *e)
{
    e->accept();
}

void
AudioManagerDialog::slotSelectionChanged(QListViewItem *item)
{
    slotEnableButtons();

    AudioListItem *aItem = dynamic_cast<AudioListItem*>(item);

    // If we're on a segment then send a "select" signal
    // and enable appropriate buttons.
    //
    if (aItem && aItem->getSegment())
    {
        emit segmentSelected(aItem->getSegment());
    }
}


// Select a Segment - traverse the tree and locate the correct child
// or parent.  We need both id and segment because of this duality of
// purpose.  Set segment to 0 for parent audio entries.
//
void
AudioManagerDialog::setSelected(Rosegarden::AudioFileId id,
                                Rosegarden::Segment *segment)
{
    QListViewItem *it = m_fileList->firstChild();
    QListViewItem *chIt = 0;
    AudioListItem *aItem;

    while (it)
    {
        // If we're looking for a top level audio file
        if (segment == 0)
        {
            aItem = dynamic_cast<AudioListItem*>(it);

            if (aItem->getId() == id)
            {
                m_fileList->ensureItemVisible(it);
                m_fileList->setSelected(it, true);
                return;
            }
        }
        else // look for a child
        {
            if (it->childCount() > 0)
                chIt = it->firstChild();
        
            while (chIt)
            {
                aItem = dynamic_cast<AudioListItem*>(chIt);
    
                if (aItem)
                {
                    if (aItem->getId() == id && aItem->getSegment() == segment)
                    {
                        m_fileList->ensureItemVisible(chIt);
                        m_fileList->setSelected(chIt, true);
                        emit segmentSelected(segment);
                        return;
                    }
                }
                chIt = chIt->nextSibling();
            }
        }

        it = it->nextSibling();
    }

}

MultiViewCommandHistory*
AudioManagerDialog::getCommandHistory()
{
        return m_doc->getCommandHistory();
}

// May eventually predicate this on some subset of Segment only
// commands - but we may not want to or need to.
//
void
AudioManagerDialog::slotCommandExecuted(KCommand * /*command */)
{
    AudioListItem *item =
            dynamic_cast<AudioListItem*>(m_fileList->selectedItem());

    // repopulate
    slotPopulateFileList();

    if (item)
    {
        Rosegarden::AudioFileId id = item->getId();
        Rosegarden::Segment *segment = item->getSegment();

        // set selected
        setSelected(id, segment);
    }

}


// Pass in a set of Segment to select - we check for embedded audio
// segments and if we find exactly one we highlight it.  If we don't
// we unselect everything.
//
void
AudioManagerDialog::slotSegmentSelection(
        const Rosegarden::SegmentSelection &segments)
{
    Rosegarden::Segment *segment = 0;

    for (SegmentSelection::iterator it = segments.begin();
                                    it != segments.end(); it++)
    {
        if ((*it)->getType() == Rosegarden::Segment::Audio) 
        {
            // Only get one audio segment
            if (segment == 0)
                segment = *it;
            else
                segment = 0;
        }

    }

    if (segment)
    {
        setSelected(segment->getAudioFileId(), segment);
    }
    else
    {
        m_fileList->clearSelection();
    }

}


}

