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

#include <qpushbutton.h>
#include <qaccel.h>
#include <qtooltip.h>
#include <qdragobject.h>
#include <qhbox.h>
#include <qvbuttongroup.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qinputdialog.h>

#include <kurl.h>
#include <kapplication.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kfiledialog.h>
#include <klistview.h>
#include <kio/netaccess.h>

#include "WAVAudioFile.h"
#include "audiomanagerdialog.h"
#include "widgets.h"
#include "dialogs.h"
#include "Progress.h"
#include "multiviewcommandhistory.h"

#include "rosedebug.h"

using std::cout;
using std::cerr;
using std::endl;

namespace Rosegarden
{

// Add an Id to a QListViewItem
//
class AudioListItem : public QListViewItem
{

public:

    AudioListItem(QListView *parent):QListViewItem(parent),
                                     m_segment(0) {;}

    AudioListItem(QListViewItem *parent):QListViewItem(parent),
                                         m_segment(0) {;}

    AudioListItem(QListView *parent,
                  QString label,
                  Rosegarden::AudioFileId id):
                      QListViewItem(parent,
                                    label,
                                    "", "", "", "", "", "", ""),
                                    m_id(id),
                                    m_segment(0) {;}

    AudioListItem(QListViewItem *parent, 
                  QString label,
                  Rosegarden::AudioFileId id):
                      QListViewItem(parent,
                                    label,
                                    "", "", "", "", "", "", ""),
                                    m_id(id),
                                    m_segment(0) {;}


    Rosegarden::AudioFileId getId() { return m_id; }

    void setStartTime(const Rosegarden::RealTime &time)
        { m_startTime = time; }
    Rosegarden::RealTime getStartTime() { return m_startTime; }

    void setDuration(const Rosegarden::RealTime &time)
        { m_duration = time; }
    Rosegarden::RealTime getDuration() { return m_duration; }

    void setSegment(Rosegarden::Segment *segment)
        { m_segment = segment; }
    Rosegarden::Segment *getSegment() { return m_segment; }

protected:
    Rosegarden::AudioFileId m_id;

    // for audio segments
    Rosegarden::RealTime m_startTime;
    Rosegarden::RealTime m_duration;

    // pointer to a segment
    Rosegarden::Segment *m_segment;

};

//---------------------------------------------

class AudioListView : public KListView
{
public:
    AudioListView(QWidget *parent = 0, const char *name = 0);

protected:
    virtual QDragObject* dragObject ();
};

AudioListView::AudioListView(QWidget *parent, const char *name)
    : KListView(parent, name)
{
    setDragEnabled(true);
}

QDragObject* AudioListView::dragObject()
{
    AudioListItem* item = dynamic_cast<AudioListItem*>(currentItem());

    QString audioData;
    QTextOStream ts(&audioData);
    ts << item->getId() << '\n'
       << item->getStartTime().sec << '\n'
       << item->getStartTime().usec << '\n'
       << item->getDuration().sec << '\n'
       << item->getDuration().usec << '\n';
    
    return new QTextDrag(audioData, this);
}

//---------------------------------------------

const int AudioManagerDialog::m_maxPreviewWidth            = 100;
const int AudioManagerDialog::m_previewHeight              = 30;
const char* const AudioManagerDialog::m_listViewLayoutName = "AudioManagerDialog Layout";

AudioManagerDialog::AudioManagerDialog(QWidget *parent,
                                       RosegardenGUIDoc *doc):
    KDialogBase(parent, "", false,
                i18n("Rosegarden Audio File Manager"), Close),
    m_doc(doc),
    m_playingAudioFile(0),
    m_audioPlayingDialog(0),
    m_audiblePreview(true)
{
    // accept dnd
    setAcceptDrops(true);

    QHBox *h = makeHBoxMainWidget();
    QVButtonGroup *v = new QVButtonGroup(i18n("Audio File actions"), h);

    if (m_doc == 0)
    {
        KMessageBox::sorry(this,
                           i18n("No RosegardenGUIDoc - internal error"));
        delete this;
    }

    // create widgets
    m_addButton       = new QPushButton(i18n("Add Audio File"), v);
    m_deleteButton    = new QPushButton(i18n("Remove Audio FIle"), v);
    m_playButton      = new QPushButton(i18n("Play Preview"), v);
    m_renameButton    = new QPushButton(i18n("Rename File"), v);
    m_insertButton    = new QPushButton(i18n("Insert into Composition"), v);
    m_deleteAllButton = new QPushButton(i18n("Delete All Audio Files"), v);
    m_fileList        = new AudioListView(h);

    QToolTip::add(m_fileList, i18n("Drag'n Drop .wav files here"));

    // Set the column names
    //
    m_fileList->addColumn(i18n("Name"));
    m_fileList->addColumn(i18n("Duration"));
    m_fileList->addColumn(i18n("Envelope"));
    m_fileList->addColumn(i18n("File"));
    m_fileList->addColumn(i18n("Resolution"));
    m_fileList->addColumn(i18n("Channels"));
    m_fileList->addColumn(i18n("Sample rate"));

    m_fileList->setColumnAlignment(1, Qt::AlignHCenter);
    m_fileList->setColumnAlignment(2, Qt::AlignHCenter);
    m_fileList->setColumnAlignment(4, Qt::AlignHCenter);
    m_fileList->setColumnAlignment(5, Qt::AlignHCenter);
    m_fileList->setColumnAlignment(6, Qt::AlignHCenter);

#ifdef RGKDE3
    m_fileList->restoreLayout(kapp->config(), m_listViewLayoutName);
#endif

    // a minimum width for the list box
    //m_fileList->setMinimumWidth(300);

    // show focus across all columns
    m_fileList->setAllColumnsShowFocus(true);

    // show tooltips when columns are partially hidden
#ifdef RGKDE3
    m_fileList->setShowToolTips(true);
#endif

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
    m_accelerators = new QAccel(this);

    // delete
    //
    m_accelerators->connectItem(m_accelerators->insertItem(Key_Delete),
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
#ifdef RGKDE3
    m_fileList->saveLayout(kapp->config(), m_listViewLayoutName);
#endif
}

// Scan the AudioFileManager and populate the m_fileList
//
void
AudioManagerDialog::slotPopulateFileList()
{

    // create pixmap of given size
    QPixmap *audioPixmap = new QPixmap(m_maxPreviewWidth, m_previewHeight);

    // Store last selected item if we have one
    //
    AudioListItem *selectedItem =
        dynamic_cast<AudioListItem*>(m_fileList->selectedItem());
    Rosegarden::AudioFileId lastId = 0;
    Rosegarden::Segment *lastSegment = 0;
    bool findSelection = false;

    if (selectedItem)
    {
        lastId = selectedItem->getId();
        lastSegment = selectedItem->getSegment();
        findSelection = true;
    }

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
    QString msecs, sRate;
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

        // Sample rate
        //
        sRate.sprintf("%.1f KHz", float((*it)->getSampleRate())/ 1000.0);
        item->setText(6, sRate);

        // Test audio file element for selection criteria
        //
        if (findSelection && lastSegment == 0 && lastId == (*it)->getId())
        {
            m_fileList->setSelected(item, true);
            findSelection = false;
        }

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

                if (findSelection && lastSegment == (*iit))
                {
                    m_fileList->setSelected(childItem, true);
                    findSelection = false;
                }

        // Add children
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
            setSelected(id, segment, true); // propagate

        // Do it - will force update
        //
        Rosegarden::SegmentSelection selection;
        selection.insert(item->getSegment());
        emit deleteSegments(selection);

        return;
    }

    QString question = i18n("Really delete audio file \"") +
                       QString(audioFile->getFilename().c_str()) +
                       QString("\"\n") +
                       i18n("and all associated audio segments?");

    // Ask the question
    int reply = KMessageBox::questionYesNo(this, question);

    if (reply != KMessageBox::Yes)
        return;

    // remove segments along with audio file
    //
    Rosegarden::AudioFileId id = audioFile->getId();
    m_doc->getAudioFileManager().removeFile(id);

    Rosegarden::SegmentSelection selection;
    Composition &comp = m_doc->getComposition();

    for (Composition::iterator it = comp.begin(); it != comp.end(); it++)
    {
        if ((*it)->getType() == Rosegarden::Segment::Audio &&
            (*it)->getAudioFileId() == id)
            selection.insert(*it);
    }
    // delete segments
    emit deleteSegments(selection);

    // tell the sequencer
    emit deleteAudioFile(id);

    // repopulate
    slotPopulateFileList();
}

void
AudioManagerDialog::slotPlayPreview()
{
    AudioFile *audioFile = getCurrentSelection();
    AudioListItem *item =
            dynamic_cast<AudioListItem*>(m_fileList->selectedItem());

    if (item == 0 || audioFile == 0) return;

    // store the audio file we're playing
    m_playingAudioFile = audioFile->getId();

    // tell the sequencer
    emit playAudioFile(audioFile->getId(),
                       item->getStartTime(),
                       item->getDuration());

    // now open up the playing dialog
    //
    m_audioPlayingDialog =
        new AudioPlayingDialog(this, QString(audioFile->getFilename().c_str()));

    // just execute
    //
    if (m_audioPlayingDialog->exec() == QDialog::Rejected)
        emit cancelPlayingAudioFile(m_playingAudioFile);

    m_audioPlayingDialog = 0;

}

// Add a file to the audio file manager - allow previews and
// enforce file types.
//
void
AudioManagerDialog::slotAdd()
{
    KURL::List kurlList = KFileDialog::getOpenURLs(":WAVS",
                                                   QString(i18n("*.wav|WAV files (*.wav)")),
                                                   this, i18n("Select one or more Audio Files"));

#ifdef RGKDE3
    KURL::List::iterator it;
#else
    KURL::List::Iterator it;
#endif

    for (it = kurlList.begin(); it != kurlList.end(); ++it)
        addFile(*it);
}

// Enable these action buttons
void
AudioManagerDialog::slotEnableButtons()
{
    m_deleteButton->setDisabled(false);
    m_renameButton->setDisabled(false);
    m_insertButton->setDisabled(false);
    m_deleteAllButton->setDisabled(false);

    if (m_audiblePreview)
        m_playButton->setDisabled(false);
    else
        m_playButton->setDisabled(true);

}

void
AudioManagerDialog::slotInsert()
{
    AudioFile *audioFile = getCurrentSelection();
    if (audioFile == 0)
        return;

    kdDebug(KDEBUG_AREA) << "AudioManagerDialog::slotInsert\n";

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
    if (instr == 0) {
        kdDebug(KDEBUG_AREA) << "AudioManagerDialog::slotInsert() instr = 0\n";
        return;
    }

    // find selected audio file and guess a track
    emit insertAudioSegment(audioFile->getId(),
                            instr->getId(),
                            Rosegarden::RealTime(0, 0),
                            audioFile->getLength());
}

void
AudioManagerDialog::slotDeleteAll()
{
    QString question =
        i18n("Really delete all audio files and associated segments?");

    int reply = KMessageBox::questionYesNo(this, question);

    if (reply != KMessageBox::Yes)
        return;

    Rosegarden::SegmentSelection selection;
    Composition &comp = m_doc->getComposition();

    for (Composition::iterator it = comp.begin(); it != comp.end(); it++)
    {
        if ((*it)->getType() == Rosegarden::Segment::Audio)
            selection.insert(*it);
    }
    // delete segments
    emit deleteSegments(selection);

    // and now the audio files
    emit deleteAllAudioFiles();

    // clear the file list
    m_fileList->clear();
    slotPopulateFileList();
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
    emit closeClicked();
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
        Rosegarden::SegmentSelection selection;
        selection.insert(aItem->getSegment());
        emit segmentsSelected(selection);
    }
}


// Select a Segment - traverse the tree and locate the correct child
// or parent.  We need both id and segment because of this duality of
// purpose.  Set segment to 0 for parent audio entries.
//
void
AudioManagerDialog::setSelected(Rosegarden::AudioFileId id,
                                Rosegarden::Segment *segment,
                                bool propagate)
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
                        
                        // Only propagate to segmentcanvas if asked to
                        if (propagate)
                        {
                            Rosegarden::SegmentSelection selection;
                            selection.insert(aItem->getSegment());
                            emit segmentsSelected(selection);
                        }

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
        setSelected(id, segment, true); // propagate
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
        // We don't propagate this segment setting to the canvas 
        // as we probably got called from there.
        //
        setSelected(segment->getAudioFileId(), segment, false);
    }
    else
    {
        m_fileList->clearSelection();
    }

}

void
AudioManagerDialog::slotCancelPlayingAudioFile()
{
    emit cancelPlayingAudioFile(m_playingAudioFile);
}


void
AudioManagerDialog::closePlayingDialog(Rosegarden::AudioFileId id)
{
    if (m_audioPlayingDialog && id == m_playingAudioFile)
    {
        delete m_audioPlayingDialog;
        m_audioPlayingDialog = 0;
    }

}

bool
AudioManagerDialog::addFile(const KURL& kurl)
{
    Rosegarden::AudioFileId id = 0;

    // Now set the "last add" path so that next time we use "file add"
    // we start looking in the same place.
    //
    RosegardenProgressDialog *progressDlg =
        new RosegardenProgressDialog(i18n("Generating audio preview..."),
                                     i18n("Cancel"),
                                     100,
                                     this);
    QString newFilePath;

    if (kurl.isLocalFile()) {

        newFilePath = kurl.path();

    } else { // download locally
        if (KIO::NetAccess::download(kurl, newFilePath) == false) {
            KMessageBox::error(this, QString(i18n("Cannot download file %1"))
                               .arg(kurl.prettyURL()));
            return false;
        }
    }

    try
    {
        id = m_doc->
            getAudioFileManager().addFile(std::string(newFilePath.data()));
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
        return false;
    }
    catch(QString e)
    {
        delete progressDlg;
        progressDlg = 0;
        KMessageBox::sorry(this, e);
        return false;
    }

    try
    {
        m_doc->getAudioFileManager().
            generatePreview(dynamic_cast<Progress*>(progressDlg), id);
    }
    catch(std::string e)
    {
        delete progressDlg;
        progressDlg = 0;
        KMessageBox::error(this, QString(e.c_str()));
        return false;
    }

    if (progressDlg) delete progressDlg;

    slotPopulateFileList();

    // tell the sequencer
    emit addAudioFile(id);

    return true;
}


void
AudioManagerDialog::dragEnterEvent(QDragEnterEvent *event)
{
    // accept uri drops only
    event->accept(QUriDrag::canDecode(event));
}

void
AudioManagerDialog::dropEvent(QDropEvent *event)
{
    // this is a very simplistic implementation of a drop event.  we
    // will only accept a dropped URL.  the Qt dnd code can do *much*
    // much more, so please read the docs there
    QStrList uri;

    // see if we can decode a URI.. if not, just ignore it
    if (QUriDrag::decode(event, uri))
    {
        // okay, we have a URI.. process it
        QString url, target;
        for(const char* url = uri.first(); url; url = uri.next()) {
            
            kdDebug(KDEBUG_AREA) << "AudioManagerDialog::dropEvent() : got "
                                 << url << endl;

            addFile(KURL(url));
        }
        
    }
}

void
AudioManagerDialog::setAudioSubsystemStatus(bool ok)
{
    // We can do something more fancy in the future but for the moment
    // this will suffice.
    //
    m_audiblePreview = ok;
}


}

