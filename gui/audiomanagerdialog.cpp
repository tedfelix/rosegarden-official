// -*- c-basic-offset: 4 -*-
/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2005
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
#include <qtimer.h>

#include <kurl.h>
#include <kapplication.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kfiledialog.h>
#include <klineeditdlg.h>
#include <kstddirs.h>
#include <kglobal.h>
#include <klistview.h>
#include <kio/netaccess.h>
#include <kaction.h>
#include <kstdaction.h>
#include <kdialogbase.h>

#include "audiomanagerdialog.h"
#include "dialogs.h"
#include "widgets.h"
#include "multiviewcommandhistory.h"
#include "rosegardenguiview.h"
#include "rosestrings.h"
#include "rgapplication.h"
#include "rosedebug.h"

#include "WAVAudioFile.h"

using std::endl;


namespace Rosegarden
{

// Add an Id to a QListViewItem
//
class AudioListItem : public KListViewItem
{

public:

    AudioListItem(KListView *parent):KListViewItem(parent),
                                     m_segment(0) {;}

    AudioListItem(KListViewItem *parent):KListViewItem(parent),
                                         m_segment(0) {;}

    AudioListItem(KListView *parent,
                  QString label,
                  AudioFileId id):
                      KListViewItem(parent,
                                    label,
                                    "", "", "", "", "", "", ""),
                                    m_id(id),
                                    m_segment(0) {;}

    AudioListItem(KListViewItem *parent, 
                  QString label,
                  AudioFileId id):
                      KListViewItem(parent,
                                    label,
                                    "", "", "", "", "", "", ""),
                                    m_id(id),
                                    m_segment(0) {;}


    AudioFileId getId() { return m_id; }

    void setStartTime(const RealTime &time)
        { m_startTime = time; }
    RealTime getStartTime() { return m_startTime; }

    void setDuration(const RealTime &time)
        { m_duration = time; }
    RealTime getDuration() { return m_duration; }

    void setSegment(Segment *segment)
        { m_segment = segment; }
    Segment *getSegment() { return m_segment; }

protected:
    AudioFileId m_id;

    // for audio segments
    RealTime m_startTime;
    RealTime m_duration;

    // pointer to a segment
    Segment *m_segment;

};

//---------------------------------------------

class AudioListView : public KListView
{
public:
    AudioListView(QWidget *parent = 0, const char *name = 0);

protected:
    bool acceptDrag(QDropEvent* e) const;
    virtual QDragObject* dragObject();
};

AudioListView::AudioListView(QWidget *parent, const char *name)
    : KListView(parent, name)
{
    setDragEnabled(true);
    setAcceptDrops(true);
    setDropVisualizer(false);
}

bool AudioListView::acceptDrag(QDropEvent* e) const
{
    return QUriDrag::canDecode(e) || KListView::acceptDrag(e);
}

QDragObject* AudioListView::dragObject()
{
    AudioListItem* item = dynamic_cast<AudioListItem*>(currentItem());

    QString audioData;
    QTextOStream ts(&audioData);
    ts << "AudioFileManager\n"
       << item->getId() << '\n'
       << item->getStartTime().sec << '\n'
       << item->getStartTime().nsec << '\n'
       << item->getDuration().sec << '\n'
       << item->getDuration().nsec << '\n';

    RG_DEBUG << "AudioListView::dragObject - "
             << "file id = " << item->getId()
             << ", start time = " << item->getStartTime() << endl;
    
    return new QTextDrag(audioData, this);
}

//---------------------------------------------

const int AudioManagerDialog::m_maxPreviewWidth            = 100;
const int AudioManagerDialog::m_previewHeight              = 30;
const char* const AudioManagerDialog::m_listViewLayoutName = "AudioManagerDialog Layout";

AudioManagerDialog::AudioManagerDialog(QWidget *parent,
                                       RosegardenGUIDoc *doc):
    KMainWindow(parent, "audioManagerDialog"),
    m_doc(doc),
    m_playingAudioFile(0),
    m_audioPlayingDialog(0),
    m_playTimer(new QTimer(this)),
    m_audiblePreview(true)
{
    setCaption(i18n("Audio File Manager"));
    setWFlags(WDestructiveClose);

    QVBox *box = new QVBox(this);
    setCentralWidget(box);
    box->setMargin(10);
    box->setSpacing(5);

    m_sampleRate = 0;
    
    QCString replyType;
    QByteArray replyData;
    if (rgapp->sequencerCall("getSampleRate()", replyType, replyData)) {
        QDataStream streamIn(replyData, IO_ReadOnly);
        unsigned int result;
        streamIn >> result;
        m_sampleRate = result;
    }

    m_fileList = new AudioListView(box);

    m_wrongSampleRates = new QLabel(i18n("* Some audio files are encoded at a sample rate different from that of the JACK audio server.\nRosegarden will play them at the correct speed, but they will sound terrible.\nPlease consider resampling such files externally, or adjusting the sample rate of the JACK server."), box);
    m_wrongSampleRates->hide();

    QString pixmapDir = KGlobal::dirs()->findResource("appdata", "pixmaps/");
    QIconSet icon(QPixmap(pixmapDir + "/toolbar/transport-play.xpm"));

    new KAction(i18n("&Add Audio File"), "fileopen", 0, this,
		SLOT(slotAdd()), actionCollection(), "add_audio");

    new KAction(i18n("&Unload Audio File"), "editdelete", 0, this,
		SLOT(slotRemove()), 
		actionCollection(), "remove_audio");

    icon = QIconSet(QPixmap(pixmapDir + "/toolbar/transport-play.xpm"));
    new KAction(i18n("&Play Preview"), icon, 0, this,
		SLOT(slotPlayPreview()), 
		actionCollection(), "preview_audio");

/*!!! Not actually implemented -- this never worked right!
    new KAction(i18n("Re&label"), 0, 0, this,
		SLOT(slotRename()), 
		actionCollection(), "rename_audio");
*/

    icon = QIconSet(QPixmap(pixmapDir + "/toolbar/insert_audio_into_track.xpm"));
    new KAction(i18n("&Insert into Selected Audio Track"), 
		icon, 0, this, SLOT(slotInsert()), 
		actionCollection(), "insert_audio");

    new KAction(i18n("Unload &all Audio Files"), 0, 0, this,
		SLOT(slotRemoveAll()), 
		actionCollection(), "remove_all_audio");

    new KAction(i18n("Unload all &Unused Audio Files"), 0, 0, this,
		SLOT(slotRemoveAllUnused()), 
		actionCollection(), "remove_all_unused_audio");

    new KAction(i18n("&Delete Unused Audio Files..."), 0, 0, this,
		SLOT(slotDeleteUnused()), 
		actionCollection(), "delete_unused_audio");

    new KAction(i18n("&Export Audio File"), "fileexport", 0, this,
		SLOT(slotExportAudio()), 
		actionCollection(), "export_audio");

    new KAction(i18n("Distribute Audio on &MIDI"), 
		0, 0, this,
		SLOT(slotDistributeOnMidiSegment()), 
		actionCollection(), 
		"distribute_audio");

    // Set the column names
    //
    m_fileList->addColumn(i18n("Name"));           // 0
    m_fileList->addColumn(i18n("Duration"));       // 1
    m_fileList->addColumn(i18n("Envelope"));       // 2
    m_fileList->addColumn(i18n("Sample rate"));    // 3
    m_fileList->addColumn(i18n("Channels"));       // 4
    m_fileList->addColumn(i18n("Resolution"));     // 5
    m_fileList->addColumn(i18n("File"));           // 6

    m_fileList->setColumnAlignment(1, Qt::AlignHCenter);
    m_fileList->setColumnAlignment(2, Qt::AlignHCenter);
    m_fileList->setColumnAlignment(3, Qt::AlignHCenter);
    m_fileList->setColumnAlignment(4, Qt::AlignHCenter);
    m_fileList->setColumnAlignment(5, Qt::AlignHCenter);

    m_fileList->restoreLayout(kapp->config(), m_listViewLayoutName);

    // a minimum width for the list box
    //m_fileList->setMinimumWidth(300);

    // show focus across all columns
    m_fileList->setAllColumnsShowFocus(true);

    // show tooltips when columns are partially hidden
    m_fileList->setShowToolTips(true);

    // connect selection mechanism
    connect(m_fileList, SIGNAL(selectionChanged(QListViewItem*)),
                        SLOT(slotSelectionChanged(QListViewItem*)));

    connect(m_fileList, SIGNAL(dropped(QDropEvent*, QListViewItem*)),
            SLOT(slotDropped(QDropEvent*, QListViewItem*)));

    // setup local accelerators
    //
    m_accelerators = new QAccel(this);

    // delete
    //
    m_accelerators->connectItem(m_accelerators->insertItem(Key_Delete),
                                this,
                                SLOT(slotRemove()));

    slotPopulateFileList();

    // Connect command history for updates
    //
    connect(getCommandHistory(), SIGNAL(commandExecuted(KCommand *)),
                        this, SLOT(slotCommandExecuted(KCommand *)));

    //setInitialSize(configDialogSize(AudioManagerDialogConfigGroup));

    connect(m_playTimer, SIGNAL(timeout()), 
            this, SLOT(slotCancelPlayingAudio()));

    KStdAction::close(this,
                      SLOT(slotClose()),
                      actionCollection());

    createGUI("audiomanager.rc");

    updateActionState(false);
}

AudioManagerDialog::~AudioManagerDialog()
{
    RG_DEBUG << "\n*** AudioManagerDialog::~AudioManagerDialog\n" << endl;
    m_fileList->saveLayout(kapp->config(), m_listViewLayoutName);
    //saveDialogSize(AudioManagerDialogConfigGroup);
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
    AudioFileId lastId = 0;
    Segment *lastSegment = 0;
    bool findSelection = false;
    bool foundSelection = false;

    if (selectedItem)
    {
        lastId = selectedItem->getId();
        lastSegment = selectedItem->getSegment();
        findSelection = true;
    }

    // We don't want the selection changes to be propagated
    // to the main view
    //
    m_fileList->blockSignals(true);

    // clear file list and disable associated action buttons
    m_fileList->clear();

    if (m_doc->getAudioFileManager().begin() ==
            m_doc->getAudioFileManager().end())
    {
        // Turn off selection and report empty list
        //
        new AudioListItem(m_fileList, i18n("<no audio files>"), 0);
        m_fileList->setSelectionMode(QListView::NoSelection);
        m_fileList->setRootIsDecorated(false);

	m_fileList->blockSignals(false);
	updateActionState(false);
        return;
    }

    // show tree hierarchy
    m_fileList->setRootIsDecorated(true);

    // enable selection
    m_fileList->setSelectionMode(QListView::Single);

    // for the sample file length
    QString msecs, sRate;
    RealTime length;

    // Create a vector of audio Segments only
    //
    std::vector<Segment*> segments;
    std::vector<Segment*>::const_iterator iit;

    for (Composition::iterator it = m_doc->getComposition().begin();
             it != m_doc->getComposition().end(); ++it)
    {
        if ((*it)->getType() == Segment::Audio)
            segments.push_back(*it);
    }

    // duration
    RealTime segmentDuration;
    bool wrongSampleRates = false;

    for (std::vector<AudioFile*>::const_iterator
             it = m_doc->getAudioFileManager().begin();
             it != m_doc->getAudioFileManager().end();
             ++it)
    {
        try
        {
            m_doc->getAudioFileManager().
                    drawPreview((*it)->getId(),
                                RealTime::zeroTime,
                                (*it)->getLength(),
                                audioPixmap);
        }
        catch(std::string e)
        {
            audioPixmap->fill(); // white
            QPainter p(audioPixmap);
            p.setPen(Qt::black);
            p.drawText(10, m_previewHeight / 2, QString("<no preview>"));
        }

	//!!! Why isn't the label the label the user assigned to the file?
	// Why do we allow the user to assign a label at all, then?

        QString label = QString((*it)->getShortFilename().c_str());
             
        // Set the label, duration, envelope pixmap and filename
        //
        AudioListItem *item = new AudioListItem(m_fileList, label,
                                                (*it)->getId());
        // Duration
        //
        length = (*it)->getLength();
        msecs.sprintf("%03d", length.nsec / 1000000);
        item->setText(1, QString("%1.%2s").arg(length.sec).arg(msecs));

        // set start time and duration
        item->setStartTime(RealTime::zeroTime);
        item->setDuration(length);

        // Envelope pixmap
        //
        item->setPixmap(2, *audioPixmap);

        // File location
        //
        item->setText(6, QString(
                    m_doc->getAudioFileManager().
                        substituteHomeForTilde((*it)->getFilename()).c_str()));
                                       
        // Resolution
        //
        item->setText(5, QString("%1 bits").arg((*it)->getBitsPerSample()));

        // Channels
        //
        item->setText(4, QString("%1").arg((*it)->getChannels()));

        // Sample rate
        //
	if (m_sampleRate != 0 && (*it)->getSampleRate() != m_sampleRate) {
	    sRate.sprintf("%.1f KHz *", float((*it)->getSampleRate())/ 1000.0);
	    wrongSampleRates = true;
	} else {
	    sRate.sprintf("%.1f KHz", float((*it)->getSampleRate())/ 1000.0);
	}
        item->setText(3, sRate);

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
                msecs.sprintf("%03d", segmentDuration.nsec / 1000000);
                childItem->setText(1, QString("%1.%2s")
                                          .arg(segmentDuration.sec)
                                          .arg(msecs));

                try
                {
                    m_doc->getAudioFileManager().
                        drawHighlightedPreview((*it)->getId(),
                                               RealTime::zeroTime,
                                               (*it)->getLength(),
                                               (*iit)->getAudioStartTime(),
                                               (*iit)->getAudioEndTime(),
                                               audioPixmap);
                }
                catch(std::string e)
                {
                    // should already be set to "no file"
                }

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
		    foundSelection = true;
                }

        // Add children
            }
        }
    }

    updateActionState(foundSelection);

    if (wrongSampleRates) {
	m_wrongSampleRates->show();
    } else {
	m_wrongSampleRates->hide();
    }

    m_fileList->blockSignals(false);
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
         ++it)
    {
        // If we match then return the valid AudioFile
        //
        if (item->getId() == (*it)->getId())
            return (*it);
    }

    return 0;
}

void
AudioManagerDialog::slotExportAudio()
{
    WAVAudioFile *sourceFile 
        = dynamic_cast<WAVAudioFile*>(getCurrentSelection());
    
    AudioListItem *item =
        dynamic_cast<AudioListItem*>(m_fileList->selectedItem());
    
    Segment *segment = item->getSegment();

    QString saveFile =
        KFileDialog::getSaveFileName(":WAVS",
                                     i18n("*.wav|WAV files (*.wav)"),
                                     this, i18n("Choose a name to save this file as"));
    
    if (sourceFile == 0 || item == 0 || segment == 0 || saveFile.isEmpty())
        return;

    // Check for a dot extension and append ".wav" if not found
    //
    if (saveFile.contains(".") == 0)
        saveFile += ".wav";

    RosegardenProgressDialog progressDlg(i18n("Exporting audio file..."),
                                         100,
                                         this);

    progressDlg.progressBar()->setProgress(0);

    RealTime segmentDuration 
        = segment->getAudioEndTime() - segment->getAudioStartTime();
    
    WAVAudioFile *destFile 
        = new WAVAudioFile(qstrtostr(saveFile),
                           sourceFile->getChannels(),
                           sourceFile->getSampleRate(),
                           sourceFile->getBytesPerSecond(),
                           sourceFile->getBytesPerFrame(),
                           sourceFile->getBitsPerSample());
    
    if (sourceFile->open() == false) 
    {
        delete destFile;
        return;
    }
    
    destFile->write();

    sourceFile->scanTo(segment->getAudioStartTime());

    destFile->appendSamples
        (sourceFile->getSampleFrameSlice(segmentDuration));

    destFile->close();
    sourceFile->close();    
    delete destFile;

    progressDlg.progressBar()->setProgress(100);
}

void
AudioManagerDialog::slotRemove()
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
        AudioFileId id = 0;
        Segment *segment = 0;
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
        SegmentSelection selection;
        selection.insert(item->getSegment());
        emit deleteSegments(selection);

        return;
    }

    // remove segments along with audio file
    //
    AudioFileId id = audioFile->getId();
    SegmentSelection selection;
    Composition &comp = m_doc->getComposition();

    bool haveSegments = false;
    for (Composition::iterator it = comp.begin(); it != comp.end(); ++it)
    {
        if ((*it)->getType() == Segment::Audio &&
            (*it)->getAudioFileId() == id) {
	    haveSegments = true;
	    break;
	}
    }

    if (haveSegments) {

	QString question = i18n("This will unload audio file \"%1\" and remove all associated segments.  Are you sure?")
	    .arg(QString(audioFile->getFilename().c_str()));

	// Ask the question
	int reply = KMessageBox::warningContinueCancel(this, question);

	if (reply != KMessageBox::Continue)
	    return;
    }

    for (Composition::iterator it = comp.begin(); it != comp.end(); ++it)
    {
        if ((*it)->getType() == Segment::Audio &&
            (*it)->getAudioFileId() == id)
            selection.insert(*it);
    }
    emit deleteSegments(selection);

    m_doc->getAudioFileManager().removeFile(id);

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

    // Setup timer to pop down dialog after file has completed
    //
    int msecs = item->getDuration().sec * 1000 +
                item->getDuration().nsec / 1000000;
    m_playTimer->start(msecs, true); // single shot

    // just execute
    //
    if (m_audioPlayingDialog->exec() == QDialog::Rejected)
       emit cancelPlayingAudioFile(m_playingAudioFile);

    delete m_audioPlayingDialog;
    m_audioPlayingDialog = 0;

    m_playTimer->stop();

}

void
AudioManagerDialog::slotCancelPlayingAudio()
{
    //std::cout << "AudioManagerDialog::slotCancelPlayingAudio" << std::endl;
    if (m_audioPlayingDialog)
    {
        m_playTimer->stop();
        delete m_audioPlayingDialog;
        m_audioPlayingDialog = 0;
    }
}


// Add a file to the audio file manager - allow previews and
// enforce file types.
//
void
AudioManagerDialog::slotAdd()
{
    KURL::List kurlList =
        KFileDialog::getOpenURLs(":WAVS",
                                 i18n("*.wav|WAV files (*.wav)\n*.mp3|MP3 files (*.mp3)"),
                                 this, i18n("Select one or more audio files"));

    KURL::List::iterator it;

    for (it = kurlList.begin(); it != kurlList.end(); ++it)
        addFile(*it);
}

// Enable these action buttons
void
AudioManagerDialog::updateActionState(bool haveSelection)
{
    if (m_doc->getAudioFileManager().begin() ==
	m_doc->getAudioFileManager().end()) {
	stateChanged("have_audio_files", KXMLGUIClient::StateReverse);
    } else {
	stateChanged("have_audio_files", KXMLGUIClient::StateNoReverse);
    }

    if (haveSelection) {

	stateChanged("have_audio_selected", KXMLGUIClient::StateNoReverse);

	if (m_audiblePreview) {
	    stateChanged("have_audible_preview", KXMLGUIClient::StateNoReverse);
	} else {
	    stateChanged("have_audible_preview", KXMLGUIClient::StateReverse);
	}	

	if (isSelectedTrackAudio()) {
	    stateChanged("have_audio_insertable", KXMLGUIClient::StateNoReverse);
	} else {
	    stateChanged("have_audio_insertable", KXMLGUIClient::StateReverse);
	}

    } else {
	stateChanged("have_audio_selected", KXMLGUIClient::StateReverse);
	stateChanged("have_audio_insertable", KXMLGUIClient::StateReverse);
	stateChanged("have_audible_preview", KXMLGUIClient::StateReverse);
    }
}

void
AudioManagerDialog::slotInsert()
{
    AudioFile *audioFile = getCurrentSelection();
    if (audioFile == 0)
        return;

    RG_DEBUG << "AudioManagerDialog::slotInsert\n";

    emit insertAudioSegment(audioFile->getId(),
                            RealTime::zeroTime,
                            audioFile->getLength());
}

void
AudioManagerDialog::slotRemoveAll()
{
    QString question =
        i18n("This will unload all audio files and remove their associated segments.\nThis action cannot be undone, and associations with these files will be lost.\nFiles will not be removed from your disk.\nAre you sure?");

    int reply = KMessageBox::warningContinueCancel(this, question);

    if (reply != KMessageBox::Continue)
        return;

    SegmentSelection selection;
    Composition &comp = m_doc->getComposition();

    for (Composition::iterator it = comp.begin(); it != comp.end(); ++it)
    {
        if ((*it)->getType() == Segment::Audio)
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
AudioManagerDialog::slotRemoveAllUnused()
{
    QString question =
        i18n("This will unload all audio files that are not associated with any segments in this composition.\nThis action cannot be undone, and associations with these files will be lost.\nFiles will not be removed from your disk.\nAre you sure?");

    int reply = KMessageBox::warningContinueCancel(this, question);

    if (reply != KMessageBox::Continue)
        return;

    std::set<AudioFileId> audioFiles;
    Composition &comp = m_doc->getComposition();

    for (Composition::iterator it = comp.begin(); it != comp.end(); ++it)
    {
        if ((*it)->getType() == Segment::Audio)
            audioFiles.insert((*it)->getAudioFileId());
    }

    std::vector<AudioFileId> toDelete;
    for (std::vector<AudioFile*>::const_iterator
         aIt = m_doc->getAudioFileManager().begin();
         aIt != m_doc->getAudioFileManager().end(); ++aIt)
    {
        if (audioFiles.find((*aIt)->getId()) == audioFiles.end())
            toDelete.push_back((*aIt)->getId());
    }

    // Delete the audio files from the AFM
    //
    for (std::vector<AudioFileId>::iterator dIt = toDelete.begin();
            dIt != toDelete.end(); ++dIt)
    {
        m_doc->getAudioFileManager().removeFile(*dIt);
        emit deleteAudioFile(*dIt);
    }

    // clear the file list
    m_fileList->clear();
    slotPopulateFileList();
}

class UnusedAudioSelectionDialog : public KDialogBase
{
public:
    UnusedAudioSelectionDialog(QWidget *, AudioFileManager *, std::vector<AudioFileId>);
    
    std::vector<QString> getSelectedAudioFileNames() const;

protected:
    AudioFileManager *m_mgr;
    std::vector<AudioFileId> m_ids;
    QListView *m_listView;
};

UnusedAudioSelectionDialog::UnusedAudioSelectionDialog(QWidget *parent,
						       AudioFileManager *mgr,
						       std::vector<AudioFileId> ids) :
    KDialogBase(parent, 0, true, i18n("Select Unused Audio Files"), Ok | Cancel),
    m_mgr(mgr),
    m_ids(ids)
{
    QVBox *vbox = makeVBoxMainWidget();
    new QLabel(i18n("The following audio files are not used in the current composition.\n\nPlease select the ones you wish to delete permanently from the hard disk.\n"), vbox);

    m_listView = new KListView(vbox);

    m_listView->addColumn(i18n("File name"));
    m_listView->addColumn(i18n("File size"));
    m_listView->addColumn(i18n("Last modified date"));

    for (int i = 0; i < ids.size(); ++i) {
	AudioFile *af = mgr->getAudioFile(ids[i]);
	if (!af) {
	    std::cerr << "WARNING: UnusedAudioSelectionDialog: No audio file for id " << ids[i] << std::endl;
	    continue;
	}
	QString fileName = strtoqstr(af->getFilename());
	QFileInfo info(fileName);
	QString fileSize = i18n(" (not found) ");
	QString fileDate;
	if (info.exists()) {
	    fileSize = QString(" %1 ").arg(info.size());
	    fileDate = QString(" %1 ").arg(info.lastModified().toString());
	}
	QListViewItem *item = new KListViewItem
	    (m_listView, fileName, fileSize, fileDate);
    }

    m_listView->setSelectionMode(QListView::Multi);
}

std::vector<QString> 
UnusedAudioSelectionDialog::getSelectedAudioFileNames() const
{
    std::vector<QString> selectedNames;

    QListViewItem *item = 0;

    for (int i = 0; i < m_ids.size(); ++i) {

	if (item) item = item->nextSibling();
	else item = m_listView->firstChild();
	
	if (m_listView->isSelected(item)) {
	    selectedNames.push_back(item->text(0));
	}
    }

    return selectedNames;
}

void
AudioManagerDialog::slotDeleteUnused()
{
    std::set<AudioFileId> audioFiles;
    Composition &comp = m_doc->getComposition();

    for (Composition::iterator it = comp.begin(); it != comp.end(); ++it)
    {
        if ((*it)->getType() == Segment::Audio)
            audioFiles.insert((*it)->getAudioFileId());
    }

    std::vector<AudioFileId> toDelete;
    std::map<QString, AudioFileId> nameMap;

    for (std::vector<AudioFile*>::const_iterator
         aIt = m_doc->getAudioFileManager().begin();
         aIt != m_doc->getAudioFileManager().end(); ++aIt)
    {
        if (audioFiles.find((*aIt)->getId()) == audioFiles.end()) {
            toDelete.push_back((*aIt)->getId());
	    nameMap[(*aIt)->getFilename()] = (*aIt)->getId();
	}
    }

    UnusedAudioSelectionDialog *dialog = new UnusedAudioSelectionDialog
	(this, &m_doc->getAudioFileManager(), toDelete);

    if (dialog->exec() == QDialog::Accepted) {

	std::vector<QString> names = dialog->getSelectedAudioFileNames();

	QString question =
	    i18n("About to delete %1 audio file(s) permanently from the hard disk.\nThis action cannot be undone, and there will be no way to recover these files.\nAre you sure?").arg(names.size());

	int reply = KMessageBox::warningContinueCancel(this, question);

	if (reply != KMessageBox::Continue) {
	    delete dialog;
	    return;
	}

	for (int i = 0; i < names.size(); ++i) {
	    std::cerr << i << ": " << names[i] << std::endl;
	    QFile file(names[i]);
	    if (!file.remove()) {
		KMessageBox::error(this, i18n("File %1 could not be deleted.").arg(names[i]));
	    } else {
		if (nameMap.find(names[i]) != nameMap.end()) {
		    m_doc->getAudioFileManager().removeFile(nameMap[names[i]]);
		    emit deleteAudioFile(nameMap[names[i]]);
		} else {
		    std::cerr << "WARNING: Audio file name " << names[i] << " not in name map" << std::endl;
		}

		QFile peakFile(QString("%1.pk").arg(names[i]));
		peakFile.remove();
	    }
	}
    }

    m_fileList->clear();
    slotPopulateFileList();
    
    delete dialog;
}


void
AudioManagerDialog::slotRename()
{
    AudioFile *audioFile = getCurrentSelection();

    if (audioFile == 0)
        return;

    bool ok = false;

    QString newText = KLineEditDlg::getText(
                                            i18n("Change Audio File label"),
                                            i18n("Enter new label"),
                                            QString(audioFile->getName().c_str()),
                                            &ok,
                                            this);

    if ( ok && !newText.isEmpty() )
        audioFile->setName(qstrtostr(newText));

    slotPopulateFileList();
}

void
AudioManagerDialog::slotSelectionChanged(QListViewItem *item)
{
    AudioListItem *aItem = dynamic_cast<AudioListItem*>(item);

    // If we're on a segment then send a "select" signal
    // and enable appropriate buttons.
    //
    if (aItem && aItem->getSegment())
    {
        SegmentSelection selection;
        selection.insert(aItem->getSegment());
        emit segmentsSelected(selection);
    }
    
    updateActionState(aItem != 0);
}


// Select a Segment - traverse the tree and locate the correct child
// or parent.  We need both id and segment because of this duality of
// purpose.  Set segment to 0 for parent audio entries.
//
void
AudioManagerDialog::setSelected(AudioFileId id, 
                                const Segment *segment,
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
                selectFileListItemNoSignal(it);
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
                        selectFileListItemNoSignal(chIt);
                        
                        // Only propagate to segmentcanvas if asked to
                        if (propagate)
                        {
                            SegmentSelection selection;
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

void
AudioManagerDialog::selectFileListItemNoSignal(QListViewItem* it)
{
    m_fileList->blockSignals(true);

    if (it) {
        m_fileList->ensureItemVisible(it);
        m_fileList->setSelected(it, true);
    } else {
        m_fileList->clearSelection();
    }
    
    m_fileList->blockSignals(false);
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
AudioManagerDialog::slotCommandExecuted(KCommand*)
{
    slotPopulateFileList();
}


void
AudioManagerDialog::slotSegmentSelection(
        const SegmentSelection &segments)
{
    const Segment *segment = 0;

    for (SegmentSelection::const_iterator it = segments.begin();
                                    it != segments.end(); ++it)
    {
        if ((*it)->getType() == Segment::Audio) 
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
        selectFileListItemNoSignal(0);
    }

}

void
AudioManagerDialog::slotCancelPlayingAudioFile()
{
    emit cancelPlayingAudioFile(m_playingAudioFile);
}


void
AudioManagerDialog::closePlayingDialog(AudioFileId id)
{
    //std::cout << "AudioManagerDialog::closePlayingDialog" << std::endl;
    if (m_audioPlayingDialog && id == m_playingAudioFile)
    {
        m_playTimer->stop();
        delete m_audioPlayingDialog;
        m_audioPlayingDialog = 0;
    }

}

bool
AudioManagerDialog::addFile(const KURL& kurl)
{
    AudioFileId id = 0;

    // Now set the "last add" path so that next time we use "file add"
    // we start looking in the same place.
    //
    RosegardenProgressDialog progressDlg(i18n("Generating audio preview..."),
                                         100,
                                         this);
    connect(&progressDlg, SIGNAL(cancelClicked()), 
            this, SLOT(slotAddCancel()));

    CurrentProgressDialog::set(&progressDlg);

    QString newFilePath;

    if (kurl.isLocalFile()) {

        newFilePath = kurl.path();

    } else { // download locally
        if (KIO::NetAccess::download(kurl, newFilePath) == false) {
            KMessageBox::error(this, i18n("Cannot download file %1").arg(kurl.prettyURL()));
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
        CurrentProgressDialog::freeze();

        QString errorString = i18n("Cannot add file %1: %2")
	    .arg(kurl.prettyURL()).arg(strtoqstr(e));
        KMessageBox::sorry(this, errorString);
        return false;
    }
    catch(QString e)
    {
        CurrentProgressDialog::freeze();

        QString errorString = i18n("Cannot add file %1: %2")
	    .arg(kurl.prettyURL()).arg(e);
        KMessageBox::sorry(this, errorString);
        return false;
    }

    // Connect the progress dialog
    //
    connect(&m_doc->getAudioFileManager(), SIGNAL(setProgress(int)),
            progressDlg.progressBar(), SLOT(setValue(int)));

    try
    {
        m_doc->getAudioFileManager().generatePreview(id);
    }
    catch(std::string e)
    {
        CurrentProgressDialog::freeze();

        QString message = strtoqstr(e) + "\n\n" +
                          i18n("Try copying this file to a directory where you have write permission and re-add it");
        KMessageBox::information(this, message);
        //return false;
    }

    disconnect(&progressDlg, SIGNAL(cancelClicked()),
               this, SLOT(slotAddCancel()));

    slotPopulateFileList();

    // tell the sequencer
    emit addAudioFile(id);

    return true;
}

void
AudioManagerDialog::slotAddCancel()
{
    RG_DEBUG << "AudioManagerDialog::slotAddCancel" << endl;
    m_doc->getAudioFileManager().stopPreview();
    CurrentProgressDialog::freeze();
}



void
AudioManagerDialog::slotDropped(QDropEvent *event, QListViewItem*)
{
    QStrList uri;

    // see if we can decode a URI.. if not, just ignore it
    if (QUriDrag::decode(event, uri))
    {
        // okay, we have a URI.. process it
        for(QString url = uri.first(); url; url = uri.next()) {
            
            RG_DEBUG << "AudioManagerDialog::dropEvent() : got "
                                 << url << endl;

            addFile(KURL(url));
        }
        
    }
}

void
AudioManagerDialog::closeEvent(QCloseEvent *e)
{
    RG_DEBUG << "AudioManagerDialog::closeEvent()\n";
    emit closing();
    KMainWindow::closeEvent(e);
}

void
AudioManagerDialog::slotClose()
{
    emit closing();
    close();
    //KDockMainWindow::slotClose();
//     delete this;
}

void
AudioManagerDialog::setAudioSubsystemStatus(bool ok)
{
    // We can do something more fancy in the future but for the moment
    // this will suffice.
    //
    m_audiblePreview = ok;
}

// Wrapper to allow external things to add files (say from drops on the main view)
//
bool
AudioManagerDialog::addAudioFile(const QString &filePath)
{
    return addFile(QFileInfo(filePath).absFilePath());
}


bool
AudioManagerDialog::isSelectedTrackAudio()
{
    Composition &comp = m_doc->getComposition();
    Studio &studio = m_doc->getStudio();

    TrackId currentTrackId = comp.getSelectedTrack();
    Track *track = comp.getTrackById(currentTrackId);

    if (track) {
	InstrumentId ii = track->getInstrument();
	Instrument *instrument = studio.getInstrumentById(ii);

	if (instrument &&
	    instrument->getType() == Instrument::Audio)
            return true;
    }
    
    return false;
    
}

void
AudioManagerDialog::slotDistributeOnMidiSegment()
{
    RG_DEBUG << "AudioManagerDialog::slotDistributeOnMidiSegment" << endl;

    //Composition &comp = m_doc->getComposition();

    QList<RosegardenGUIView>& viewList = m_doc->getViewList();
    RosegardenGUIView *w = 0;
    SegmentSelection selection;

    for(w = viewList.first(); w != 0; w = viewList.next())
    {
        selection = w->getSelection();
    }

    // Store the insert times in a local vector
    //
    std::vector<timeT> insertTimes;

    for (SegmentSelection::iterator i = selection.begin();
         i != selection.end(); ++i)
    {
        // For MIDI (Internal) Segments only of course
        //
        if ((*i)->getType() == Segment::Internal)
        {
            for (Segment::iterator it = (*i)->begin(); it != (*i)->end(); ++it)
            {
                if ((*it)->isa(Note::EventType))
                    insertTimes.push_back((*it)->getAbsoluteTime());
            }
        }
    }

    for (unsigned int i = 0; i < insertTimes.size(); ++i)
    {
        RG_DEBUG << "AudioManagerDialog::slotDistributeOnMidiSegment - "
                 << "insert audio segment at " << insertTimes[i]
                 << endl;
    }
}


const char* const AudioManagerDialog::AudioManagerDialogConfigGroup = "AudioManagerDialog";
 
}

#include "audiomanagerdialog.moc"
