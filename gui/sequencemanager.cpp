// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>

#include <qdir.h>
#include <qbutton.h>
#include <qpushbutton.h>
#include <qcursor.h>
#include <qtimer.h>

#include <klocale.h>
#include <kconfig.h>
#include <kmessagebox.h>
#include <kstddirs.h>

#include "rgapplication.h"
#include "constants.h"
#include "audiopluginmanager.h"
#include "ktmpstatusmsg.h"
#include "rosestrings.h"
#include "rosegardenguidoc.h"
#include "rosegardentransportdialog.h"
#include "rosegardenguiview.h"
#include "sequencemanager.h"
#include "ControlBlock.h"
#include "SegmentPerformanceHelper.h"
#include "SoundDriver.h"
#include "MappedRealTime.h"
#include "studiocontrol.h"
#include "MidiDevice.h"
#include "widgets.h"
#include "dialogs.h"
#include "diskspace.h"

using std::cout;
using std::cerr;
using std::endl;

// Seems not to be properly defined under some gcc 2.95 setups
#ifndef MREMAP_MAYMOVE
#define MREMAP_MAYMOVE 1
#endif

namespace Rosegarden
{

class ControlBlockMmapper
{
public:
    ControlBlockMmapper(RosegardenGUIDoc*);
    ~ControlBlockMmapper();
    
    QString getFileName() { return m_fileName; }
    void refresh();
    void updateTrackData(Track*);

protected:
    void initControlBlock();
    void setFileSize(size_t);
    QString createFileName();

    //--------------- Data members ---------------------------------
    RosegardenGUIDoc* m_doc;
    QString m_fileName;
    bool m_needsRefresh;
    int m_fd;
    void* m_mmappedBuffer;
    size_t m_mmappedSize;
    ControlBlock* m_controlBlock;
};

ControlBlockMmapper::ControlBlockMmapper(RosegardenGUIDoc* doc)
    : m_doc(doc),
      m_fileName(createFileName()),
      m_needsRefresh(true),
      m_fd(-1),
      m_mmappedBuffer(0),
      m_mmappedSize(ControlBlock::getSize()),
      m_controlBlock(0)
{
    // just in case
    QFile::remove(m_fileName);

    m_fd = ::open(m_fileName.latin1(), O_RDWR|O_CREAT|O_TRUNC,
                  S_IRUSR|S_IWUSR);
    if (m_fd < 0) {
        SEQMAN_DEBUG << "ControlBlockMmapper : Couldn't open " << m_fileName
                     << endl;
        throw Rosegarden::Exception("Couldn't open " + qstrtostr(m_fileName));
    }

    setFileSize(m_mmappedSize);

    //
    // mmap() file for writing
    //
    m_mmappedBuffer = ::mmap(0, m_mmappedSize,
                             PROT_READ|PROT_WRITE,
                             MAP_SHARED, m_fd, 0);

    if (m_mmappedBuffer == (void*)-1) {
        SEQMAN_DEBUG << QString("mmap failed : (%1) %2\n").arg(errno).arg(strerror(errno));
        throw Rosegarden::Exception("mmap failed");
    }

    SEQMAN_DEBUG << "ControlBlockMmapper : mmap size : " << m_mmappedSize
                 << " at " << (void*)m_mmappedBuffer << endl;

    // Create new control block on file
    m_controlBlock = new (m_mmappedBuffer) ControlBlock(doc->getComposition().getNbTracks());

    initControlBlock();
}

ControlBlockMmapper::~ControlBlockMmapper()
{
    ::munmap(m_mmappedBuffer, m_mmappedSize);
    ::close(m_fd);
}


QString ControlBlockMmapper::createFileName()
{
    return KGlobal::dirs()->resourceDirs("tmp").first() + "/rosegarden_control_block";
}

void ControlBlockMmapper::refresh()
{
    SEQMAN_DEBUG << "ControlBlockMmapper : refresh\n";

    if (m_needsRefresh) {
        ::msync(m_mmappedBuffer, m_mmappedSize, MS_ASYNC);

        rgapp->sequencerSend("remapControlBlock()");

        m_needsRefresh = false;
    }
}

void ControlBlockMmapper::updateTrackData(Track *t)
{
    m_controlBlock->updateTrackData(t);
    m_needsRefresh = true;
}


void ControlBlockMmapper::initControlBlock()
{
    Composition& comp = m_doc->getComposition();
    
    for(Composition::trackiterator i = comp.getTracks().begin(); i != comp.getTracks().end(); ++i) {
        Track* track = i->second;
        if (track == 0) continue;
        
        m_controlBlock->updateTrackData(track);
    }

    refresh();
}


void ControlBlockMmapper::setFileSize(size_t size)
{
    SEQMAN_DEBUG << "ControlBlockMmapper : setting size of "
                 << m_fileName << " to " << size << endl;
    // rewind
    ::lseek(m_fd, 0, SEEK_SET);

    //
    // enlarge the file
    // (seek() to wanted size, then write a byte)
    //
    if (::lseek(m_fd, size - 1, SEEK_SET) == -1) {
        SEQMAN_DEBUG << "ControlBlockMmapper : Couldn't lseek in " << m_fileName
                     << " to " << size << endl;
        throw Rosegarden::Exception("lseek failed");
    }
    
    if (::write(m_fd, "\0", 1) != 1) {
        SEQMAN_DEBUG << "ControlBlockMmapper : Couldn't write byte in  "
                     << m_fileName << endl;
        throw Rosegarden::Exception("write failed");
    }
    
}

//----------------------------------------


class SegmentMmapper
{
public:
    SegmentMmapper(RosegardenGUIDoc*, Segment*,
                   const QString& fileName);
    ~SegmentMmapper();

    /**
     * refresh the object after the segment has been modified
     * returns true if size changed (and thus the sequencer
     * needs to be told about it
     */
    bool refresh();

    QString getFileName() { return m_fileName; }

    unsigned int getSegmentRepeatCount();
    size_t computeMmappedSize();

protected:
    /// put all data to be dumped in internal byte array
    void prepareDump();

    /// set the size of the mmapped filed
    void setFileSize(size_t);

    /// perform the mmap() of the file
    void doMmap();

    /// mremap() the file after a size change
    void remap(size_t newsize);

    /// dump all segment data in the file
    void dump();

    //--------------- Data members ---------------------------------
    RosegardenGUIDoc* m_doc;
    Segment* m_segment;
    QString m_fileName;

    int m_fd;
    size_t m_mmappedSize;
    char* m_mmappedBuffer;
    QByteArray m_byteArray;
};

//----------------------------------------

class CompositionMmapper
{
    friend class SequenceManager;

public:
    CompositionMmapper(RosegardenGUIDoc *doc);
    ~CompositionMmapper();

    QString getSegmentFileName(Segment*);

    void cleanup();

protected:
    bool segmentModified(Segment*);
    void segmentAdded(Segment*);
    void segmentDeleted(Segment*);

    void mmapSegment(Segment*);
    QString makeFileName(Segment*);

    //--------------- Data members ---------------------------------

    RosegardenGUIDoc* m_doc;
    typedef std::map<Segment*, SegmentMmapper*> segmentmmapers;

    segmentmmapers m_segmentMmappers;
};

//----------------------------------------

SequenceManager::SequenceManager(RosegardenGUIDoc *doc,
                                 RosegardenTransportDialog *transport):
    m_doc(doc),
    m_compositionMmapper(new CompositionMmapper(m_doc)),
    m_controlBlockMmapper(new ControlBlockMmapper(m_doc)),
    m_transportStatus(STOPPED),
    m_soundDriverStatus(NO_DRIVER),
    m_transport(transport),
    m_lastRewoundAt(clock()),
    m_sliceFetched(true), // default to true (usually ignored)
    m_countdownDialog(0),
    m_countdownTimer(new QTimer(doc)),
    m_recordTime(new QTime()),
    m_compositionRefreshStatusId(m_doc->getComposition().getNewRefreshStatusId()),
    m_updateRequested(true)
{
    m_compositionMmapper->cleanup();

    m_countdownDialog = new CountdownDialog(dynamic_cast<QWidget*>
                                (m_doc->parent())->parentWidget());
    // Connect this for use later
    //
    connect(m_countdownTimer, SIGNAL(timeout()),
            this, SLOT(slotCountdownTimerTimeout()));

    connect(doc->getCommandHistory(), SIGNAL(commandExecuted()),
	    this, SLOT(update()));

    m_doc->getComposition().addObserver(this);
}


SequenceManager::~SequenceManager()
{
    m_doc->getComposition().removeObserver(this);

    SEQMAN_DEBUG << "SequenceManager::~SequenceManager()\n";   
    delete m_compositionMmapper;
    delete m_controlBlockMmapper;
}

void SequenceManager::setDocument(RosegardenGUIDoc* doc)
{
    m_doc->getComposition().removeObserver(this);
    disconnect(m_doc->getCommandHistory(), SIGNAL(commandExecuted()));
    
    m_segments.clear();
    m_doc = doc;

    // Must recreate and reconnect the countdown timer and dialog
    // (bug 729039)
    //
    delete m_countdownDialog;
    delete m_countdownTimer;

    m_countdownDialog = new CountdownDialog(dynamic_cast<QWidget*>
                                (m_doc->parent())->parentWidget());

    m_countdownTimer = new QTimer(m_doc);

    // Connect this for use later
    //
    connect(m_countdownTimer, SIGNAL(timeout()),
            this, SLOT(slotCountdownTimerTimeout()));

    if (m_doc) {
	m_compositionRefreshStatusId =
	    m_doc->getComposition().getNewRefreshStatusId();
        m_doc->getComposition().addObserver(this);

        connect(m_doc->getCommandHistory(), SIGNAL(commandExecuted()),
                this, SLOT(update()));

    }

    resetCompositionMmapper();
}

RosegardenGUIDoc* SequenceManager::getDocument()
{
    return m_doc;
}


void
SequenceManager::setTransportStatus(const TransportStatus &status)
{
    m_transportStatus = status;
}

void
SequenceManager::play()
{
    Composition &comp = m_doc->getComposition();

    QByteArray data;
    QCString replyType;
    QByteArray replyData;
  
    // If already playing or recording then stop
    //
    if (m_transportStatus == PLAYING ||
        m_transportStatus == RECORDING_MIDI ||
        m_transportStatus == RECORDING_AUDIO )
        {
            stopping();
            return;
        }

    // This check may throw an exception
    checkSoundDriverStatus();

    // Align Instrument lists and send initial program changes
    //
    preparePlayback();

    // Send audio latencies
    //
    //sendAudioLatencies();

    // make sure we toggle the play button
    // 
    m_transport->PlayButton()->setOn(true);

    // write the start position argument to the outgoing stream
    //
    QDataStream streamOut(data, IO_WriteOnly);

    if (comp.getTempo() == 0)
        {
            comp.setDefaultTempo(120.0);

            SEQMAN_DEBUG << "SequenceManager::play() - setting Tempo to Default value of 120.000\n";
        }
    else
        {
            SEQMAN_DEBUG << "SequenceManager::play() - starting to play\n";
        }

    // set the tempo in the transport
    m_transport->setTempo(comp.getTempo());

    // The arguments for the Sequencer
    RealTime startPos = comp.getElapsedRealTime(comp.getPosition());

    // If we're looping then jump to loop start
    if (comp.isLooping())
        startPos = comp.getElapsedRealTime(comp.getLoopStart());

    KConfig* config = kapp->config();
    config->setGroup(Rosegarden::LatencyOptionsConfigGroup);

    Rosegarden::Configuration& docConfig = m_doc->getConfiguration();

    // playback start position
    streamOut << startPos.sec;
    streamOut << startPos.usec;

    // playback latency
    streamOut << config->readLongNumEntry("playbacklatencysec", 0);
    streamOut << config->readLongNumEntry("playbacklatencyusec", 100000);

    // fetch latency
    RealTime fetchLatency = docConfig.get<RealTimeT>("fetchlatency");
    streamOut << fetchLatency.sec;
    streamOut << fetchLatency.usec;

    // read ahead slice
    streamOut << config->readLongNumEntry("readaheadsec", 0);
    streamOut << config->readLongNumEntry("readaheadusec", 40000);

    // Send Play to the Sequencer
    try {
        
        rgapp->sequencerCall("play(long int, long int, long int, long int, long int, long int, long int, long int)",
                             replyType, replyData, data);
    
    } catch (Rosegarden::Exception e) {
        m_transportStatus = STOPPED;
        throw(e);
    }

    // ensure the return type is ok
    QDataStream streamIn(replyData, IO_ReadOnly);
    int result;
    streamIn >> result;
  
    if (result) {
        // completed successfully 
        m_transportStatus = STARTING_TO_PLAY;
    } else {
        m_transportStatus = STOPPED;
        throw(Rosegarden::Exception("Failed to start playback"));
    }
}

void
SequenceManager::stopping()
{
    // Do this here rather than in stop() to avoid any potential
    // race condition (we use setPointerPosition() during stop()).
    //
    if (m_transportStatus == STOPPED)
    {
        if (m_doc->getComposition().isLooping())
            m_doc->setPointerPosition(m_doc->getComposition().getLoopStart());
        else
            m_doc->setPointerPosition(m_doc->getComposition().getStartMarker());

        return;
    }

    // Disarm recording and drop back to STOPPED
    //
    if (m_transportStatus == RECORDING_ARMED)
    {
        m_transportStatus = STOPPED;
        m_transport->RecordButton()->setOn(false);
        m_transport->MetronomeButton()->
            setOn(m_doc->getComposition().usePlayMetronome());
        return;
    }

    SEQMAN_DEBUG << "SequenceManager::stopping() - preparing to stop\n";

    stop();

}

void
SequenceManager::stop()
{
    // Toggle off the buttons - first record
    //
    if (m_transportStatus == RECORDING_MIDI ||
        m_transportStatus == RECORDING_AUDIO)
    {
        m_transport->RecordButton()->setOn(false);
        m_transport->MetronomeButton()->
            setOn(m_doc->getComposition().usePlayMetronome());

        // Remove the countdown dialog and stop the timer (if being used)
        //
        m_countdownDialog->hide();
        m_countdownTimer->stop();
    }

    // Now playback
    m_transport->PlayButton()->setOn(false);

    // "call" the sequencer with a stop so we get a synchronous
    // response - then we can fiddle about with the audio file
    // without worrying about the sequencer causing problems
    // with access to the same audio files.
    //

    // wait cursor
    //
    QApplication::setOverrideCursor(QCursor(Qt::waitCursor));

    rgapp->sequencerSend("stop()");

    // restore
    QApplication::restoreOverrideCursor();

    // if we're recording MIDI or Audio then tidy up the recording Segment
    if (m_transportStatus == RECORDING_MIDI)
    {
        m_doc->stopRecordingMidi();
        SEQMAN_DEBUG << "SequenceManager::stop() - stopped recording MIDI\n";
    }

    if (m_transportStatus == RECORDING_AUDIO)
    {
        m_doc->stopRecordingAudio();
        SEQMAN_DEBUG << "SequenceManager::stop() - stopped recording audio\n";
    }

    // always untoggle the play button at this stage
    //
    m_transport->PlayButton()->setOn(false);
    SEQMAN_DEBUG << "SequenceManager::stop() - stopped playing\n";

    // ok, we're stopped
    //
    m_transportStatus = STOPPED;

    resetControllers();
}

// Jump to previous bar
//
void
SequenceManager::rewind()
{
    Rosegarden::Composition &composition = m_doc->getComposition();

    timeT position = composition.getPosition();
    std::pair<timeT, timeT> barRange =
	composition.getBarRangeForTime(position - 1);

    if (m_transportStatus == PLAYING) {

	// if we're playing and we had a rewind request less than 200ms
	// ago and we're some way into the bar but less than half way
	// through it, rewind two barlines instead of one

	clock_t now = clock();
	int elapsed = (now - m_lastRewoundAt) * 1000 / CLOCKS_PER_SEC;

	SEQMAN_DEBUG << "That was " << m_lastRewoundAt << ", this is " << now << ", elapsed is " << elapsed << endl;

	if (elapsed >= 0 && elapsed <= 200) {
	    if (position > barRange.first &&
		position < barRange.second &&
		position <= (barRange.first + (barRange.second -
					       barRange.first) / 2)) {
		barRange = composition.getBarRangeForTime(barRange.first - 1);
	    }
	}

	m_lastRewoundAt = now;
    }
    
    if (barRange.first < composition.getStartMarker()) {
	m_doc->setPointerPosition(composition.getStartMarker());
    } else {
	m_doc->setPointerPosition(barRange.first);
    }
}


// Jump to next bar
//
void
SequenceManager::fastforward()
{
    Rosegarden::Composition &composition = m_doc->getComposition();

    timeT position = composition.getPosition() + 1;
    timeT newPosition = composition.getBarRangeForTime(position).second;

    // Don't skip past end marker
    //
    if (newPosition > composition.getEndMarker())
        newPosition = composition.getEndMarker();

    m_doc->setPointerPosition(newPosition);

}


// This method is a callback from the Sequencer to update the GUI
// with state change information.  The GUI requests the Sequencer
// to start playing or to start recording and enters a pending
// state (see rosegardendcop.h for TransportStatus values).
// The Sequencer replies when ready with it's status.  If anything
// fails then we default (or try to default) to STOPPED at both
// the GUI and the Sequencer.
//
void
SequenceManager::notifySequencerStatus(TransportStatus status)
{
    // for the moment we don't do anything fancy
    m_transportStatus = status;

}


void
SequenceManager::sendSequencerJump(const Rosegarden::RealTime &time)
{
    QByteArray data;
    QDataStream streamOut(data, IO_WriteOnly);
    streamOut << time.sec;
    streamOut << time.usec;

    rgapp->sequencerSend("jumpTo(long int, long int)", data);
}



// Called when we want to start recording from the GUI.
// This method tells the sequencer to start recording and
// from then on the sequencer returns MappedCompositions
// to the GUI via the "processRecordedMidi() method -
// also called via DCOP
//
//

void
SequenceManager::record(bool toggled)
{
    Rosegarden::Composition &comp = m_doc->getComposition();
    Rosegarden::Studio &studio = m_doc->getStudio();
    KConfig* config = kapp->config();
    config->setGroup(Rosegarden::GeneralOptionsConfigGroup);

    // Rather clumsy additional check for audio subsys when we start
    // recording - once we enforce audio subsystems then this will
    // become redundant.
    //
    if (!(m_soundDriverStatus & AUDIO_OK)) {
        int rID = comp.getRecordTrack();
        Rosegarden::InstrumentId instrId =
            comp.getTrackById(rID)->getInstrument();
        Rosegarden::Instrument *instr = studio.getInstrumentById(instrId);

        if (!instr || instr->getType() == Rosegarden::Instrument::Audio) {
            m_transport->RecordButton()->setOn(false);
            throw(Rosegarden::Exception("Audio subsystem is not available - can't record audio"));
        }
    }

    if (toggled) {
        if (m_transportStatus == RECORDING_ARMED) {
            SEQMAN_DEBUG << "SequenceManager::record - unarming record\n";
            m_transportStatus = STOPPED;

            // Toggle the buttons
            m_transport->MetronomeButton()->setOn(comp.usePlayMetronome());
            m_transport->RecordButton()->setOn(false);

            return;
        }

        if (m_transportStatus == STOPPED) {
            SEQMAN_DEBUG << "SequenceManager::record - armed record\n";
            m_transportStatus = RECORDING_ARMED;

            // Toggle the buttons
            m_transport->MetronomeButton()->setOn(comp.useRecordMetronome());
            m_transport->RecordButton()->setOn(true);

            return;
        }

        if (m_transportStatus == RECORDING_MIDI ||
            m_transportStatus == RECORDING_AUDIO) {
            SEQMAN_DEBUG << "SequenceManager::record - stop recording and keep playing\n";
            return;
        }

        if (m_transportStatus == PLAYING) {
            SEQMAN_DEBUG << "SequenceManager::record - punch in recording\n";
            return;
        }

    } else {
        // if already recording then stop
        //
        if (m_transportStatus == RECORDING_MIDI ||
            m_transportStatus == RECORDING_AUDIO) {
            stopping();
            return;
        }

        // Get the record track and check the Instrument type
        int rID = comp.getRecordTrack();
        Rosegarden::InstrumentId inst =
            comp.getTrackById(rID)->getInstrument();

        // If no matching record instrument
        //
        if (studio.getInstrumentById(inst) == 0) {
            m_transport->RecordButton()->setDown(false);
            throw(Rosegarden::Exception("No Record instrument selected"));
        }


        // may throw an exception
        checkSoundDriverStatus();

        // toggle the Metronome button if it's in use
        m_transport->MetronomeButton()->setOn(comp.useRecordMetronome());

        // If we are looping then jump to start of loop and start recording,
        // if we're not take off the number of count-in bars and start 
        // recording.
        //
        if(comp.isLooping())
            m_doc->setPointerPosition(comp.getLoopStart());
        else {
            if (m_transportStatus != RECORDING_ARMED) {
                int startBar = comp.getBarNumber(comp.getPosition());
                startBar -= config->readUnsignedNumEntry("countinbars", 2);
                m_doc->setPointerPosition(comp.getBarRange(startBar).first);
            }
        }

        // Some locals
        //
        TransportStatus recordType;
        QByteArray data;
        QCString replyType;
        QByteArray replyData;

        switch (studio.getInstrumentById(inst)->getType()) {
        case Rosegarden::Instrument::Midi:
            recordType = STARTING_TO_RECORD_MIDI;
            SEQMAN_DEBUG << "SequenceManager::record() - starting to record MIDI\n";
            break;

        case Rosegarden::Instrument::Audio: {
            // check for disk space available
            Rosegarden::DiskSpace *space;
            Rosegarden::AudioFileManager &afm = 
                m_doc->getAudioFileManager();
            QString audioPath = strtoqstr(afm.getAudioPath());

            try {
                space = new Rosegarden::DiskSpace(audioPath);
            }
            catch(QString e)
                {
                    // Add message and re-throw
                    //
                    QString m = i18n("Audio record path \"") +
                        audioPath + QString("\". ") + e + QString("\n") +
                        i18n("Edit your audio path properties (Edit->Edit Document Properties->Audio)");
                    throw(m);
                }

            // Check the disk space available is within current
            // audio recording limit
            //
            config->setGroup(Rosegarden::SequencerOptionsConfigGroup);
            int audioRecordMinutes = config->
                readNumEntry("audiorecordminutes", 5);

            Rosegarden::AudioPluginManager *apm = 
                m_doc->getPluginManager();

            // Ok, check disk space and compare to limits
            //space->getFreeKBytes()

            recordType = STARTING_TO_RECORD_AUDIO;
            SEQMAN_DEBUG << "SequenceManager::record() - "
                         << "starting to record Audio\n";
            break;
        }
            
        default:
            SEQMAN_DEBUG << "SequenceManager::record() - unrecognised instrument type\n";
            return;
            break;
        }

        // set the buttons
        m_transport->RecordButton()->setOn(true);
        m_transport->PlayButton()->setOn(true);

        // write the start position argument to the outgoing stream
        //
        QDataStream streamOut(data, IO_WriteOnly);

        if (comp.getTempo() == 0) {
            SEQMAN_DEBUG << "SequenceManager::play() - setting Tempo to Default value of 120.000\n";
            comp.setDefaultTempo(120.0);
        } else {
            SEQMAN_DEBUG << "SequenceManager::record() - starting to record\n";
        }

        // set the tempo in the transport
        //
        m_transport->setTempo(comp.getTempo());

        // The arguments for the Sequencer  - record is similar to playback,
        // we must being playing to record.
        //
        Rosegarden::RealTime startPos =
            comp.getElapsedRealTime(comp.getPosition());
        Rosegarden::Configuration &docConfig = m_doc->getConfiguration();

        // playback start position
        streamOut << startPos.sec;
        streamOut << startPos.usec;
    
        // set group
        config->setGroup(Rosegarden::LatencyOptionsConfigGroup);

        // playback latency
        streamOut << config->readLongNumEntry("playbacklatencysec", 0);
        streamOut << config->readLongNumEntry("playbacklatencyusec", 100000);

        // fetch latency
        RealTime fetchLatency = docConfig.get<RealTimeT>("fetchlatency");
        streamOut << fetchLatency.sec;
        streamOut << fetchLatency.usec;

        // read ahead slice
        streamOut << config->readLongNumEntry("readaheadsec", 0);
        streamOut << config->readLongNumEntry("readaheadusec", 40000);
    
        // record type
        streamOut << (int)recordType;
    
        // Send Play to the Sequencer
        try {
            
            rgapp->sequencerCall("record(long int, long int, long int, long int, long int, long int, long int, long int, int)",
                                 replyType, replyData, data);
        } catch(Rosegarden::Exception e) {
            // failed
            m_transportStatus = STOPPED;
            throw(e);
        }

        // ensure the return type is ok
        QDataStream streamIn(replyData, IO_ReadOnly);
        int result;
        streamIn >> result;
  
        if (result) {
            // completed successfully 
            m_transportStatus = recordType;

            if (recordType == STARTING_TO_RECORD_AUDIO) {
                // Create the countdown timer dialog for limiting the
                // audio recording time.
                //
                KConfig* config = kapp->config();
                config->setGroup(Rosegarden::SequencerOptionsConfigGroup);

                int seconds = 60 * 
                    (config->readNumEntry("audiorecordminutes", 5));

                // re-initialise
                m_countdownDialog->setTotalTime(seconds);

                connect(m_countdownDialog, SIGNAL(stopped()),
                        this, SLOT(slotCountdownCancelled()));

                connect(m_countdownDialog, SIGNAL(completed()),
                        this, SLOT(slotCountdownStop()));

                // Create the timer
                //
                m_recordTime->start();

                // Start an elapse timer for updating the dialog -
                // it will fire every second.
                //
                m_countdownTimer->start(1000);

                // Pop-up the dialog (don't use exec())
                //
                m_countdownDialog->show();

            }
        } else {
            // Stop immediately - turn off buttons in parent
            //
            m_transportStatus = STOPPED;

            if (recordType == STARTING_TO_RECORD_AUDIO) {
                throw(Rosegarden::Exception("Couldn't start recording audio.  Ensure your audio record path is valid\nin Document Properties (Edit->Edit Document Properties->Audio)"));
            } else {
                throw(Rosegarden::Exception("Couldn't start recording MIDI"));
            }

        }
    }
}



// This method accepts an incoming MappedComposition and goes about
// inserting it into the Composition and updating the display to show
// what has been recorded and where.
//
// For ALSA we reflect the events straight back out to the instrument
// we're currently playing just like in processAsynchronousMidi.
// aRts does it's own MIDI thru which we don't bother we worrying
// about for the moment.
//
//
void
SequenceManager::processRecordedMidi(const MappedComposition &mC)
{
    processAsynchronousMidi(mC, 0);


    // Send any recorded Events to a Segment for storage and display.
    // We have to send the transport status because this method is
    // called asynchronously from the sequencer and the calls below
    // can create a new recording SegmentItem on the canvas if we
    // don't check that recording is coming to a close (or has already
    // been stopped).
    //
    //  Filter on the way out.
    //
    //
    m_doc->insertRecordedMidi(
            applyFiltering(mC,
                           Rosegarden::MappedEvent::MappedEventType(
                               m_doc->getStudio().getMIDIRecordFilter())),
            m_transportStatus);

}



// Process unexpected MIDI events at the GUI - send them to the Transport
// or to a MIDI mixer for display purposes only.  Useful feature to enable
// the musician to prove to herself quickly that the MIDI input is still
// working.
//
//
void
SequenceManager::processAsynchronousMidi(const MappedComposition &mC,
                                         Rosegarden::AudioManagerDialog
                                             *audioManagerDialog)
{
    if (m_doc == 0) return;

    if (mC.size())
    {
        Rosegarden::MappedComposition::iterator i;
        Rosegarden::Composition &comp = m_doc->getComposition();
        Rosegarden::Track *track =
                  comp.getTrackById(comp.getSelectedTrack());
        Rosegarden::InstrumentId id = track->getInstrument();

        Rosegarden::MappedComposition tempMC =
                applyFiltering(mC,
                               Rosegarden::MappedEvent::MappedEventType(
                                   m_doc->getStudio().getMIDIThruFilter()));

        Rosegarden::MappedComposition retMC;

        // send all events to the MIDI in label
        //
        for (i = tempMC.begin(); i != tempMC.end(); ++i )
        {
            m_transport->setMidiInLabel(*i);

            // Skip all audio events
            //
            if ((*i)->getType() >= Rosegarden::MappedEvent::Audio)
            {
                if ((*i)->getType() == Rosegarden::MappedEvent::AudioStopped)
                {
                    /*
                    SEQMAN_DEBUG << "AUDIO FILE ID = "
                                 << int((*i)->getData1())
                                 << " - FILE STOPPED - " 
                                 << "INSTRUMENT = "
                                 << (*i)->getInstrument()
                                 << endl;
                    */

                    if (audioManagerDialog && (*i)->getInstrument() == 
                            m_doc->getStudio().getAudioPreviewInstrument())
                    {
                        audioManagerDialog->
                            closePlayingDialog(
                                Rosegarden::AudioFileId((*i)->getData1()));
                    }
                }

                if ((*i)->getType() == Rosegarden::MappedEvent::AudioLevel)
                    sendAudioLevel(*i);

                if ((*i)->getType() == 
                        Rosegarden::MappedEvent::AudioGeneratePreview)
                {
                    m_doc->finalizeAudioFile(
                            Rosegarden::AudioFileId((*i)->getData1()));
                }

                if ((*i)->getType() ==
                        Rosegarden::MappedEvent::SystemUpdateInstruments)
                {
                    // resync Devices and Instruments
                    //
                    m_doc->syncDevices();
                }

                continue;

            } else {

		// if we aren't playing or recording, consider invoking any
		// step-by-step clients

		SEQMAN_DEBUG << "m_transportStatus = " << m_transportStatus << endl;

		if (m_transportStatus == STOPPED ||
		    m_transportStatus == RECORDING_ARMED) {

		    if ((*i)->getType() == Rosegarden::MappedEvent::MidiNote) {
			if ((*i)->getVelocity() == 0) {
			    emit insertableNoteOffReceived((*i)->getPitch());
			} else {
			    emit insertableNoteOnReceived((*i)->getPitch());
			}
		    }
		}
	    }

#ifdef HAVE_ALSA
            (*i)->setInstrument(id);
#endif
            retMC.insert(new Rosegarden::MappedEvent(*i));
        }

#ifdef HAVE_ALSA
        // MIDI thru implemented at this layer for ALSA for
        // the moment.  aRts automatically does MIDI through,
        // this does it to the currently selected instrument.
        //
        showVisuals(retMC);

        // Filter
        //
        Rosegarden::StudioControl::sendMappedComposition(retMC);

#endif 
    }
}


void
SequenceManager::rewindToBeginning()
{
    SEQMAN_DEBUG << "SequenceManager::rewindToBeginning()\n";
    m_doc->setPointerPosition(m_doc->getComposition().getStartMarker());
}


void
SequenceManager::fastForwardToEnd()
{
    SEQMAN_DEBUG << "SequenceManager::fastForwardToEnd()\n";

    Composition &comp = m_doc->getComposition();
    m_doc->setPointerPosition(comp.getDuration());
}


// Called from the LoopRuler (usually a double click)
// to set position and start playing
//
void
SequenceManager::setPlayStartTime(const timeT &time)
{

    // If already playing then stop
    //
    if (m_transportStatus == PLAYING ||
        m_transportStatus == RECORDING_MIDI ||
        m_transportStatus == RECORDING_AUDIO )
    {
        stopping();
        return;
    }
    
    // otherwise off we go
    //
    m_doc->setPointerPosition(time);
    play();
}


void
SequenceManager::setLoop(const timeT &lhs, const timeT &rhs)
{
    // Let the sequencer know about the loop markers
    //
    QByteArray data;
    QDataStream streamOut(data, IO_WriteOnly);

    Rosegarden::RealTime loopStart =
            m_doc->getComposition().getElapsedRealTime(lhs);
    Rosegarden::RealTime loopEnd =
            m_doc->getComposition().getElapsedRealTime(rhs);

    streamOut << loopStart.sec;
    streamOut << loopStart.usec;
    streamOut << loopEnd.sec;
    streamOut << loopEnd.usec;
  
    rgapp->sequencerSend("setLoop(long int, long int, long int, long int)", data);
}

void
SequenceManager::checkSoundDriverStatus()
{
    QCString replyType;
    QByteArray replyData;

    try {
        rgapp->sequencerCall("getSoundDriverStatus()", replyType, replyData);

    } catch (Rosegarden::Exception e) {
        // failed
	m_soundDriverStatus = NO_DRIVER;
        throw(e);
    }

    QDataStream streamIn(replyData, IO_ReadOnly);
    unsigned int result;
    streamIn >> result;
    m_soundDriverStatus = result;

    if (m_soundDriverStatus == NO_DRIVER)
        throw(Rosegarden::Exception("MIDI and Audio subsystems have failed to initialise"));

    if (!(m_soundDriverStatus & MIDI_OK))
        throw(Rosegarden::Exception("MIDI subsystem has failed to initialise"));

    /*
      if (!(m_soundDriverStatus & AUDIO_OK))
      throw(Rosegarden::Exception("Audio subsystem has failed to initialise"));
    */
}


// Insert metronome clicks into the global MappedComposition that
// will be returned as part of the slice fetch from the Sequencer.
//
//
void
SequenceManager::insertMetronomeClicks(const timeT &sliceStart,
                                       const timeT &sliceEnd)
{
    Composition &comp = m_doc->getComposition();
    Studio &studio = m_doc->getStudio();
    Configuration &config = m_doc->getConfiguration();

    const MidiMetronome *metronome = studio.getMetronome();

    // Create a default metronome if we haven't loaded one
    //
    if(metronome == 0)
    {
	long pitch = 60;
	config.get<Int>("metronomepitch", pitch);

        // Default instrument is the first possible instrument
        //
        metronome = new MidiMetronome(Rosegarden::MidiProgram(),
				      pitch,
				      Rosegarden::SystemInstrumentBase);
    }

    Rosegarden::RealTime mDuration = config.get<RealTimeT>("metronomeduration");
    Rosegarden::MidiByte mBarVelocity =
        Rosegarden::MidiByte(config.get<Int>("metronomebarvelocity"));
    Rosegarden::MidiByte mBeatVelocity =
        Rosegarden::MidiByte(config.get<Int>("metronomebeatvelocity"));

    if (mDuration == Rosegarden::RealTime(0, 0))
        mDuration = Rosegarden::RealTime(0, 10000);

    if (mBarVelocity == 0) mBarVelocity = 120;
    if (mBeatVelocity == 0) mBeatVelocity = 80;

    // If neither metronome is armed and we're not playing or recording
    // then don't sound the metronome
    //
    if(!((m_transportStatus == PLAYING || m_transportStatus == STARTING_TO_PLAY)
         && comp.usePlayMetronome()) &&
       !((m_transportStatus == RECORDING_MIDI ||
          m_transportStatus == RECORDING_AUDIO ||
          m_transportStatus == STARTING_TO_RECORD_MIDI ||
          m_transportStatus == STARTING_TO_RECORD_AUDIO)
          && comp.useRecordMetronome()))
        return;

    std::pair<timeT, timeT> barStart =
        comp.getBarRange(comp.getBarNumber(sliceStart));
    std::pair<timeT, timeT> barEnd =
        comp.getBarRange(comp.getBarNumber(sliceEnd));

    // The slice can straddle a bar boundary so check
    // in both bars for the marker
    //
    if (barStart.first >= sliceStart && barStart.first <= sliceEnd)
    {
        MappedEvent *me =
                new MappedEvent(metronome->getInstrument(),
                                metronome->getPitch(),
                                mBarVelocity,
                                comp.getElapsedRealTime(barStart.first),
                                mDuration);
        m_mC.insert(me);
    }
    else if (barEnd.first >= sliceStart && barEnd.first <= sliceEnd)
    {
        MappedEvent *me =
                new MappedEvent(metronome->getInstrument(),
                                metronome->getPitch(),
                                mBarVelocity,
                                comp.getElapsedRealTime(barEnd.first),
                                mDuration);
        m_mC.insert(me);
    }

    // Is this solution for the beats bulletproof?  I'm not so sure.
    //
    bool isNew;
    TimeSignature timeSig =
        comp.getTimeSignatureInBar(comp.getBarNumber(sliceStart), isNew);

    for (int i = barStart.first + timeSig.getBeatDuration();
             i < barStart.second;
             i += timeSig.getBeatDuration())
    {
        if (i >= sliceStart && i <= sliceEnd)
        {
            MappedEvent *me = new MappedEvent(metronome->getInstrument(),
                                              metronome->getPitch(),
                                              mBeatVelocity,
                                              comp.getElapsedRealTime(i),
                                              mDuration);
            m_mC.insert(me);
        }
    }
}

// Send Instrument list to Sequencer and ensure that initial program
// changes follow them.  Sending the instruments ensures that we have
// channels available on the Sequencer and then the program changes
// are sent to those specific channel (referenced by Instrument ID)
//
// 
void
SequenceManager::preparePlayback()
{
    Rosegarden::Studio &studio = m_doc->getStudio();
    Rosegarden::InstrumentList list = studio.getAllInstruments();
    Rosegarden::MappedComposition mC;
    Rosegarden::MappedEvent *mE;

    // Send the MappedInstruments (minimal Instrument information
    // required for Performance) to the Sequencer
    //
    InstrumentList::iterator it = list.begin();
    for (; it != list.end(); it++)
    {
        Rosegarden::StudioControl::sendMappedInstrument(MappedInstrument(*it));

        // Send program changes for MIDI Instruments
        //
        if ((*it)->getType() == Instrument::Midi)
        {
            // send bank select always before program change
            //
            if ((*it)->sendsBankSelect())
            {
                
                mE = new MappedEvent((*it)->getId(),
                                     Rosegarden::MappedEvent::MidiController,
                                     Rosegarden::MIDI_CONTROLLER_BANK_MSB,
                                     (*it)->getMSB());
                mC.insert(mE);

                mE = new MappedEvent((*it)->getId(),
                                     Rosegarden::MappedEvent::MidiController,
                                     Rosegarden::MIDI_CONTROLLER_BANK_LSB,
                                     (*it)->getLSB());
                mC.insert(mE);
            }

            // send program change
            //
            if ((*it)->sendsProgramChange())
            {
                mE = new MappedEvent((*it)->getId(),
                                     Rosegarden::MappedEvent::MidiProgramChange,
                                     (*it)->getProgramChange());
                mC.insert(mE);
            }

        }
        else if ((*it)->getType() == Instrument::Audio)
        {
            Rosegarden::StudioControl::setStudioObjectProperty(
                    (*it)->getId(), "value", (*it)->getVolume());
        }
        else
        {
            std::cerr << "SequenceManager::preparePlayback - "
                      << "unrecognised instrument type" << std::endl;
        }


    }

    // Send the MappedComposition if it's got anything in it
    showVisuals(mC);
    Rosegarden::StudioControl::sendMappedComposition(mC);

    // Set up the audio playback latency
    //
    KConfig* config = kapp->config();
    config->setGroup(Rosegarden::LatencyOptionsConfigGroup);

    int jackSec = config->readLongNumEntry("jackplaybacklatencysec", 0);
    int jackUSec = config->readLongNumEntry("jackplaybacklatencyusec", 0);
    m_playbackAudioLatency = Rosegarden::RealTime(jackSec, jackUSec);

}

void
SequenceManager::processRecordedAudio(const Rosegarden::RealTime &time)
{
    m_doc->insertRecordedAudio(time, m_transportStatus);
}


void
SequenceManager::sendAudioLevel(Rosegarden::MappedEvent *mE)
{
    RosegardenGUIView *v;
    QList<RosegardenGUIView>& viewList = m_doc->getViewList();

    for (v = viewList.first(); v != 0; v = viewList.next())
    {
        v->showVisuals(mE);
    }

}

void
SequenceManager::resetControllers()
{
    SEQMAN_DEBUG << "SequenceManager::resetControllers - resetting\n";
    Rosegarden::MappedComposition mC;

    // Should do all Midi Instrument - not just guess like this is doing
    // currently.

    for (unsigned int i = 0; i < 16; i++)
    {
        Rosegarden::MappedEvent *mE =
            new Rosegarden::MappedEvent(Rosegarden::MidiInstrumentBase + i,
                                        Rosegarden::MappedEvent::MidiController,
                                        MIDI_CONTROLLER_RESET,
                                        0);

        mC.insert(mE);
    }
    showVisuals(mC);
    Rosegarden::StudioControl::sendMappedComposition(mC);
}


void
SequenceManager::getSequencerPlugins(Rosegarden::AudioPluginManager *aPM)
{
    Rosegarden::MappedObjectId id =
        Rosegarden::StudioControl::getStudioObjectByType(
                Rosegarden::MappedObject::AudioPluginManager);

    SEQMAN_DEBUG << "getSequencerPlugins - getting plugin information" << endl;
    
    Rosegarden::MappedObjectPropertyList seqPlugins
        = Rosegarden::StudioControl::getStudioObjectProperty(
                id, Rosegarden::MappedAudioPluginManager::Plugins);

    SEQMAN_DEBUG << "getSequencerPlugins - got "
                 << seqPlugins.size() << " items" << endl;

    /*
    Rosegarden::MappedObjectPropertyList seqPluginIds
        = getSequencerPropertyList(id,
                               Rosegarden::MappedAudioPluginManager::PluginIds);
                               */

    //Rosegarden::MappedObjectPropertyList::iterator it;

    unsigned int i = 0;

    while (i < seqPlugins.size())
    {
        Rosegarden::MappedObjectId id = seqPlugins[i++].toInt();
        QString name = seqPlugins[i++];
        unsigned long uniqueId = seqPlugins[i++].toLong();
        QString label = seqPlugins[i++];
        QString author = seqPlugins[i++];
        QString copyright = seqPlugins[i++];
        unsigned int portCount = seqPlugins[i++].toInt();

        AudioPlugin *aP = aPM->addPlugin(id,
                                         name,
                                         uniqueId,
                                         label,
                                         author,
                                         copyright);

        // SEQMAN_DEBUG << "PLUGIN = \"" << name << "\"" << endl;

        for (unsigned int j = 0; j < portCount; j++)
        {
            id = seqPlugins[i++].toInt();
            name = seqPlugins[i++];
            Rosegarden::PluginPort::PortType type =
                Rosegarden::PluginPort::PortType(seqPlugins[i++].toInt());
            Rosegarden::PluginPort::PortRange range =
                Rosegarden::PluginPort::PortRange(seqPlugins[i++].toInt());
            Rosegarden::PortData lowerBound = seqPlugins[i++].toFloat();
            Rosegarden::PortData upperBound = seqPlugins[i++].toFloat();
	    Rosegarden::PortData defaultValue = seqPlugins[i++].toFloat();

	    // SEQMAN_DEBUG << "DEFAULT =  " << defaultValue << endl;
            // SEQMAN_DEBUG << "ADDED PORT = \"" << name << "\"" << endl;
            aP->addPort(id,
                        name,
                        type,
                        range,
                        lowerBound,
                        upperBound,
			defaultValue);

        }

        // SEQMAN_DEBUG << " = " << seqPlugins[i] << endl;

        /*
        Rosegarden::MappedObjectPropertyList author =
            getSequencerPropertyList(seqPluginIds[i].toInt(), "author");

        if (author.size() == 1)
            SEQMAN_DEBUG << "PLUGIN AUTHOR = \"" << author[0] << "\"" << std::endl;
            */
    }
}

// Clear down all temporary (non read-only) objects and then
// add the basic audio faders only (one per instrument).
//
void
SequenceManager::reinitialiseSequencerStudio()
{
    // Send the MIDI recording device to the sequencer
    //
    KConfig* config = kapp->config();
    config->setGroup(Rosegarden::SequencerOptionsConfigGroup);

    QString recordDeviceStr = config->readEntry("midirecorddevice");

    if (recordDeviceStr)
    {
        int recordDevice = recordDeviceStr.toInt();

        if (recordDevice >= 0)
        {
            Rosegarden::MappedEvent *mE =
                new Rosegarden::MappedEvent(
                    Rosegarden::MidiInstrumentBase, // InstrumentId
                    Rosegarden::MappedEvent::SystemRecordDevice,
                    Rosegarden::MidiByte(recordDevice));

            Rosegarden::StudioControl::sendMappedEvent(mE);
            SEQMAN_DEBUG << "set MIDI record device to "
                         << recordDevice << endl;
        }
    }

    // Setup JACK audio inputs
    //
    int jackAudioInputs = config->readNumEntry("jackaudioinputs", 2);

    Rosegarden::MappedEvent *mE =
        new Rosegarden::MappedEvent(
            Rosegarden::MidiInstrumentBase, // InstrumentId
            Rosegarden::MappedEvent::SystemAudioInputs,
            Rosegarden::MidiByte(jackAudioInputs));

    Rosegarden::StudioControl::sendMappedEvent(mE);


    // Set the studio from the current document
    //
    m_doc->initialiseStudio();
}

// Clear down all playing notes and reset controllers
//
void
SequenceManager::panic()
{
    SEQMAN_DEBUG << "panic button\n";

    Studio &studio = m_doc->getStudio();

    InstrumentList list = studio.getPresentationInstruments();
    InstrumentList::iterator it;

    Rosegarden::MappedComposition mC;
    Rosegarden::MappedEvent *mE;

    int maxDevices = 0, device = 0;
    for (it = list.begin(); it != list.end(); it++)
        if ((*it)->getType() == Instrument::Midi)
            maxDevices++;

    emit setProgress(10);
    for (it = list.begin(); it != list.end(); it++)
    {
        if ((*it)->getType() == Instrument::Midi)
        {
            emit setProgress(int(70.0 * float(device)/float(maxDevices)));
            for (unsigned int i = 0; i < 128; i++)
            {
                mE = new MappedEvent((*it)->getId(),
                                     Rosegarden::MappedEvent::MidiNote,
                                     i,
                                     0,
                                     RealTime(0, 0),
                                     RealTime(0, 0),
                                     RealTime(0, 0));
                mC.insert(mE);
            }

            device++;
        }
    }

    Rosegarden::StudioControl::sendMappedComposition(mC);
    emit setProgress(90);

    resetControllers();
}

// In this case we only route MIDI events to the transport ticker
//
void
SequenceManager::showVisuals(const Rosegarden::MappedComposition &mC)
{
    RosegardenGUIView *v;
    QList<RosegardenGUIView>& viewList = m_doc->getViewList();

    MappedComposition::iterator it = mC.begin();
    for (; it != mC.end(); it++)
    {
        for (v = viewList.first(); v != 0; v = viewList.next())
        {
            //v->showVisuals(*it);
            m_transport->setMidiOutLabel(*it);
        }
    }
}


// Filter a MappedComposition by Type.
//
Rosegarden::MappedComposition
SequenceManager::applyFiltering(const Rosegarden::MappedComposition &mC,
                                Rosegarden::MappedEvent::MappedEventType filter)
{
    Rosegarden::MappedComposition retMc;
    Rosegarden::MappedComposition::iterator it = mC.begin();

    for (; it != mC.end(); it++)
    {
        if (!((*it)->getType() & filter))
            retMc.insert(new MappedEvent(*it));
    }

    return retMc;
}

void
SequenceManager::setSequencerSliceSize(const RealTime &time)
{
//     if (m_transportStatus == PLAYING ||
//         m_transportStatus == RECORDING_MIDI ||
//         m_transportStatus == RECORDING_AUDIO )
//     {
//         QByteArray data;
//         QDataStream streamOut(data, IO_WriteOnly);

//         KConfig* config = kapp->config();
//         config->setGroup(Rosegarden::LatencyOptionsConfigGroup);

//         if (time == RealTime(0, 0)) // reset to default values
//         {
//             streamOut << config->readLongNumEntry("readaheadsec", 0);
//             streamOut << config->readLongNumEntry("readaheadusec", 40000);
//         }
//         else
//         {
//             streamOut << time.sec;
//             streamOut << time.usec;
//         }

//         if (!kapp->dcopClient()->
//                 send(ROSEGARDEN_SEQUENCER_APP_NAME,
//                      ROSEGARDEN_SEQUENCER_IFACE_NAME,
//                      "setSliceSize(long int, long int)",
//                      data))
//         {
//             SEQMAN_DEBUG << "couldn't set sequencer slice" << endl;
//             return;
//         }

//         // Ok, set this token and wait for the sequencer to fetch a
//         // new slice.
//         //
//         m_sliceFetched = false;

//         int msecs = (time.sec * 1000) + (time.usec / 1000);
//         SEQMAN_DEBUG << "set sequencer slice = " << msecs
//                      << "ms" << endl;

//         // Spin until the sequencer has got the next slice - we only
//         // do this if we're at the top loop level (otherwise DCOP
//         // events don't get processed)
//         //
//         /*
//         if (kapp->loopLevel() > 1)
//             SEQMAN_DEBUG << "can't wait for slice fetch" << endl;
//         else
//             while (m_sliceFetched == false) kapp->processEvents();
//             */

//     }
}

// Set a temporary slice size.  Wait until this slice is fetched
// before continuing.  The sequencer will then revert slice size 
// to original value for subsequent fetches.
//
void
SequenceManager::setTemporarySequencerSliceSize(const RealTime& time)
{
//     if (m_transportStatus == PLAYING ||
//         m_transportStatus == RECORDING_MIDI ||
//         m_transportStatus == RECORDING_AUDIO )
//     {
//         QByteArray data;
//         QDataStream streamOut(data, IO_WriteOnly);

//         KConfig* config = kapp->config();
//         config->setGroup(Rosegarden::LatencyOptionsConfigGroup);

//         if (time == RealTime(0, 0)) // reset to default values
//         {
//             streamOut << config->readLongNumEntry("readaheadsec", 0);
//             streamOut << config->readLongNumEntry("readaheadusec", 40000);
//         }
//         else
//         {
//             streamOut << time.sec;
//             streamOut << time.usec;
//         }


//         if (!kapp->dcopClient()->
//                 send(ROSEGARDEN_SEQUENCER_APP_NAME,
//                      ROSEGARDEN_SEQUENCER_IFACE_NAME,
//                      "setTemporarySliceSize(long int, long int)",
//                      data))
//         {
//             SEQMAN_DEBUG << "couldn't set temporary sequencer slice" << endl;
//             return;
//         }

//         // Ok, set this token and wait for the sequencer to fetch a
//         // new slice.
//         //
//         m_sliceFetched = false;

//         int msecs = (time.sec * 1000) + (time.usec / 1000);
//         SEQMAN_DEBUG << "set temporary sequencer slice = " << msecs
//                      << "ms" << endl;

//         // Spin until the sequencer has got the next slice - only do
//         // this if we're being called from the top loop level.
//         //
//         if (kapp->loopLevel() > 1)
//             SEQMAN_DEBUG << "can't wait for slice fetch" << endl;
//         else
//             while (m_sliceFetched == false) kapp->processEvents();
//     }
}

void SequenceManager::resetCompositionMmapper()
{
    SEQMAN_DEBUG << "SequenceManager::resetCompositionMmapper()\n";
    delete m_compositionMmapper;
    m_compositionMmapper = new CompositionMmapper(m_doc);
}

//----------------------------------------

CompositionMmapper::CompositionMmapper(RosegardenGUIDoc *doc)
    : m_doc(doc)
{
    SEQMAN_DEBUG << "CompositionMmapper() - doc = " << doc << endl;
    Composition &comp = m_doc->getComposition();

    for (Composition::iterator it = comp.begin(); it != comp.end(); it++) {

        Rosegarden::Track* track = comp.getTrackById((*it)->getTrack());

        // check to see if track actually exists
        //
        if (track == 0)
            continue;

        mmapSegment(*it);
    }
}

CompositionMmapper::~CompositionMmapper()
{
    SEQMAN_DEBUG << "~CompositionMmapper()\n";

    //
    // Clean up possible left-overs
    //
    cleanup();

    for(segmentmmapers::iterator i = m_segmentMmappers.begin();
        i != m_segmentMmappers.end(); ++i)
        delete i->second;
}

void CompositionMmapper::cleanup()
{
    // In case the sequencer is still running, mapping some segments
    //
    rgapp->sequencerSend("closeAllSegments()");

    // Erase all 'segment_*' files
    //
    QString tmpPath = KGlobal::dirs()->resourceDirs("tmp").first();

    QDir segmentsDir(tmpPath, "segment_*");
    for (unsigned int i = 0; i < segmentsDir.count(); ++i) {
        QString segmentName = tmpPath + '/' + segmentsDir[i];
        SEQMAN_DEBUG << "CompositionMmapper : cleaning up " << segmentName << endl;
        QFile::remove(segmentName);
    }
    
}


bool CompositionMmapper::segmentModified(Segment* segment)
{
    SegmentMmapper* mmapper = m_segmentMmappers[segment];

    SEQMAN_DEBUG << "CompositionMmapper::segmentModified(" << segment << ") - mmapper = "
                 << mmapper << endl;

    return mmapper->refresh();
}

void CompositionMmapper::segmentAdded(Segment* segment)
{
    SEQMAN_DEBUG << "CompositionMmapper::segmentAdded(" << segment << ")\n";

    mmapSegment(segment);
}

void CompositionMmapper::segmentDeleted(Segment* segment)
{
    SEQMAN_DEBUG << "CompositionMmapper::segmentDeleted(" << segment << ")\n";
    SegmentMmapper* mmapper = m_segmentMmappers[segment];
    m_segmentMmappers.erase(segment);
    SEQMAN_DEBUG << "CompositionMmapper::segmentDeleted() : deleting SegmentMmapper " << mmapper << endl;

    delete mmapper;
}

void CompositionMmapper::mmapSegment(Segment* segment)
{
    SEQMAN_DEBUG << "CompositionMmapper::mmapSegment(" << segment << ")\n";

    SegmentMmapper* mmapper = new SegmentMmapper(m_doc, segment,
                                                 makeFileName(segment));

    m_segmentMmappers[segment] = mmapper;
}

QString CompositionMmapper::makeFileName(Segment* segment)
{
    QStringList tmpDirs = KGlobal::dirs()->resourceDirs("tmp");

    return QString("%1/segment_%2")
        .arg(tmpDirs.first())
        .arg((unsigned int)segment, 0, 16);
}

QString CompositionMmapper::getSegmentFileName(Segment* s)
{
    SegmentMmapper* mmapper = m_segmentMmappers[s];
    
    if (mmapper)
        return mmapper->getFileName();
    else
        return QString::null;
}


//----------------------------------------

SegmentMmapper::SegmentMmapper(RosegardenGUIDoc* doc,
                               Segment* segment, const QString& fileName)
    : m_doc(doc),
      m_segment(segment),
      m_fileName(fileName),
      m_fd(-1),
      m_mmappedSize(computeMmappedSize()),
      m_mmappedBuffer((char*)0)
{
    SEQMAN_DEBUG << "SegmentMmapper : " << this
                 << " trying to mmap " << m_fileName
                 << endl;

    m_fd = ::open(m_fileName.latin1(), O_RDWR|O_CREAT|O_TRUNC,
                  S_IRUSR|S_IWUSR);
    if (m_fd < 0) {
        SEQMAN_DEBUG << "SegmentMmapper : Couldn't open " << m_fileName
                     << endl;
        throw Rosegarden::Exception("Couldn't open " + qstrtostr(m_fileName));
    }

    SEQMAN_DEBUG << "SegmentMmapper : mmap size = " << m_mmappedSize
                 << endl;

    if (m_mmappedSize > 0) {
        setFileSize(m_mmappedSize);
        doMmap();
        dump();
    } else {
        SEQMAN_DEBUG << "SegmentMmapper : mmap size = 0 - skipping mmapping for now\n";
    }
}

size_t SegmentMmapper::computeMmappedSize()
{
    int repeatCount = getSegmentRepeatCount();

    return (repeatCount + 1) * m_segment->size() * MappedEvent::streamedSize;
}


SegmentMmapper::~SegmentMmapper()
{
    SEQMAN_DEBUG << "~SegmentMmapper : " << this
                 << " unmapping " << (void*)m_mmappedBuffer
                 << " of size " << m_mmappedSize
                 << endl;

    if (m_mmappedBuffer && m_mmappedSize)
        ::munmap(m_mmappedBuffer, m_mmappedSize);

    ::close(m_fd);
    SEQMAN_DEBUG << "~SegmentMmapper : removing " << m_fileName << endl;

    QFile::remove(m_fileName);
}


bool SegmentMmapper::refresh()
{
    bool res = false;

    int repeatCount = getSegmentRepeatCount();

    size_t newMmappedSize = computeMmappedSize();

    SEQMAN_DEBUG << "SegmentMmapper::refresh() - m_mmappedBuffer = "
                 << (void*)m_mmappedBuffer << " - size = " << newMmappedSize << endl;

    // always zero out
    memset(m_mmappedBuffer, 0, m_mmappedSize);

    if (newMmappedSize != m_mmappedSize) {

        if (newMmappedSize == 0) {

            // nothing to do, just msync and go
            ::msync(m_mmappedBuffer, m_mmappedSize, MS_ASYNC);
            m_mmappedSize = 0;
            return true;

        } else if (newMmappedSize > m_mmappedSize) {

            setFileSize(newMmappedSize);
            remap(newMmappedSize);
            res = true;
        }
    }
    
    SEQMAN_DEBUG << "SegmentMmapper::refresh : mmap size = " << m_mmappedSize
                 << endl;

    dump();

    return res;
}

void SegmentMmapper::setFileSize(size_t size)
{
    SEQMAN_DEBUG << "SegmentMmapper : setting size of "
                 << m_fileName << " to " << size << endl;
    // rewind
    ::lseek(m_fd, 0, SEEK_SET);

    //
    // enlarge the file
    // (seek() to wanted size, then write a byte)
    //
    if (::lseek(m_fd, size - 1, SEEK_SET) == -1) {
        SEQMAN_DEBUG << "SegmentMmapper : Couldn't lseek in " << m_fileName
                     << " to " << size << endl;
        throw Rosegarden::Exception("lseek failed");
    }
    
    if (::write(m_fd, "\0", 1) != 1) {
        SEQMAN_DEBUG << "SegmentMmapper : Couldn't write byte in  "
                     << m_fileName << endl;
        throw Rosegarden::Exception("write failed");
    }
    
}

void SegmentMmapper::remap(size_t newsize)
{
    SEQMAN_DEBUG << "SegmentMmapper : remapping " << m_fileName
                 << " from size " << m_mmappedSize
                 << " to size " << newsize << endl;

    if (!m_mmappedBuffer) { // nothing to mremap, just mmap
        
        SEQMAN_DEBUG << "SegmentMmapper : nothing to remap - mmap instead\n";
        m_mmappedSize = newsize;
        doMmap();

    } else {

#ifdef linux
        m_mmappedBuffer = (char*)::mremap(m_mmappedBuffer, m_mmappedSize,
                                          newsize, MREMAP_MAYMOVE);
#else
	::munmap(m_mmappedBuffer, m_mmappedSize);
	m_mmappedBuffer = (char *)::mmap(0, newsize,
					 PROT_READ|PROT_WRITE,
					 MAP_SHARED, m_fd, 0);
#endif
    
        if (m_mmappedBuffer == (void*)-1) {
            SEQMAN_DEBUG << QString("mremap failed : (%1) %2\n").arg(errno).arg(strerror(errno));
            throw Rosegarden::Exception("mremap failed");

        }

        m_mmappedSize = newsize;

    }
    
}

void SegmentMmapper::doMmap()
{
    //
    // mmap() file for writing
    //
    m_mmappedBuffer = (char*)::mmap(0, m_mmappedSize,
                                    PROT_READ|PROT_WRITE,
                                    MAP_SHARED, m_fd, 0);

    if (m_mmappedBuffer == (void*)-1) {
        SEQMAN_DEBUG << QString("mmap failed : (%1) %2\n").arg(errno).arg(strerror(errno));
        throw Rosegarden::Exception("mmap failed");
    }

    SEQMAN_DEBUG << "SegmentMmapper::doMmap() - mmap size : " << m_mmappedSize
                 << " at " << (void*)m_mmappedBuffer << endl;
    
}

void SegmentMmapper::prepareDump()
{
    // temporary byte array on which we dump the events
    QByteArray byteArray;
    QDataStream stream(byteArray, IO_WriteOnly);

    Composition &comp = m_doc->getComposition();

    Rosegarden::RealTime eventTime;
    Rosegarden::RealTime duration;
    Rosegarden::Instrument *instrument = 0;
    Rosegarden::Track* track = comp.getTrackById(m_segment->getTrack());
    
    SegmentPerformanceHelper helper(*m_segment);

    timeT segmentStartTime = m_segment->getStartTime();
    timeT segmentEndTime = m_segment->getEndMarkerTime();
    timeT segmentDuration = segmentEndTime - segmentStartTime;
    timeT repeatEndTime = segmentEndTime;

    int repeatCount = getSegmentRepeatCount();

    if (repeatCount > 0) repeatEndTime = m_segment->getRepeatEndTime();

    unsigned int nbEvents = 0;

    for (int repeatNo = 0; repeatNo <= repeatCount; ++repeatNo) {

        for (Segment::iterator j = m_segment->begin();
             j != m_segment->end(); ++j) {

            if ((*j)->isa(Rosegarden::Note::EventRestType)) continue;

            timeT playTime =
                helper.getSoundingAbsoluteTime(j) + repeatNo * segmentDuration;
            if (playTime >= repeatEndTime) break;

            eventTime = comp.getElapsedRealTime(playTime);

            duration = helper.getRealSoundingDuration(j);

            // No duration and we're a note?  Probably in a tied
            // series, but not as first note
            //
            if (duration == Rosegarden::RealTime(0, 0) &&
                (*j)->isa(Rosegarden::Note::EventType))
                continue;
	    
            try {
                // Create mapped event
                MappedEvent mE(0, // the instrument will be extracted from the ControlBlock by the sequencer
                               **j,
                               eventTime,
                               duration);
                mE.setTrackId(track->getId());

                // dump it on stream
                //             SEQMAN_DEBUG << "SegmentMmapper::dump - event "
                //                          << nbEvents++ << " at "
                //                          << stream.device()->at() << endl;
                stream << mE;
                //             SEQMAN_DEBUG << "SegmentMmapper::dump - now at "
                //                          << stream.device()->at() << endl;
            } catch(...) {
                SEQMAN_DEBUG << "SegmentMmapper::dump - caught exception while trying to create MappedEvent\n";
            }
        }
    }

    m_byteArray = byteArray;
}

void SegmentMmapper::dump()
{
    prepareDump();

    if (m_byteArray.size() > 0) {

        // "Safe" way to do things : resize the mmapped file if the QByteArray has grown larger
        //
//         if (m_byteArray.size() > m_mmappedSize) {
//             SEQMAN_DEBUG << QString("SegmentMmapper::dump : internal byte array has grown larger (%1) than mmapped buffer (%2), enlarging buffer\n")
//                 .arg(m_byteArray.size()).arg(m_mmappedSize);
//             setFileSize(m_byteArray.size());
//             remap(m_byteArray.size());
//         }

        // But apparently simply copying the smaller of both sizes works just as well
        //
        size_t sizeToMap = m_byteArray.size();
        if (sizeToMap > m_mmappedSize) sizeToMap = m_mmappedSize;
        
        // copy byte array on mmapped zone
        //
        SEQMAN_DEBUG << QString("SegmentMmapper::dump : memcpy from %1 to %2 of size %3 (actual size) - mmapped size is %4 - sizeToMap : %5\n")
            .arg((unsigned int)m_byteArray.data(), 0, 16)
            .arg((unsigned int)m_mmappedBuffer, 0, 16)
            .arg(m_byteArray.size()).arg(m_mmappedSize).arg(sizeToMap);

        memcpy(m_mmappedBuffer, m_byteArray.data(), sizeToMap);
        ::msync(m_mmappedBuffer, sizeToMap, MS_ASYNC);
    }
    
}

unsigned int SegmentMmapper::getSegmentRepeatCount()
{
    int repeatCount = 0;

    timeT segmentStartTime = m_segment->getStartTime();
    timeT segmentEndTime = m_segment->getEndMarkerTime();
    timeT segmentDuration = segmentEndTime - segmentStartTime;
    timeT repeatEndTime = segmentEndTime;

    if (m_segment->isRepeating() && segmentDuration > 0) {
	repeatEndTime = m_segment->getRepeatEndTime();
	repeatCount = 1 + (repeatEndTime - segmentEndTime) / segmentDuration;
    }

    return repeatCount;
}



bool SequenceManager::event(QEvent *e)
{
    if (e->type() == QEvent::User) {
	if (m_updateRequested) {
	    checkRefreshStatus();
            m_controlBlockMmapper->refresh();
	    m_updateRequested = false;
	}
	return true;
    } else {
	return QObject::event(e);
    }
}


void SequenceManager::update()
{
    SEQMAN_DEBUG << "SequenceManager::update()\n";
    // schedule a refresh-status check for the next event loop
    QEvent *e = new QEvent(QEvent::User);
    m_updateRequested = true;
    QApplication::postEvent(this, e);
}


void SequenceManager::checkRefreshStatus()
{
    SEQMAN_DEBUG << "SequenceManager::checkRefreshStatus()\n";

//     bool regetSegments = false;
    
//     if (m_segments.empty()) {

// 	regetSegments = true;

//     } else {

// 	Rosegarden::RefreshStatus &rs =
// 	    m_doc->getComposition().getRefreshStatus
// 	    (m_compositionRefreshStatusId);

// 	if (rs.needsRefresh()) {
// 	    rs.setNeedsRefresh(false);
// 	    regetSegments = true;
// 	}
//     }

//     if (regetSegments) { // check for added and deleted segments
	
// 	SegmentSelection ss;
	
// 	for (Composition::iterator ci = m_doc->getComposition().begin();
// 	     ci != m_doc->getComposition().end(); ++ci) {
// 	    ss.insert(*ci);
// 	}

// 	for (SegmentRefreshMap::iterator si = m_segments.begin();
// 	     si != m_segments.end(); ++si) {

// 	    if (ss.find(si->first) == ss.end()) {
// 		m_segments.erase(si);
// 		SEQMAN_DEBUG << "Segment deleted, updating (now have " << m_segments.size() << " segments)" << endl;
// 		segmentRemoved(&m_doc->getComposition(), si->first);
// 	    }
// 	}

// 	for (SegmentSelection::iterator si = ss.begin();
// 	     si != ss.end(); ++si) {

// 	    if (m_segments.find(*si) == m_segments.end()) {
// 		int id = (*si)->getNewRefreshStatusId();
// 		m_segments.insert(SegmentRefreshMap::value_type(*si, id));
// 		SEQMAN_DEBUG << "Segment created, adding (now have " << m_segments.size() << " segments)" << endl;
// 		segmentAdded(&m_doc->getComposition(), *si);
// 		(*si)->getRefreshStatus(id).setNeedsRefresh(false);
// 	    }
// 	}
//     }	    

    std::vector<Segment*>::iterator i;

    // Check removed segments first
    for (i = m_removedSegments.begin(); i != m_removedSegments.end(); ++i) {
        processRemovedSegment(*i);
    }
    m_removedSegments.clear();

    // then the ones which are still there
    for (SegmentRefreshMap::iterator i = m_segments.begin();
	 i != m_segments.end(); ++i) {
	if (i->first->getRefreshStatus(i->second).needsRefresh()) {
	    segmentModified(i->first);
	    i->first->getRefreshStatus(i->second).setNeedsRefresh(false);
	}
    }

    // then added ones
    for (i = m_addedSegments.begin(); i != m_addedSegments.end(); ++i) {
        processAddedSegment(*i);
    }
    m_addedSegments.clear();
}

void SequenceManager::segmentModified(Segment* s)
{
    SEQMAN_DEBUG << "SequenceManager::segmentModified(" << s << ")\n";

    bool sizeChanged = m_compositionMmapper->segmentModified(s);

    if ((m_transportStatus == PLAYING) && sizeChanged) {

        QByteArray data;
        QDataStream streamOut(data, IO_WriteOnly);

        streamOut << m_compositionMmapper->getSegmentFileName(s);
        
        try { rgapp->sequencerSend("remapSegment(QString)", data); }
        catch(Rosegarden::Exception e) {
            // failed
            m_transportStatus = STOPPED;
            throw(e);
        }
    }
}

void SequenceManager::segmentAdded(const Composition*, Segment* s)
{
    SEQMAN_DEBUG << "SequenceManager::segmentAdded(" << s << ")\n";
    m_addedSegments.push_back(s);
}

void SequenceManager::segmentRemoved(const Composition *c, Segment* s)
{
    SEQMAN_DEBUG << "SequenceManager::segmentRemoved(" << s << ")\n";
    m_removedSegments.push_back(s);
}

void SequenceManager::processAddedSegment(Segment* s)
{
    SEQMAN_DEBUG << "SequenceManager::processAddedSegment(" << s << ")\n";

    m_compositionMmapper->segmentAdded(s);

    if (m_transportStatus == PLAYING) {

        QByteArray data;
        QDataStream streamOut(data, IO_WriteOnly);

        streamOut << m_compositionMmapper->getSegmentFileName(s);
        
        try { rgapp->sequencerSend("addSegment(QString)", data); }
        catch (Rosegarden::Exception e) {
            // failed
            m_transportStatus = STOPPED;
            throw(e);
        }
    }

    // Add to segments map
    int id = s->getNewRefreshStatusId();
    m_segments.insert(SegmentRefreshMap::value_type(s, id));

}

void SequenceManager::processRemovedSegment(Segment* s)
{
    SEQMAN_DEBUG << "SequenceManager::processRemovedSegment(" << s << ")\n";

    QString filename = m_compositionMmapper->getSegmentFileName(s);
    m_compositionMmapper->segmentDeleted(s);

    if (m_transportStatus == PLAYING) {

        QByteArray data;
        QDataStream streamOut(data, IO_WriteOnly);

        streamOut << filename;

        try { rgapp->sequencerSend("deleteSegment(QString)", data); }
        catch (Rosegarden::Exception e) {
            // failed
            m_transportStatus = STOPPED;
            throw(e);
        }
    }

    // Remove from segments map
    m_segments.erase(s);
}

void SequenceManager::endMarkerTimeChanged(const Composition *, bool /*shorten*/)
{
    // do nothing
}

void SequenceManager::compositionDeleted(const Composition *)
{
    // do nothing
}

void SequenceManager::trackChanged(const Composition *, Track* t)
{
    m_controlBlockMmapper->updateTrackData(t);
}

void
SequenceManager::sendTransportControlStatuses()
{
    KConfig* config = kapp->config();
    config->setGroup(Rosegarden::SequencerOptionsConfigGroup);

    // Get the config values
    //
    bool jackTransport = config->readBoolEntry("jacktransport", false);
    bool jackMaster = config->readBoolEntry("jackmaster", false);

    bool mmcTransport = config->readBoolEntry("mmctransport", false);
    bool mmcMaster = config->readBoolEntry("mmcmaster", false);

    bool midiClock = config->readBoolEntry("midiclock", false);

    // Send JACK transport
    //
    int jackValue = 0;
    if (jackTransport && jackMaster)
        jackValue = 2;
    else 
    {
        if (jackTransport)
            jackValue = 1;
        else
            jackValue = 0;
    }

    Rosegarden::MappedEvent *mE =
        new Rosegarden::MappedEvent(
            Rosegarden::MidiInstrumentBase, // InstrumentId
            Rosegarden::MappedEvent::SystemJackTransport,
            Rosegarden::MidiByte(jackValue));
    Rosegarden::StudioControl::sendMappedEvent(mE);


    // Send MMC transport
    //
    int mmcValue = 0;
    if (mmcTransport && mmcMaster)
        mmcValue = 2;
    else
    {
        if (mmcTransport)
            mmcValue = 1;
        else
            mmcValue = 0;
    }

    mE = new Rosegarden::MappedEvent(
                Rosegarden::MidiInstrumentBase, // InstrumentId
                Rosegarden::MappedEvent::SystemMMCTransport,
                Rosegarden::MidiByte(mmcValue));

    Rosegarden::StudioControl::sendMappedEvent(mE);


    // Send MIDI Clock
    //
    mE = new Rosegarden::MappedEvent(
                Rosegarden::MidiInstrumentBase, // InstrumentId
                Rosegarden::MappedEvent::SystemMIDIClock,
                Rosegarden::MidiByte(midiClock));

    Rosegarden::StudioControl::sendMappedEvent(mE);



}

void
SequenceManager::slotCountdownCancelled()
{
    SEQMAN_DEBUG << "SequenceManager::slotCountdownCancelled - "
                 << "stopping" << endl;

    // stop timer
    m_countdownTimer->stop();
    m_countdownDialog->hide();

    // stop recording
    stopping();
}

void
SequenceManager::slotCountdownTimerTimeout()
{
    // Set the elapsed time in seconds
    //
    m_countdownDialog->setElapsedTime(m_recordTime->elapsed() / 1000);
}

// The countdown has completed - stop recording
//
void
SequenceManager::slotCountdownStop()
{
    SEQMAN_DEBUG << "SequenceManager::slotCountdownStop - "
                 << "countdown timed out - automatically stopping recording"
                 << endl;

    stopping(); // erm - simple as that
}

}
