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
#include <qcursor.h>
#include <qtimer.h>

#include <klocale.h>
#include <kconfig.h>
#include <kstddirs.h>
#include <kmessagebox.h>
#include <kstartuplogo.h>

#include <sys/time.h>

#include "rgapplication.h"
#include "constants.h"
#include "audiopluginmanager.h"
#include "ktmpstatusmsg.h"
#include "rosegardenguidoc.h"
#include "rosegardentransportdialog.h"
#include "rosegardenguiview.h"
#include "sequencemanager.h"
#include "SoundDriver.h"
#include "BaseProperties.h"
#include "studiocontrol.h"
#include "MidiDevice.h"
#include "rosestrings.h"
#include "mmapper.h"
#include "sequencermapper.h"

#include "Profiler.h"
#ifdef QUERY_PLUGINS_FROM_GUI
#include "PluginFactory.h"
#endif


namespace Rosegarden
{

//----------------------------------------

SequenceManager::SequenceManager(RosegardenGUIDoc *doc,
                                 RosegardenTransportDialog *transport):
    m_doc(doc),
    m_compositionMmapper(new CompositionMmapper(m_doc)),
    m_controlBlockMmapper(new ControlBlockMmapper(m_doc)),
    m_metronomeMmapper(SegmentMmapperFactory::makeMetronome(m_doc)),
    m_tempoSegmentMmapper(SegmentMmapperFactory::makeTempo(m_doc)),
    m_timeSigSegmentMmapper(SegmentMmapperFactory::makeTimeSig(m_doc)),
    m_transportStatus(STOPPED),
    m_soundDriverStatus(NO_DRIVER),
    m_transport(transport),
    m_lastRewoundAt(clock()),
    m_countdownDialog(0),
    m_countdownTimer(new QTimer(doc)),
    m_shownOverrunWarning(false),
    m_recordTime(new QTime()),
    m_compositionRefreshStatusId(m_doc->getComposition().getNewRefreshStatusId()),
    m_updateRequested(true),
    m_sequencerMapper(0),
    m_reportTimer(new QTimer(doc)),
    m_canReport(true),
    m_lastLowLatencySwitchSent(false)
{
    // Replaced this with a call to cleanup() from composition mmapper ctor:
    // if done here, this removes the mmapped versions of any segments stored
    // in the autoload (that have only just been mapped by the ctor!)
//    m_compositionMmapper->cleanup();

    m_countdownDialog = new CountdownDialog(dynamic_cast<QWidget*>
                                (m_doc->parent())->parentWidget());
    // Connect these for use later
    //
    connect(m_countdownTimer, SIGNAL(timeout()),
            this, SLOT(slotCountdownTimerTimeout()));

    m_reportTimer->stop();
    connect(m_reportTimer, SIGNAL(timeout()),
            this, SLOT(slotAllowReport()));

    connect(doc->getCommandHistory(), SIGNAL(commandExecuted()),
	    this, SLOT(update()));

    m_doc->getComposition().addObserver(this);

    // This check may throw an exception
    checkSoundDriverStatus();

    // Try to map the sequencer file
    //
    mapSequencer();
}


SequenceManager::~SequenceManager()
{
    m_doc->getComposition().removeObserver(this);

    SEQMAN_DEBUG << "SequenceManager::~SequenceManager()\n";   
    delete m_compositionMmapper;
    delete m_controlBlockMmapper;
    delete m_metronomeMmapper;
    delete m_tempoSegmentMmapper;
    delete m_timeSigSegmentMmapper;
    delete m_sequencerMapper;
}

void SequenceManager::setDocument(RosegardenGUIDoc* doc)
{
    SEQMAN_DEBUG << "SequenceManager::setDocument(" << doc << ")\n";

    DataBlockRepository::clear();

    m_doc->getComposition().removeObserver(this);
    disconnect(m_doc->getCommandHistory(), SIGNAL(commandExecuted()));
    
    m_segments.clear();
    m_triggerSegments.clear();

    m_doc = doc;
    Composition &comp = m_doc->getComposition();

    // Must recreate and reconnect the countdown timer and dialog
    // (bug 729039)
    //
    delete m_countdownDialog;
    delete m_countdownTimer;

    m_countdownDialog = new CountdownDialog(dynamic_cast<QWidget*>
                                (m_doc->parent())->parentWidget());

    // Bug 933041: no longer connect the CountdownDialog from
    // SequenceManager; instead let the RosegardenGUIApp connect it to
    // its own slotStop to ensure the right housekeeping is done

    m_countdownTimer = new QTimer(m_doc);

    // Connect this for use later
    //
    connect(m_countdownTimer, SIGNAL(timeout()),
            this, SLOT(slotCountdownTimerTimeout()));

    if (m_doc) {
	m_compositionRefreshStatusId = comp.getNewRefreshStatusId();
        comp.addObserver(this);

        connect(m_doc->getCommandHistory(), SIGNAL(commandExecuted()),
                this, SLOT(update()));

	for (Composition::iterator i = comp.begin(); i != comp.end(); ++i) {

	    SEQMAN_DEBUG << "Adding segment with rid " << (*i)->getRuntimeId() << endl;

	    m_segments.insert(SegmentRefreshMap::value_type
			      (*i, (*i)->getNewRefreshStatusId()));
	}

	for (Composition::triggersegmentcontaineriterator i =
		 comp.getTriggerSegments().begin();
	     i != comp.getTriggerSegments().end(); ++i) {
	    m_triggerSegments.insert(SegmentRefreshMap::value_type
				     ((*i)->getSegment(),
				      (*i)->getSegment()->getNewRefreshStatusId()));
	}
    }

    resetCompositionMmapper();

    // Try to map the sequencer file
    //
    mapSequencer();
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
SequenceManager::mapSequencer()
{
    if (m_sequencerMapper) return;

    try
    {
        m_sequencerMapper = new SequencerMapper(
            KGlobal::dirs()->resourceDirs("tmp").last() + "/rosegarden_sequencer_timing_block");
    }
    catch(Exception)
    {
        m_sequencerMapper = 0;
    }
}

bool
SequenceManager::play()
{
    mapSequencer();

    Composition &comp = m_doc->getComposition();
  
    // If already playing or recording then stop
    //
    if (m_transportStatus == PLAYING ||
        m_transportStatus == RECORDING_MIDI ||
        m_transportStatus == RECORDING_AUDIO )
        {
            stopping();
            return true;
        }

    // This check may throw an exception
    checkSoundDriverStatus();

    // Align Instrument lists and send initial program changes
    //
    preparePlayback();

    // Update play metronome status
    // 
    m_controlBlockMmapper->updateMetronomeData
	(m_metronomeMmapper->getMetronomeInstrument());
    m_controlBlockMmapper->updateMetronomeForPlayback();

    // make sure we toggle the play button
    // 
    m_transport->PlayButton()->setOn(true);

    if (comp.getTempo() == 0) {
        comp.setDefaultTempo(120.0);

        SEQMAN_DEBUG << "SequenceManager::play() - setting Tempo to Default value of 120.000\n";
    } else {
            SEQMAN_DEBUG << "SequenceManager::play() - starting to play\n";
    }

    // Send initial tempo
    //
    double qnD = 60.0/comp.getTempo();
    RealTime qnTime =
        RealTime(long(qnD), 
                long((qnD - double(long(qnD))) * 1000000000.0));
    StudioControl::sendQuarterNoteLength(qnTime);

    // set the tempo in the transport
    m_transport->setTempo(comp.getTempo());

    // The arguments for the Sequencer
    RealTime startPos = comp.getElapsedRealTime(comp.getPosition());

    // If we're looping then jump to loop start
    if (comp.isLooping())
        startPos = comp.getElapsedRealTime(comp.getLoopStart());

    KConfig* config = kapp->config();
    config->setGroup(SequencerOptionsConfigGroup);

    bool lowLat = config->readBoolEntry("audiolowlatencymonitoring", true);

    if (lowLat != m_lastLowLatencySwitchSent) {

	QByteArray data;
	QDataStream streamOut(data, IO_WriteOnly);
	streamOut << lowLat;

	rgapp->sequencerSend("setLowLatencyMode(bool)", data);
	m_lastLowLatencySwitchSent = lowLat;
    }

    QByteArray data;
    QCString replyType;
    QByteArray replyData;
    QDataStream streamOut(data, IO_WriteOnly);

    // playback start position
    streamOut << (long)startPos.sec;
    streamOut << (long)startPos.nsec;

    // Apart from perhaps the small file size, I think with hindsight
    // that these options are more easily set to reasonable defaults
    // here than left to the user.  Mostly.

    //!!! need some cleverness somewhere to ensure the read-ahead
    //is larger than the JACK period size

    if (lowLat) {
	streamOut << 0L; // read-ahead sec
	streamOut << 160000000L; // read-ahead nsec
	streamOut << 0L; // audio mix sec
	streamOut << 60000000L; // audio mix nsec: ignored in lowlat mode
	streamOut << 2L; // audio read sec
	streamOut << 500000000L; // audio read nsec
	streamOut << 4L; // audio write sec
	streamOut << 0L; // audio write nsec
	streamOut << 256L; // cacheable small file size in K
    } else {
	streamOut << 0L; // read-ahead sec
	streamOut << 500000000L; // read-ahead nsec
	streamOut << 0L; // audio mix sec
	streamOut << 400000000L; // audio mix nsec
	streamOut << 2L; // audio read sec
	streamOut << 500000000L; // audio read nsec
	streamOut << 4L; // audio write sec
	streamOut << 0L; // audio write nsec
	streamOut << 256L; // cacheable small file size in K
    }

    // Send Play to the Sequencer
    if (!rgapp->sequencerCall("play(long int, long int, long int, long int, long int, long int, long int, long int, long int, long int, long int)",
                              replyType, replyData, data)) {
        m_transportStatus = STOPPED;
        return false;
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
        throw(Exception("Failed to start playback"));
    }

    return false;
}

void
SequenceManager::stopping()
{
    if (m_countdownTimer) m_countdownTimer->stop();
    if (m_countdownDialog) m_countdownDialog->hide();

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

    m_shownOverrunWarning = false;
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

        // Remove the countdown dialog and stop the timer
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

    QCString replyType;
    QByteArray replyData;

    bool failed = false;
    if (!rgapp->sequencerCall("stop()", replyType, replyData)) {
	failed = true;
    }

    // restore
    QApplication::restoreOverrideCursor();

    TransportStatus status = m_transportStatus;
    
    // set new transport status first, so that if we're stopping
    // recording we don't risk the record segment being restored by a
    // timer while the document is busy trying to do away with it
    m_transportStatus = STOPPED;

    // if we're recording MIDI or Audio then tidy up the recording Segment
    if (status == RECORDING_MIDI)
    {
        m_doc->stopRecordingMidi();

        SEQMAN_DEBUG << "SequenceManager::stop() - stopped recording MIDI\n";
    }

    if (status == RECORDING_AUDIO)
    {
        m_doc->stopRecordingAudio();
        SEQMAN_DEBUG << "SequenceManager::stop() - stopped recording audio\n";
    }

    // always untoggle the play button at this stage
    //
    m_transport->PlayButton()->setOn(false);
    SEQMAN_DEBUG << "SequenceManager::stop() - stopped playing\n";

    // We don't reset controllers at this point - what happens with static 
    // controllers the next time we play otherwise?  [rwb]
    //resetControllers();

    if (failed) {
        throw(Exception("Failed to contact Rosegarden sequencer with stop command.   Please save your composition and restart Rosegarden to continue."));
    }
}

// Jump to previous bar
//
void
SequenceManager::rewind()
{
    Composition &composition = m_doc->getComposition();

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
    Composition &composition = m_doc->getComposition();

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
// The Sequencer replies when ready with its status.  If anything
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
SequenceManager::sendSequencerJump(const RealTime &time)
{
    QByteArray data;
    QDataStream streamOut(data, IO_WriteOnly);
    streamOut << (long)time.sec;
    streamOut << (long)time.nsec;

    rgapp->sequencerSend("jumpTo(long int, long int)", data);
}



// Called when we want to start recording from the GUI.
// This method tells the sequencer to start recording and
// from then on the sequencer returns MappedCompositions
// to the GUI via the sequencer mmapper
// also called via DCOP
//
//

void
SequenceManager::record(bool toggled)
{
    mapSequencer();

    Composition &comp = m_doc->getComposition();
    Studio &studio = m_doc->getStudio();
    KConfig* config = kapp->config();
    config->setGroup(GeneralOptionsConfigGroup);

    bool punchIn = false; // are we punching in?

    // Rather clumsy additional check for audio subsys when we start
    // recording - once we enforce audio subsystems then this will
    // become redundant.
    //
    if (!(m_soundDriverStatus & AUDIO_OK)) {
        int rID = comp.getRecordTrack();
        InstrumentId instrId =
            comp.getTrackById(rID)->getInstrument();
        Instrument *instr = studio.getInstrumentById(instrId);

        if (!instr || instr->getType() == Instrument::Audio) {
            m_transport->RecordButton()->setOn(false);
            throw(Exception("Audio subsystem is not available - can't record audio"));
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

            QByteArray data;
            QCString replyType;
            QByteArray replyData;

            // Send Record to the Sequencer to signal it to drop out of record mode
            //
	    //!!! huh? this doesn't look very plausible
            if (!rgapp->sequencerCall("play(long int, long int, long int, long int, long int, long int, long int, long int, long int, long int, long int)",
                                  replyType, replyData, data))
            {
                m_transportStatus = STOPPED;
                return;
            }

            m_doc->stopRecordingMidi();
            m_transportStatus = PLAYING;

            return;
        }

        if (m_transportStatus == PLAYING) {
            SEQMAN_DEBUG << "SequenceManager::record - punch in recording\n";
            punchIn = true;
            goto punchin;
        }

    } else {

punchin:

        // Get the record track and check the Instrument type
        int rID = comp.getRecordTrack();
        InstrumentId inst =
            comp.getTrackById(rID)->getInstrument();

        // If no matching record instrument
        //
        if (studio.getInstrumentById(inst) == 0) {
            m_transport->RecordButton()->setDown(false);
            throw(Exception("No Record instrument selected"));
        }


        // may throw an exception
        checkSoundDriverStatus();

        // toggle the Metronome button if it's in use
        m_transport->MetronomeButton()->setOn(comp.useRecordMetronome());

	// Update record metronome status
	// 
	m_controlBlockMmapper->updateMetronomeData
	    (m_metronomeMmapper->getMetronomeInstrument());
	m_controlBlockMmapper->updateMetronomeForRecord();

        // If we are looping then jump to start of loop and start recording,
        // if we're not take off the number of count-in bars and start 
        // recording.
        //
        if(comp.isLooping())
            m_doc->setPointerPosition(comp.getLoopStart());
        else {
            if (m_transportStatus != RECORDING_ARMED && punchIn == false) {
                int startBar = comp.getBarNumber(comp.getPosition());
                startBar -= config->readUnsignedNumEntry("countinbars", 2);
                m_doc->setPointerPosition(comp.getBarRange(startBar).first);
            }
        }

	m_doc->setRecordStartTime(m_doc->getComposition().getPosition());

        // Some locals
        //
        TransportStatus recordType;

        switch (studio.getInstrumentById(inst)->getType()) {

        case Instrument::Midi:
	case Instrument::SoftSynth:
            recordType = STARTING_TO_RECORD_MIDI;
            SEQMAN_DEBUG << "SequenceManager::record() - starting to record MIDI\n";
            break;

        case Instrument::Audio: {

//             AudioFileManager &afm = m_doc->getAudioFileManager();
//             QString mountPoint = KIO::findPathMountPoint(strtoqstr(afm.getAudioPath()));
//             KDiskFreeSp * job = new KDiskFreeSp;
//             connect(job, SIGNAL(foundMountPoint(const QString&, unsigned long, unsigned long,
//                                                 unsigned long)),
//                     this, SLOT(slotFoundMountPoint(const QString&, unsigned long, unsigned long,
//                                                    unsigned long)));
//             m_gotDiskSpaceResult = false;
//             job->readDF(mountPoint);
//             while (!m_gotDiskSpaceResult) {
//                 rgapp::refreshGUI(50);
//             }
            

            // Check the disk space available is within current
            // audio recording limit
            //
//             config->setGroup(SequencerOptionsConfigGroup);

	    // Ask the document to update its record latencies so as to
	    // do latency compensation when we stop
	    m_doc->updateAudioRecordLatency();

            recordType = STARTING_TO_RECORD_AUDIO;
            SEQMAN_DEBUG << "SequenceManager::record() - "
                         << "starting to record Audio\n";
            break;
        }
            
        default:
            SEQMAN_DEBUG << "SequenceManager::record() - unrecognised instrument type " << int(studio.getInstrumentById(inst)->getType()) << " for instrument " << inst << "\n";
            return;
            break;
        }

        // set the buttons
        m_transport->RecordButton()->setOn(true);
        m_transport->PlayButton()->setOn(true);

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
        RealTime startPos =
            comp.getElapsedRealTime(comp.getPosition());

	bool lowLat = config->readBoolEntry("audiolowlatencymonitoring", true);

	if (lowLat != m_lastLowLatencySwitchSent) {

	    QByteArray data;
	    QDataStream streamOut(data, IO_WriteOnly);
	    streamOut << lowLat;

	    rgapp->sequencerSend("setLowLatencyMode(bool)", data);
	    m_lastLowLatencySwitchSent = lowLat;
	}

        QByteArray data;
        QCString replyType;
        QByteArray replyData;
        QDataStream streamOut(data, IO_WriteOnly);

        // playback start position
        streamOut << (long)startPos.sec;
        streamOut << (long)startPos.nsec;

	// Apart from perhaps the small file size, I think with hindsight
	// that these options are more easily set to reasonable defaults
	// here than left to the user.  Mostly.
	
	//!!! Duplicates code in play()

	//!!! need some cleverness somewhere to ensure the read-ahead
	//is larger than the JACK period size

	if (lowLat) {
	    streamOut << 0L; // read-ahead sec
	    streamOut << 160000000L; // read-ahead nsec
	    streamOut << 0L; // audio mix sec
	    streamOut << 60000000L; // audio mix nsec: ignored in lowlat mode
	    streamOut << 2L; // audio read sec
	    streamOut << 500000000L; // audio read nsec
	    streamOut << 4L; // audio write sec
	    streamOut << 0L; // audio write nsec
	    streamOut << 256L; // cacheable small file size in K
	} else {
	    streamOut << 0L; // read-ahead sec
	    streamOut << 500000000L; // read-ahead nsec
	    streamOut << 0L; // audio mix sec
	    streamOut << 400000000L; // audio mix nsec
	    streamOut << 2L; // audio read sec
	    streamOut << 500000000L; // audio read nsec
	    streamOut << 4L; // audio write sec
	    streamOut << 0L; // audio write nsec
	    streamOut << 256L; // cacheable small file size in K
	}

        // record type
        streamOut << (long)recordType;
    
        // Send Play to the Sequencer
        if (!rgapp->sequencerCall("record(long int, long int, long int, long int, long int, long int, long int, long int, long int, long int, long int, long int)",
                                  replyType, replyData, data)) {
            // failed
            m_transportStatus = STOPPED;
            return;
        }

        // ensure the return type is ok
        QDataStream streamIn(replyData, IO_ReadOnly);
        int result;
        streamIn >> result;
  
        if (result) {
            // completed successfully 
            m_transportStatus = recordType;

	    // Create the countdown timer dialog to show recording time
	    // remaining.  (Note (dmm) this has changed, and it now reports
	    // the time remaining during both MIDI and audio recording.)
	    //	
	    Rosegarden::timeT p = comp.getPosition(); 
	    Rosegarden::timeT d = comp.getEndMarker();
	    // end marker less current position == available duration 
	    d -= p;
	    
	    // set seconds to total possible time, initially
	    Rosegarden::RealTime rtd = comp.getElapsedRealTime(d);
	    int seconds = rtd.sec;

	    /* #1045380 ("minutes of audio recording" just insanely
	       confusing) -- No, let's not use this.  We should count
	       to the end of the composition in both cases.

	    // if we're recording audio, and if the audio recording limit is
	    // less than the total available time, adjust the time down to the
	    // audio limit
	    if (recordType == STARTING_TO_RECORD_AUDIO) {
		KConfig* config = kapp->config();
		config->setGroup(SequencerOptionsConfigGroup);

		int s = 60 * 
		    (config->readNumEntry("audiorecordminutes", 5));
		if (s < seconds) seconds = s;
	    }
	    */

	    // re-initialise
	    m_countdownDialog->setTotalTime(seconds);

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
        } else {
            // Stop immediately - turn off buttons in parent
            //
            m_transportStatus = STOPPED;

            if (recordType == STARTING_TO_RECORD_AUDIO) {
                throw(Exception("Couldn't start recording audio.\nPlease set a valid file path in the Document Properties\n(Composition menu -> Edit Document Properties -> Audio)."));
            } else {
                throw(Exception("Couldn't start recording MIDI"));
            }

        }
    }
}


// Process unexpected MIDI events at the GUI - send them to the
// Transport, or to a MIDI mixer for display purposes, or to an editor
// for use in step recording.
//
void
SequenceManager::processAsynchronousMidi(const MappedComposition &mC,
                                         AudioManagerDialog
                                             *audioManagerDialog)
{
    static bool boolShowingWarning = false;
    static bool boolShowingALSAWarning = false;
    static long warningShownAt = 0;

    if (m_doc == 0 || mC.size() == 0) return;

    MappedComposition::const_iterator i;

    // Thru filtering is done at the sequencer for the actual sound
    // output, but here we need both filtered (for OUT display) and
    // unfiltered (for insertable note callbacks) compositions, so
    // we've received the unfiltered copy and will filter here
    MappedComposition tempMC =
	applyFiltering(mC,
		       MappedEvent::MappedEventType(
			   m_doc->getStudio().getMIDIThruFilter()));
    
    // send to the MIDI labels (which can only hold one event at a time)
    i = mC.begin();
    if (i != mC.end()) {
	m_transport->setMidiInLabel(*i);
    }

    i = tempMC.begin();
    if (i != tempMC.end()) {
	m_transport->setMidiOutLabel(*i);
    }

    for (i = mC.begin(); i != mC.end(); ++i )
    {
	if ((*i)->getType() >= MappedEvent::Audio)
	{
	    if ((*i)->getType() == MappedEvent::AudioStopped)
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
			    AudioFileId((*i)->getData1()));
		}
	    }
	    
	    if ((*i)->getType() == MappedEvent::AudioLevel)
		sendAudioLevel(*i);
	    
	    if ((*i)->getType() == 
		MappedEvent::AudioGeneratePreview)
	    {
		m_doc->finalizeAudioFile(
		    AudioFileId((*i)->getData1()));
	    }
	    
	    if ((*i)->getType() ==
		MappedEvent::SystemUpdateInstruments)
	    {
		// resync Devices and Instruments
		//
		m_doc->syncDevices();
		
		/*KConfig* config = kapp->config();
    		config->setGroup(SequencerOptionsConfigGroup);
		QString recordDeviceStr = config->readEntry("midirecorddevice");
		sendMIDIRecordingDevice(recordDeviceStr);*/
		restoreRecordSubscriptions();
	    }

            if (m_transportStatus == PLAYING ||
                m_transportStatus == RECORDING_MIDI ||
                m_transportStatus == RECORDING_AUDIO)
            {
		if ((*i)->getType() == MappedEvent::SystemFailure) {

		    SEQMAN_DEBUG << "Failure of some sort..." << endl;

                    // If we get any sort of audio failure and we're recording
                    // audio then assume we want to stop.  Usually this'll
                    // ruin our recording.
                    //
                    /*
                    if (m_transportStatus == RECORDING_AUDIO)
                    {
                        stopping();

                        KMessageBox::error(
                            dynamic_cast<QWidget*>
                            (m_doc->parent())->parentWidget(),
                            i18n("Audio glitch during recording.  Stopping."));
                    }
                    */

		    bool handling = true;

		    /* These are the ones that we always report or handle. */
		    
		    if ((*i)->getData1() == MappedEvent::FailureJackDied) {

			// Something horrible has happened to JACK or we got
			// bumped out of the graph.  Either way stop playback.
			//
			stopping();

		    } else if ((*i)->getData1() == MappedEvent::FailureJackRestartFailed) {

			KMessageBox::error(
			    dynamic_cast<QWidget*>(m_doc->parent())->parentWidget(),
			    i18n("The JACK Audio subsystem has failed or it has stopped Rosegarden from processing audio.\nPlease restart Rosegarden to continue working with audio.\nQuitting other running applications may improve Rosegarden's performance."));

		    } else if ((*i)->getData1() == MappedEvent::FailureJackRestart) {

			KMessageBox::error(
			    dynamic_cast<QWidget*>(m_doc->parent())->parentWidget(),
			    i18n("The JACK Audio subsystem has stopped Rosegarden from processing audio, probably because of a processing overload.\nAn attempt to restart the audio service has been made, but some problems may remain.\nQuitting other running applications may improve Rosegarden's performance."));

		    } else if ((*i)->getData1() == MappedEvent::FailureCPUOverload) {

#define REPORT_CPU_OVERLOAD 1
#ifdef REPORT_CPU_OVERLOAD

			stopping();

			KMessageBox::error(
			    dynamic_cast<QWidget*>(m_doc->parent())->parentWidget(),
			    i18n("Run out of processor power for real-time audio processing.  Cannot continue."));

#endif
		    } else {
		    
		    	handling = false;
	 	    }
		    
		    if (handling) continue;
		    
                    if (!m_canReport)
                    {
                        SEQMAN_DEBUG << "Not reporting it to user just yet" 
                                     << endl;
                        continue;
                    }

		    if ((*i)->getData1() == MappedEvent::FailureALSACallFailed) {
			
			struct timeval tv;
			(void)gettimeofday(&tv, 0);

			if (tv.tv_sec - warningShownAt >= 5 &&
			    !boolShowingALSAWarning) {

			    QString message = i18n("A serious error has occurred in the ALSA MIDI subsystem.  It may not be possible to continue sequencing.  Please check console output for more information.");
			    boolShowingALSAWarning = true;
		    
			    KMessageBox::information(0, message);
			    boolShowingALSAWarning = false;

			    (void)gettimeofday(&tv, 0);
			    warningShownAt = tv.tv_sec;
			}

		    } else if ((*i)->getData1() == MappedEvent::FailureXRuns) {

//#define REPORT_XRUNS 1
#ifdef REPORT_XRUNS

			struct timeval tv;
			(void)gettimeofday(&tv, 0);

			if (tv.tv_sec - warningShownAt >= 5 &&
			    !boolShowingWarning) {

			    QString message = i18n("JACK Audio subsystem is losing sample frames.");
			    boolShowingWarning = true;
		    
			    KMessageBox::information(0, message);
			    boolShowingWarning = false;

			    (void)gettimeofday(&tv, 0);
			    warningShownAt = tv.tv_sec;
			}
#endif

		    } else if (!m_shownOverrunWarning) {
			
			QString message;
			    
			switch ((*i)->getData1()) {
			    
			case MappedEvent::FailureDiscUnderrun:
			    message = i18n("Failed to read audio data from disc in time to service the audio subsystem.");
			    break;

			case MappedEvent::FailureDiscOverrun:
			    message = i18n("Failed to write audio data to disc fast enough to service the audio subsystem.");
			    break;

			case MappedEvent::FailureBussMixUnderrun:
			    message = i18n("The audio mixing subsystem is failing to keep up.");
			    break;

			case MappedEvent::FailureMixUnderrun:
			    message = i18n("The audio subsystem is failing to keep up.");
			    break;

			default:
			    message = i18n("Unknown sequencer failure mode!");
			    break;
			}

			m_shownOverrunWarning = true;

#ifdef REPORT_XRUNS
			KMessageBox::information(0, message);
#else
			if ((*i)->getData1() == MappedEvent::FailureDiscOverrun) {
			    // the error you can't hear
			    KMessageBox::information(0, message);
			} else {
			    std::cerr << message << std::endl;
			}
#endif
		    }

                    // Turn off the report flag and set off a one-shot
                    // timer for 5 seconds.
                    //
                    if (!m_reportTimer->isActive())
                    {
                        m_canReport = false;
                        m_reportTimer->start(5000, true);
                    }
		}
	    } else {
		KStartupLogo::hideIfStillThere();

		if ((*i)->getType() == MappedEvent::SystemFailure) {

		    if ((*i)->getData1() == MappedEvent::FailureJackRestartFailed) {

			KMessageBox::error(
			    dynamic_cast<QWidget*>(m_doc->parent())->parentWidget(),
			    i18n("The JACK Audio subsystem has failed or it has stopped Rosegarden from processing audio.\nPlease restart Rosegarden to continue working with audio.\nQuitting other running applications may improve Rosegarden's performance."));

		    } else if ((*i)->getData1() == MappedEvent::FailureJackRestart) {

			KMessageBox::error(
			    dynamic_cast<QWidget*>(m_doc->parent())->parentWidget(),
			    i18n("The JACK Audio subsystem has stopped Rosegarden from processing audio, probably because of a processing overload.\nAn attempt to restart the audio service has been made, but some problems may remain.\nQuitting other running applications may improve Rosegarden's performance."));

		    } else if ((*i)->getData1() == MappedEvent::WarningImpreciseTimer) {
			std::cerr << "Rosegarden: WARNING: No accurate sequencer timer available" << std::endl;
		    }
		}
	    }
        }
    }
    
    // if we aren't playing or recording, consider invoking any
    // step-by-step clients (using unfiltered composition)
    
    if (m_transportStatus == STOPPED ||
	m_transportStatus == RECORDING_ARMED) {

	for (i = mC.begin(); i != mC.end(); ++i )
	{
	    if ((*i)->getType() == MappedEvent::MidiNote) {
		if ((*i)->getVelocity() == 0) {
		    emit insertableNoteOffReceived((*i)->getPitch(), (*i)->getVelocity());
		} else {
		    emit insertableNoteOnReceived((*i)->getPitch(), (*i)->getVelocity());
		}
	    }
	}
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

    RealTime loopStart =
            m_doc->getComposition().getElapsedRealTime(lhs);
    RealTime loopEnd =
            m_doc->getComposition().getElapsedRealTime(rhs);

    streamOut << (long)loopStart.sec;
    streamOut << (long)loopStart.nsec;
    streamOut << (long)loopEnd.sec;
    streamOut << (long)loopEnd.nsec;
  
    rgapp->sequencerSend("setLoop(long int, long int, long int, long int)", data);
}

void
SequenceManager::checkSoundDriverStatus()
{
    QCString replyType;
    QByteArray replyData;

    if (! rgapp->sequencerCall("getSoundDriverStatus()", replyType, replyData)) {
	m_soundDriverStatus = NO_DRIVER;
        return;
    }

    QDataStream streamIn(replyData, IO_ReadOnly);
    unsigned int result;
    streamIn >> result;
    m_soundDriverStatus = result;

    if (m_soundDriverStatus == NO_DRIVER)
        throw(Exception("MIDI and Audio subsystems have failed to initialise"));

    if (!(m_soundDriverStatus & MIDI_OK))
        throw(Exception("MIDI subsystem has failed to initialise"));

    /*
      if (!(m_soundDriverStatus & AUDIO_OK))
      throw(Exception("Audio subsystem has failed to initialise"));
    */
}

// Send Instrument list to Sequencer and ensure that initial program
// changes follow them.  Sending the instruments ensures that we have
// channels available on the Sequencer and then the program changes
// are sent to those specific channel (referenced by Instrument ID)
//
void
SequenceManager::preparePlayback(bool forceProgramChanges)
{
    Studio &studio = m_doc->getStudio();
    InstrumentList list = studio.getAllInstruments();
    MappedComposition mC;
    MappedEvent *mE;

    // Send the MappedInstruments (minimal Instrument information
    // required for Performance) to the Sequencer
    //
    InstrumentList::iterator it = list.begin();
    for (; it != list.end(); it++)
    {
        StudioControl::sendMappedInstrument(MappedInstrument(*it));

        // Send program changes for MIDI Instruments
        //
        if ((*it)->getType() == Instrument::Midi)
        {
            // send bank select always before program change
            //
            if ((*it)->sendsBankSelect())
            {
                mE = new MappedEvent((*it)->getId(),
                                     MappedEvent::MidiController,
                                     MIDI_CONTROLLER_BANK_MSB,
                                     (*it)->getMSB());
                mC.insert(mE);

                mE = new MappedEvent((*it)->getId(),
                                     MappedEvent::MidiController,
                                     MIDI_CONTROLLER_BANK_LSB,
                                     (*it)->getLSB());
                mC.insert(mE);
            }

            // send program change
            //
            if ((*it)->sendsProgramChange() || forceProgramChanges)
            {
                RG_DEBUG << "SequenceManager::preparePlayback() : sending prg change for "
                         << (*it)->getPresentationName().c_str() << endl;

                mE = new MappedEvent((*it)->getId(),
                                     MappedEvent::MidiProgramChange,
                                     (*it)->getProgramChange());
                mC.insert(mE);
            }

        }
        else if ((*it)->getType() == Instrument::Audio ||
		 (*it)->getType() == Instrument::SoftSynth)
        {
        }
        else
        {
            RG_DEBUG << "SequenceManager::preparePlayback - "
                     << "unrecognised instrument type" << endl;
        }


    }

    // Send the MappedComposition if it's got anything in it
    showVisuals(mC);
    StudioControl::sendMappedComposition(mC);
}

void
SequenceManager::sendAudioLevel(MappedEvent *mE)
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

    // Should do all Midi Instrument - not just guess like this is doing
    // currently.

    InstrumentList list = m_doc->getStudio().getPresentationInstruments();
    InstrumentList::iterator it;

    MappedComposition mC;

    for (it = list.begin(); it != list.end(); it++)
    {
        if ((*it)->getType() == Instrument::Midi)
        {
            MappedEvent *mE = new MappedEvent((*it)->getId(),
					      MappedEvent::MidiController,
					      MIDI_CONTROLLER_RESET,
					      0);
	    mC.insert(mE);
        }
    }

    StudioControl::sendMappedComposition(mC);
    //showVisuals(mC);
}

void
SequenceManager::resetMidiNetwork()
{
    SEQMAN_DEBUG << "SequenceManager::resetMidiNetwork - resetting\n";
    MappedComposition mC;

    // Should do all Midi Instrument - not just guess like this is doing
    // currently.

    for (unsigned int i = 0; i < 16; i++)
    {
        MappedEvent *mE =
            new MappedEvent(MidiInstrumentBase + i,
                                        MappedEvent::MidiController,
                                        MIDI_SYSTEM_RESET,
                                        0);

        mC.insert(mE);
    }
    showVisuals(mC);
    StudioControl::sendMappedComposition(mC);
}

void
SequenceManager::getSequencerPlugins(AudioPluginManager *aPM)
{
    //!!! At this point we might be better off querying identifier
    // and category only... and then filling in the blanks when we
    // actually want to display the thing.

    SEQMAN_DEBUG << "getSequencerPlugins - getting plugin information" << endl;

    MappedObjectPropertyList seqPlugins
#ifdef QUERY_PLUGINS_FROM_GUI
	(50000)
#endif
	;

    {
	Rosegarden::Profiler profiler("querying plugins", true);

#ifdef QUERY_PLUGINS_FROM_GUI

	if (!rgapp->noSequencerMode()) {

	    // We only waste the time looking for plugins here if we
	    // know we're actually going to be able to use them.
	    // Otherwise fall back to querying the sequencer for them,
	    // which will almost certainly fail (because we just
	    // established it wasn't running) but at least will fail
	    // in a way consistent with the old
	    // non-QUERY_PLUGINS_FROM_GUI behaviour.

	    PluginFactory::enumerateAllPlugins(seqPlugins);

	    SEQMAN_DEBUG << "got " << seqPlugins.size() << " pieces of plugin data at GUI side" << endl;

	} else {
	    seqPlugins = StudioControl::getPluginInformation();
	}
#else

	seqPlugins = StudioControl::getPluginInformation();

#endif
    }

    unsigned int i = 0;

    while (i < seqPlugins.size())
    {
        QString identifier = seqPlugins[i++];
        QString name = seqPlugins[i++];
        unsigned long uniqueId = seqPlugins[i++].toLong();
        QString label = seqPlugins[i++];
        QString author = seqPlugins[i++];
        QString copyright = seqPlugins[i++];
	bool isSynth = ((seqPlugins[i++]).lower() == "true");
	bool isGrouped = ((seqPlugins[i++]).lower() == "true");
        QString category = seqPlugins[i++];
        unsigned int portCount = seqPlugins[i++].toInt();

//	std::cerr << "PLUGIN: " << identifier << " / CATEGORY: \"" << (category ? category : "(null)") << "\"" << std::endl;

        AudioPlugin *aP = aPM->addPlugin(identifier,
                                         name,
                                         uniqueId,
                                         label,
                                         author,
                                         copyright,
					 isSynth,
					 isGrouped,
					 category);

        for (unsigned int j = 0; j < portCount; j++)
        {
            int number = seqPlugins[i++].toInt();
            name = seqPlugins[i++];
            PluginPort::PortType type =
                PluginPort::PortType(seqPlugins[i++].toInt());
            PluginPort::PortDisplayHint hint =
                PluginPort::PortDisplayHint(seqPlugins[i++].toInt());
            PortData lowerBound = seqPlugins[i++].toFloat();
            PortData upperBound = seqPlugins[i++].toFloat();
	    PortData defaultValue = seqPlugins[i++].toFloat();

            aP->addPort(number,
                        name,
                        type,
                        hint,
                        lowerBound,
                        upperBound,
			defaultValue);

        }
    }
}


// Send the MIDI recording device to the sequencer
//
void
SequenceManager::sendMIDIRecordingDevice(const QString recordDeviceStr)
{

    if (recordDeviceStr)
    {
        int recordDevice = recordDeviceStr.toInt();

        if (recordDevice >= 0)
        {
            MappedEvent mE(MidiInstrumentBase, // InstrumentId
                                       MappedEvent::SystemRecordDevice,
                                       MidiByte(recordDevice),
                                       MidiByte(true));

            StudioControl::sendMappedEvent(mE);
            SEQMAN_DEBUG << "set MIDI record device to "
                         << recordDevice << endl;
        }
    }
}

void 
SequenceManager::restoreRecordSubscriptions()
{
    KConfig* config = kapp->config();
    config->setGroup(SequencerOptionsConfigGroup);
    //QString recordDeviceStr = config->readEntry("midirecorddevice");
    QStringList devList = config->readListEntry("midirecorddevice");
    
    for( QStringList::ConstIterator it = devList.begin(); 
         it != devList.end(); ++it) {
    	sendMIDIRecordingDevice(*it);
    }
	
}

// Clear down all temporary (non read-only) objects and then
// add the basic audio faders only (one per instrument).
//
void
SequenceManager::reinitialiseSequencerStudio()
{
    KConfig* config = kapp->config();
    config->setGroup(SequencerOptionsConfigGroup);
    //QString recordDeviceStr = config->readEntry("midirecorddevice");
    
    //sendMIDIRecordingDevice(recordDeviceStr);
    restoreRecordSubscriptions();
	
    // Toggle JACK audio ports appropriately
    //
    bool submasterOuts = config->readBoolEntry("audiosubmasterouts", false);
    bool faderOuts = config->readBoolEntry("audiofaderouts", false);

    MidiByte ports = 0;
    if (faderOuts) {
	ports |= MappedEvent::FaderOuts;
    }
    if (submasterOuts) {
	ports |= MappedEvent::SubmasterOuts;
    }
    MappedEvent mEports
	(MidiInstrumentBase,
	 MappedEvent::SystemAudioPorts,
	 ports);

    StudioControl::sendMappedEvent(mEports);


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

    stopping();
    
    MappedEvent mE(MidiInstrumentBase, MappedEvent::Panic, 0, 0);
    emit setProgress(40);
    StudioControl::sendMappedEvent(mE);
    emit setProgress(100);
    
//    Studio &studio = m_doc->getStudio();
//
//    InstrumentList list = studio.getPresentationInstruments();
//    InstrumentList::iterator it;
//
//    int maxDevices = 0, device = 0;
//    for (it = list.begin(); it != list.end(); it++)
//        if ((*it)->getType() == Instrument::Midi)
//            maxDevices++;
//
//    emit setProgress(40);
//
//    for (it = list.begin(); it != list.end(); it++)
//    {
//        if ((*it)->getType() == Instrument::Midi)
//        {
//            for (unsigned int i = 0; i < 128; i++)
//            {
//                MappedEvent 
//                    mE((*it)->getId(),
//                                MappedEvent::MidiNote,
//                                i,
//                                0);
//
//                StudioControl::sendMappedEvent(mE);
//            }
//
//            device++;
//        }
//        
//        emit setProgress(int(90.0 * (double(device) / double(maxDevices))));
//    }
//
//    resetControllers();
}

// In this case we only route MIDI events to the transport ticker
//
void
SequenceManager::showVisuals(const MappedComposition &mC)
{
    MappedComposition::const_iterator it = mC.begin();
    if (it != mC.end()) m_transport->setMidiOutLabel(*it);
}


// Filter a MappedComposition by Type.
//
MappedComposition
SequenceManager::applyFiltering(const MappedComposition &mC,
                                MappedEvent::MappedEventType filter)
{
    MappedComposition retMc;
    MappedComposition::const_iterator it = mC.begin();

    for (; it != mC.end(); it++)
    {
        if (!((*it)->getType() & filter))
            retMc.insert(new MappedEvent(*it));
    }

    return retMc;
}

void SequenceManager::resetCompositionMmapper()
{
    SEQMAN_DEBUG << "SequenceManager::resetCompositionMmapper()\n";
    delete m_compositionMmapper;
    m_compositionMmapper = new CompositionMmapper(m_doc);

    resetMetronomeMmapper();
    resetTempoSegmentMmapper();
    resetTimeSigSegmentMmapper();
    resetControlBlockMmapper();
}

void SequenceManager::resetMetronomeMmapper()
{
    SEQMAN_DEBUG << "SequenceManager::resetMetronomeMmapper()\n";

    delete m_metronomeMmapper;
    m_metronomeMmapper = SegmentMmapperFactory::makeMetronome(m_doc);
}

void SequenceManager::resetTempoSegmentMmapper()
{
    SEQMAN_DEBUG << "SequenceManager::resetTempoSegmentMmapper()\n";

    delete m_tempoSegmentMmapper;
    m_tempoSegmentMmapper = SegmentMmapperFactory::makeTempo(m_doc);
}

void SequenceManager::resetTimeSigSegmentMmapper()
{
    SEQMAN_DEBUG << "SequenceManager::resetTimeSigSegmentMmapper()\n";

    delete m_timeSigSegmentMmapper;
    m_timeSigSegmentMmapper = SegmentMmapperFactory::makeTimeSig(m_doc);
}

void SequenceManager::resetControlBlockMmapper()
{
    SEQMAN_DEBUG << "SequenceManager::resetControlBlockMmapper()\n";

    m_controlBlockMmapper->setDocument(m_doc);
}


bool SequenceManager::event(QEvent *e)
{
    if (e->type() == QEvent::User) {
	SEQMAN_DEBUG << "SequenceManager::event() with user event\n";
	if (m_updateRequested) {
	    SEQMAN_DEBUG << "SequenceManager::event(): update requested\n";
	    checkRefreshStatus();
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

    // Look at trigger segments first: if one of those has changed, we'll
    // need to be aware of it when scanning segments subsequently

    TriggerSegmentRec::SegmentRuntimeIdSet ridset;
    Composition &comp = m_doc->getComposition();
    SegmentRefreshMap newTriggerMap;

    for (Composition::triggersegmentcontaineriterator i =
	     comp.getTriggerSegments().begin();
	 i != comp.getTriggerSegments().end(); ++i) {

	Segment *s = (*i)->getSegment();

	if (m_triggerSegments.find(s) == m_triggerSegments.end()) {
	    newTriggerMap[s] = s->getNewRefreshStatusId();
	} else {
	    newTriggerMap[s] = m_triggerSegments[s];
	}

	if (s->getRefreshStatus(newTriggerMap[s]).needsRefresh()) {
	    TriggerSegmentRec::SegmentRuntimeIdSet &thisSet = (*i)->getReferences();
	    ridset.insert(thisSet.begin(), thisSet.end());
	    s->getRefreshStatus(newTriggerMap[s]).setNeedsRefresh(false);
	}
    }

    m_triggerSegments = newTriggerMap;

    SEQMAN_DEBUG << "SequenceManager::checkRefreshStatus: segments modified by changes to trigger segments are:" << endl;
    int x = 0;
    for (TriggerSegmentRec::SegmentRuntimeIdSet::iterator i = ridset.begin();
	 i != ridset.end(); ++i) {
	SEQMAN_DEBUG << x << ": " << *i << endl;
	++x;
    }

    std::vector<Segment*>::iterator i;

    // Check removed segments first
    for (i = m_removedSegments.begin(); i != m_removedSegments.end(); ++i) {
        processRemovedSegment(*i);
    }
    m_removedSegments.clear();

    SEQMAN_DEBUG << "SequenceManager::checkRefreshStatus: we have "
		 << m_segments.size() << " segments" << endl;
    
    // then the ones which are still there
    for (SegmentRefreshMap::iterator i = m_segments.begin();
	 i != m_segments.end(); ++i) {
	if (i->first->getRefreshStatus(i->second).needsRefresh() ||
	    ridset.find(i->first->getRuntimeId()) != ridset.end()) {
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

    SEQMAN_DEBUG << "SequenceManager::segmentModified() : size changed = "
                 << sizeChanged << endl;

    if ((m_transportStatus == PLAYING) && sizeChanged) {
        QByteArray data;
        QDataStream streamOut(data, IO_WriteOnly);

        streamOut << (QString)m_compositionMmapper->getSegmentFileName(s);
	streamOut << (size_t)m_compositionMmapper->getSegmentFileSize(s);
        
        SEQMAN_DEBUG << "SequenceManager::segmentModified() : DCOP-call sequencer remapSegment"
                     << m_compositionMmapper->getSegmentFileName(s) << endl;

        rgapp->sequencerSend("remapSegment(QString, size_t)", data);
    }
}

void SequenceManager::segmentAdded(const Composition*, Segment* s)
{
    SEQMAN_DEBUG << "SequenceManager::segmentAdded(" << s << ")\n";
    m_addedSegments.push_back(s);
}

void SequenceManager::segmentRemoved(const Composition*, Segment* s)
{
    SEQMAN_DEBUG << "SequenceManager::segmentRemoved(" << s << ")\n";
    m_removedSegments.push_back(s);
}

void SequenceManager::segmentRepeatChanged(const Composition*, Segment* s, bool repeat)
{
    SEQMAN_DEBUG << "SequenceManager::segmentRepeatChanged(" << s << ", " << repeat << ")\n";
    segmentModified(s);
}

void SequenceManager::segmentEventsTimingChanged(const Composition*, Segment * s, timeT t, RealTime)
{
    SEQMAN_DEBUG << "SequenceManager::segmentEventsTimingChanged(" << s << ", " << t << ")\n";
    segmentModified(s);
    if (s && s->getType() == Segment::Audio && m_transportStatus == PLAYING) {
	QByteArray data;
        rgapp->sequencerSend("remapTracks()", data);
    }
}

void SequenceManager::segmentTransposeChanged(const Composition*, Segment *s, int transpose)
{
    SEQMAN_DEBUG << "SequenceManager::segmentTransposeChanged(" << s << ", " << transpose << ")\n";
    segmentModified(s);
}

void SequenceManager::segmentTrackChanged(const Composition*, Segment *s, TrackId id)
{
    SEQMAN_DEBUG << "SequenceManager::segmentTrackChanged(" << s << ", " << id << ")\n";
    segmentModified(s);
    if (s && s->getType() == Segment::Audio && m_transportStatus == PLAYING) {
	QByteArray data;
        rgapp->sequencerSend("remapTracks()", data);
    }
}

void SequenceManager::processAddedSegment(Segment* s)
{
    SEQMAN_DEBUG << "SequenceManager::processAddedSegment(" << s << ")\n";

    m_compositionMmapper->segmentAdded(s);

    if (m_transportStatus == PLAYING) {

        QByteArray data;
        QDataStream streamOut(data, IO_WriteOnly);

        streamOut << m_compositionMmapper->getSegmentFileName(s);
        
        if (!rgapp->sequencerSend("addSegment(QString)", data)) {
            m_transportStatus = STOPPED;
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

        if (!rgapp->sequencerSend("deleteSegment(QString)", data)) {
            // failed
            m_transportStatus = STOPPED;
        }
    }

    // Remove from segments map
    m_segments.erase(s);
}

void SequenceManager::endMarkerTimeChanged(const Composition *, bool /*shorten*/)
{
    resetMetronomeMmapper();
}

void SequenceManager::timeSignatureChanged(const Composition *)
{
    resetMetronomeMmapper();
}

void SequenceManager::compositionDeleted(const Composition *)
{
    // do nothing
}

void SequenceManager::trackChanged(const Composition *, Track* t)
{
    m_controlBlockMmapper->updateTrackData(t);

    if (m_transportStatus == PLAYING) {
	QByteArray data;
        rgapp->sequencerSend("remapTracks()", data);
    }
}

void SequenceManager::trackDeleted(const Composition *, TrackId t)
{
    m_controlBlockMmapper->setTrackDeleted(t);
}

void SequenceManager::metronomeChanged(InstrumentId id,
				       bool regenerateTicks)
{
    // This method is called when the user has changed the
    // metronome instrument, pitch etc

    SEQMAN_DEBUG << "SequenceManager::metronomeChanged (simple)"
                 << ", instrument = "
                 << id
                 << endl;

    if (regenerateTicks) resetMetronomeMmapper();

    m_controlBlockMmapper->updateMetronomeData(id);
    if (m_transportStatus == PLAYING) {
	m_controlBlockMmapper->updateMetronomeForPlayback();
    } else {
	m_controlBlockMmapper->updateMetronomeForRecord();
    }

    m_metronomeMmapper->refresh();
    m_timeSigSegmentMmapper->refresh();
    m_tempoSegmentMmapper->refresh();
}

void SequenceManager::metronomeChanged(const Composition *)
{
    // This method is called when the muting status in the composition
    // has changed -- the metronome itself has not actually changed

    SEQMAN_DEBUG << "SequenceManager::metronomeChanged " 
                 << ", instrument = "
                 << m_metronomeMmapper->getMetronomeInstrument()
                 << endl;

    m_controlBlockMmapper->updateMetronomeData
	(m_metronomeMmapper->getMetronomeInstrument());

    if (m_transportStatus == PLAYING) {
	m_controlBlockMmapper->updateMetronomeForPlayback();
    } else {
	m_controlBlockMmapper->updateMetronomeForRecord();
    }
}

void SequenceManager::filtersChanged(MidiFilter thruFilter,
				     MidiFilter recordFilter)
{
    m_controlBlockMmapper->updateMidiFilters(thruFilter, recordFilter);
}

void SequenceManager::soloChanged(const Composition *, bool solo, TrackId selectedTrack)
{
    m_controlBlockMmapper->updateSoloData(solo, selectedTrack);
}

void SequenceManager::tempoChanged(const Composition *c)
{
    SEQMAN_DEBUG << "SequenceManager::tempoChanged()\n";

    // Refresh all segments
    //
    for (SegmentRefreshMap::iterator i = m_segments.begin();
	 i != m_segments.end(); ++i) {
        segmentModified(i->first);
    }

    // and metronome, time sig and tempo
    //
    m_metronomeMmapper->refresh();
    m_timeSigSegmentMmapper->refresh();
    m_tempoSegmentMmapper->refresh();

    if (c->isLooping()) setLoop(c->getLoopStart(), c->getLoopEnd());
}

void
SequenceManager::sendTransportControlStatuses()
{
    KConfig* config = kapp->config();
    config->setGroup(SequencerOptionsConfigGroup);

    // Get the config values
    //
    bool jackTransport = config->readBoolEntry("jacktransport", false);
    bool jackMaster = config->readBoolEntry("jackmaster", false);

    int mmcMode = config->readNumEntry("mmcmode", 0);
    int mtcMode = config->readNumEntry("mtcmode", 0);

    bool midiClock = config->readBoolEntry("midiclock", false);
    bool midiSyncAuto = config->readBoolEntry("midisyncautoconnect", false);

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

    MappedEvent mEjackValue(MidiInstrumentBase, // InstrumentId
                                        MappedEvent::SystemJackTransport,
                                        MidiByte(jackValue));
    StudioControl::sendMappedEvent(mEjackValue);


    // Send MMC transport
    //
    MappedEvent mEmmcValue(MidiInstrumentBase, // InstrumentId
                                       MappedEvent::SystemMMCTransport,
                                       MidiByte(mmcMode));

    StudioControl::sendMappedEvent(mEmmcValue);


    // Send MTC transport
    //
    MappedEvent mEmtcValue(MidiInstrumentBase, // InstrumentId
                                       MappedEvent::SystemMTCTransport,
                                       MidiByte(mtcMode));

    StudioControl::sendMappedEvent(mEmtcValue);


    // Send MIDI Clock
    //
    MappedEvent mEmidiClock(MidiInstrumentBase, // InstrumentId
                                        MappedEvent::SystemMIDIClock,
                                        MidiByte(midiClock));

    StudioControl::sendMappedEvent(mEmidiClock);


    // Send MIDI Sync Auto-Connect
    //
    MappedEvent mEmidiSyncAuto(MidiInstrumentBase, // InstrumentId
			       MappedEvent::SystemMIDISyncAuto,
			       MidiByte(midiSyncAuto ? 1 : 0));

    StudioControl::sendMappedEvent(mEmidiSyncAuto);

}

void
SequenceManager::slotCountdownTimerTimeout()
{
    // Set the elapsed time in seconds
    //
    m_countdownDialog->setElapsedTime(m_recordTime->elapsed() / 1000);
}

void
SequenceManager::slotFoundMountPoint(const QString&,
                                     unsigned long kBSize,
                                     unsigned long /*kBUsed*/,
                                     unsigned long kBAvail)
{
    m_gotDiskSpaceResult = true;
    m_diskSpaceKBAvail = kBAvail;
}




}
#include "sequencemanager.moc"
