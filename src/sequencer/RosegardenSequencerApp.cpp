/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2007
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>

    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "RosegardenSequencerApp.h"
#include <kapplication.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>

#include <iostream>

#include <klocale.h>
#include <kstandarddirs.h>

#include <dcopclient.h>
#include <qdatetime.h>
#include <qstring.h>
#include <qdir.h>
#include <qbuffer.h>
#include <qvaluevector.h>

#include "misc/Debug.h"
#include "ControlBlockMmapper.h"
#include "MmappedSegment.h"
#include "gui/application/RosegardenDCOP.h"
#include "sound/ControlBlock.h"
#include "sound/SoundDriver.h"
#include "sound/SoundDriverFactory.h"
#include "sound/MappedInstrument.h"
#include "base/Profiler.h"
#include "sound/PluginFactory.h"

namespace Rosegarden
{

// The default latency and read-ahead values are actually sent
// down from the GUI every time playback or recording starts
// so the local values are kind of meaningless.
//
//
RosegardenSequencerApp::RosegardenSequencerApp() :
        DCOPObject("RosegardenSequencerIface"),
        m_driver(0),
        m_transportStatus(STOPPED),
        m_songPosition(0, 0),
        m_lastFetchSongPosition(0, 0),
        m_readAhead(0, 80000000),          // default value
        m_audioMix(0, 60000000),          // default value
        m_audioRead(0, 100000000),          // default value
        m_audioWrite(0, 200000000),          // default value
        m_smallFileSize(128),
        m_loopStart(0, 0),
        m_loopEnd(0, 0),
        m_studio(new MappedStudio()),
        m_segmentFilesPath(KGlobal::dirs()->resourceDirs("tmp").last()),
        m_metaIterator(0),
        m_controlBlockMmapper(0),
        m_transportToken(1),
        m_isEndOfCompReached(false)
{
    SEQUENCER_DEBUG << "Registering with DCOP server" << endl;

    // Without DCOP we are nothing
    QCString realAppId = kapp->dcopClient()->registerAs(kapp->name(), false);

    if (realAppId.isNull()) {
        SEQUENCER_DEBUG << "RosegardenSequencer cannot register "
        << "with DCOP server" << endl;
        close();
    }

    // Initialise the MappedStudio
    //
    initialiseStudio();

    // Creating this object also initialises the Rosegarden ALSA/JACK
    // interface for both playback and recording. MappedStudio
    // aduio faders are also created.
    //
    m_driver = SoundDriverFactory::createDriver(m_studio);
    m_studio->setSoundDriver(m_driver);

    if (!m_driver) {
        SEQUENCER_DEBUG << "RosegardenSequencer object could not be allocated"
        << endl;
        close();
    }

    m_driver->setAudioBufferSizes(m_audioMix, m_audioRead, m_audioWrite,
                                  m_smallFileSize);

    m_driver->setSequencerDataBlock(m_sequencerMapper.getSequencerDataBlock());
    m_driver->setExternalTransportControl(this);

    // Check for new clients every so often
    //
    m_newClientTimer = new QTimer(this);
    connect(m_newClientTimer, SIGNAL(timeout()),
            this, SLOT(slotCheckForNewClients()));

    m_newClientTimer->start(3000); // every 3 seconds
}

RosegardenSequencerApp::~RosegardenSequencerApp()
{
    SEQUENCER_DEBUG << "RosegardenSequencer - shutting down" << endl;
    m_driver->shutdown();
    delete m_studio;
    delete m_driver;
    delete m_controlBlockMmapper;
}

void
RosegardenSequencerApp::quit()
{
    std::cerr << "RosegardenSequencerApp::quit()" << std::endl;

    close();

    // and break out of the loop next time around
    m_transportStatus = QUIT;
}


void
RosegardenSequencerApp::stop()
{
    // set our state at this level to STOPPING (pending any
    // unfinished NOTES)
    m_transportStatus = STOPPING;

    // report
    //
    SEQUENCER_DEBUG << "RosegardenSequencerApp::stop() - stopping" << endl;

    // process pending NOTE OFFs and stop the Sequencer
    m_driver->stopPlayback();

    // the Sequencer doesn't need to know these once
    // we've stopped.
    //
    m_songPosition.sec = 0;
    m_songPosition.nsec = 0;
    m_lastFetchSongPosition.sec = 0;
    m_lastFetchSongPosition.nsec = 0;

    cleanupMmapData();

    Profiles::getInstance()->dump();

    incrementTransportToken();
}

// Get a slice of events from the GUI
//
void
RosegardenSequencerApp::fetchEvents(MappedComposition &composition,
                                    const RealTime &start,
                                    const RealTime &end,
                                    bool firstFetch)
{
    // Always return nothing if we're stopped
    //
    if ( m_transportStatus == STOPPED || m_transportStatus == STOPPING )
        return ;

    // If we're looping then we should get as much of the rest of
    // the right hand of the loop as possible and also events from
    // the beginning of the loop.  We can do this in two fetches.
    // Make sure that we delete all returned pointers when we've
    // finished with them.
    //
    //
    /*
        if (isLooping() == true && end >= m_loopEnd)
        {
            RealTime loopOverlap = end - m_loopEnd;
     
            MappedComposition *endLoop = 0;
     
    	if (m_loopEnd > start) {
    	    endLoop = getSlice(start, m_loopEnd, firstFetch);
    	}
     
    	if (loopOverlap > RealTime::zeroTime) { 
     
    	    MappedComposition *beginLoop =
    		getSlice(m_loopStart, m_loopStart + loopOverlap, true);
     
    	    // move the start time of the begin section one loop width
    	    // into the future and ensure that we keep the clocks level
    	    // until this time has passed
    	    //
    	    beginLoop->moveStartTime(m_loopEnd - m_loopStart);
     
    	    if (endLoop) {
    		(*endLoop) = (*endLoop) + (*beginLoop);
    		delete beginLoop;
    	    } else {
    		endLoop = beginLoop;
    	    }
    	}
     
    	if (endLoop) return endLoop;
    	else return new MappedComposition();
        }
        else
    */
    getSlice(composition, start, end, firstFetch);
    applyLatencyCompensation(composition);
}


void
RosegardenSequencerApp::getSlice(MappedComposition &composition,
                                 const RealTime &start,
                                 const RealTime &end,
                                 bool firstFetch)
{
    //    SEQUENCER_DEBUG << "RosegardenSequencerApp::getSlice (" << start << " -> " << end << ", " << firstFetch << ")" << endl;

    if (firstFetch || (start < m_lastStartTime)) {
        SEQUENCER_DEBUG << "[calling jumpToTime on start]" << endl;
        m_metaIterator->jumpToTime(start);
    }

    (void)m_metaIterator->fillCompositionWithEventsUntil
    (firstFetch, &composition, start, end);

    //     setEndOfCompReached(eventsRemaining); // don't do that, it breaks recording because
    // playing stops right after it starts.

    m_lastStartTime = start;
}


void
RosegardenSequencerApp::applyLatencyCompensation(MappedComposition &composition)
{
    RealTime maxLatency = m_driver->getMaximumPlayLatency();
    if (maxLatency == RealTime::zeroTime)
        return ;

    for (MappedComposition::iterator i = composition.begin();
            i != composition.end(); ++i) {

        RealTime instrumentLatency =
            m_driver->getInstrumentPlayLatency((*i)->getInstrument());

        //	std::cerr << "RosegardenSequencerApp::applyLatencyCompensation: maxLatency " << maxLatency << ", instrumentLatency " << instrumentLatency << ", moving " << (*i)->getEventTime() << " to " << (*i)->getEventTime() + maxLatency - instrumentLatency << std::endl;

        (*i)->setEventTime((*i)->getEventTime() +
                           maxLatency - instrumentLatency);
    }
}


// The first fetch of events from the core/ and initialisation for
// this session of playback.  We fetch up to m_readAhead ahead at
// first at then top up at each slice.
//
bool
RosegardenSequencerApp::startPlaying()
{
    // Fetch up to m_readHead microseconds worth of events
    //
    m_lastFetchSongPosition = m_songPosition + m_readAhead;

    // This will reset the Sequencer's internal clock
    // ready for new playback
    m_driver->initialisePlayback(m_songPosition);

    m_mC.clear();
    fetchEvents(m_mC, m_songPosition, m_songPosition + m_readAhead, true);

    // process whether we need to or not as this also processes
    // the audio queue for us
    //
    m_driver->processEventsOut(m_mC, m_songPosition, m_songPosition + m_readAhead);

    std::vector<MappedEvent> audioEvents;
    m_metaIterator->getAudioEvents(audioEvents);
    m_driver->initialiseAudioQueue(audioEvents);

    //    SEQUENCER_DEBUG << "RosegardenSequencerApp::startPlaying: pausing to simulate high-load environment" << endl;
    //    ::sleep(2);

    // and only now do we signal to start the clock
    //
    m_driver->startClocks();

    incrementTransportToken();

    return true; // !isEndOfCompReached();
}

bool
RosegardenSequencerApp::keepPlaying()
{
    Profiler profiler("RosegardenSequencerApp::keepPlaying");

    m_mC.clear();

    RealTime fetchEnd = m_songPosition + m_readAhead;
    if (isLooping() && fetchEnd >= m_loopEnd) {
        fetchEnd = m_loopEnd - RealTime(0, 1);
    }
    if (fetchEnd > m_lastFetchSongPosition) {
        fetchEvents(m_mC, m_lastFetchSongPosition, fetchEnd, false);
    }

    // Again, process whether we need to or not to keep
    // the Sequencer up-to-date with audio events
    //
    m_driver->processEventsOut(m_mC, m_lastFetchSongPosition, fetchEnd);

    if (fetchEnd > m_lastFetchSongPosition) {
        m_lastFetchSongPosition = fetchEnd;
    }

    return true; // !isEndOfCompReached(); - until we sort this out, we don't stop at end of comp.
}

// Return current Sequencer time in GUI compatible terms
//
void
RosegardenSequencerApp::updateClocks()
{
    Profiler profiler("RosegardenSequencerApp::updateClocks");

    m_driver->runTasks();

    checkExternalTransport();

    //SEQUENCER_DEBUG << "RosegardenSequencerApp::updateClocks" << endl;

    // If we're not playing etc. then that's all we need to do
    //
    if (m_transportStatus != PLAYING &&
            m_transportStatus != RECORDING)
        return ;

    RealTime newPosition = m_driver->getSequencerTime();

    // Go around the loop if we've reached the end
    //
    if (isLooping() && newPosition >= m_loopEnd) {

        RealTime oldPosition = m_songPosition;

        // Remove the loop width from the song position and send
        // this position to the GUI
        //
        newPosition = m_songPosition = m_lastFetchSongPosition = m_loopStart;

        m_driver->stopClocks();

        // Reset playback using this jump
        //
        m_driver->resetPlayback(oldPosition, m_songPosition);

        m_mC.clear();
        fetchEvents(m_mC, m_songPosition, m_songPosition + m_readAhead, true);

        m_driver->processEventsOut(m_mC, m_songPosition, m_songPosition + m_readAhead);

        m_driver->startClocks();
    } else {
        m_songPosition = newPosition;

        if (m_songPosition <= m_driver->getStartPosition())
            newPosition = m_driver->getStartPosition();
    }

    RealTime maxLatency = m_driver->getMaximumPlayLatency();
    if (maxLatency != RealTime::zeroTime) {
        //	std::cerr << "RosegardenSequencerApp::updateClocks: latency compensation moving " << newPosition << " to " << newPosition - maxLatency << std::endl;
        newPosition = newPosition - maxLatency;
    }

    // Remap the position pointer
    //
    m_sequencerMapper.updatePositionPointer(newPosition);
}

void
RosegardenSequencerApp::notifySequencerStatus()
{
    QByteArray data, replyData;
    QCString replyType;
    QDataStream arg(data, IO_WriteOnly);

    arg << (int)m_transportStatus;

    if (!kapp->dcopClient()->send(ROSEGARDEN_GUI_APP_NAME,
                                  ROSEGARDEN_GUI_IFACE_NAME,
                                  "notifySequencerStatus(int)",
                                  data)) {
        SEQUENCER_DEBUG << "RosegardenSequencer::notifySequencerStatus()"
        << " - can't send to RosegardenGUI client"
        << endl;

        // Stop the sequencer
        //
        stop();
    }
}

void
RosegardenSequencerApp::sleep(const RealTime &rt)
{
    m_driver->sleep(rt);
}


// Sets the Sequencer object and this object to the new time
// from where playback can continue.
//
void
RosegardenSequencerApp::jumpTo(long posSec, long posNsec)
{
    SEQUENCER_DEBUG << "RosegardenSequencerApp::jumpTo(" << posSec << ", " << posNsec << ")\n";

    if (posSec < 0 && posNsec < 0)
        return ;

    m_driver->stopClocks();

    RealTime oldPosition = m_songPosition;

    m_songPosition = m_lastFetchSongPosition = RealTime(posSec, posNsec);

    if (m_sequencerMapper.getSequencerDataBlock()) {
        m_sequencerMapper.getSequencerDataBlock()->setPositionPointer
        (m_songPosition);
    }

    m_driver->resetPlayback(oldPosition, m_songPosition);

    if (m_driver->isPlaying()) {

        // Now prebuffer as in startPlaying:

        m_mC.clear();
        fetchEvents(m_mC, m_songPosition, m_songPosition + m_readAhead, true);

        // process whether we need to or not as this also processes
        // the audio queue for us
        //
        m_driver->processEventsOut(m_mC, m_songPosition, m_songPosition + m_readAhead);
    }

    incrementTransportToken();

    //    SEQUENCER_DEBUG << "RosegardenSequencerApp::jumpTo: pausing to simulate high-load environment" << endl;
    //    ::sleep(1);

    m_driver->startClocks();

    return ;
}

// Send the last recorded MIDI block
//
void
RosegardenSequencerApp::processRecordedMidi()
{
    MappedComposition *mC = m_driver->getMappedComposition();

    if (mC->empty() || !m_controlBlockMmapper)
        return ;

    applyFiltering(mC, m_controlBlockMmapper->getRecordFilter(), false);
    m_sequencerMapper.updateRecordingBuffer(mC);

    if (m_controlBlockMmapper->isMidiRoutingEnabled()) {
        applyFiltering(mC, m_controlBlockMmapper->getThruFilter(), true);
        routeEvents(mC, false);
    }
}

void
RosegardenSequencerApp::routeEvents(MappedComposition *mC, bool useSelectedTrack)
{
    InstrumentId instrumentId;

    if (useSelectedTrack) {
        instrumentId = m_controlBlockMmapper->getInstrumentForTrack
                       (m_controlBlockMmapper->getSelectedTrack());
        for (MappedComposition::iterator i = mC->begin();
                i != mC->end(); ++i) {
            (*i)->setInstrument(instrumentId);
        }
    } else {
        for (MappedComposition::iterator i = mC->begin();
                i != mC->end(); ++i) {
            instrumentId = m_controlBlockMmapper->getInstrumentForEvent
                           ((*i)->getRecordedDevice(), (*i)->getRecordedChannel());
            (*i)->setInstrument(instrumentId);
        }
    }
    m_driver->processEventsOut(*mC);
}

// Send an update
//
void
RosegardenSequencerApp::processRecordedAudio()
{
    // Nothing to do here: the recording time is sent back to the GUI
    // in the sequencer mapper as a normal case.
}


// This method is called during STOPPED or PLAYING operations
// to mop up any async (unexpected) incoming MIDI or Audio events
// and forward them to the GUI for display
//
void
RosegardenSequencerApp::processAsynchronousEvents()
{
    if (!m_controlBlockMmapper) {

        // If the control block mmapper doesn't exist, we'll just
        // return here.  But we want to ensure we don't check again
        // immediately, because we're probably waiting for the GUI to
        // start up.

        static bool lastChecked = false;
        static struct timeval lastCheckedAt;

        struct timeval tv;
        (void)gettimeofday(&tv, 0);

        if (lastChecked &&
                tv.tv_sec == lastCheckedAt.tv_sec) {
            lastCheckedAt = tv;
            return ;
        }

        lastChecked = true;
        lastCheckedAt = tv;

        try {
            m_controlBlockMmapper = new ControlBlockMmapper(KGlobal::dirs()->resourceDirs("tmp").last()
                                    + "/rosegarden_control_block");
        } catch (Exception e) {
            // Assume that the control block simply hasn't been
            // created yet because the GUI's still starting up.
            // If there's a real problem with the mmapper, it
            // will show up in play() instead.
            return ;
        }
        m_sequencerMapper.setControlBlock(m_controlBlockMmapper->getControlBlock());
    }

    MappedComposition *mC = m_driver->getMappedComposition();

    if (mC->empty()) {
        m_driver->processPending();
        return ;
    }

    //    std::cerr << "processAsynchronousEvents: have " << mC->size() << " events" << std::endl;

    QByteArray data;
    QDataStream arg(data, IO_WriteOnly);
    arg << mC;

    if (m_controlBlockMmapper->isMidiRoutingEnabled()) {
        applyFiltering(mC, m_controlBlockMmapper->getThruFilter(), true);
        routeEvents(mC, true);
    }

    //    std::cerr << "processAsynchronousEvents: sent " << mC->size() << " events" << std::endl;

    if (!kapp->dcopClient()->send(ROSEGARDEN_GUI_APP_NAME,
                                  ROSEGARDEN_GUI_IFACE_NAME,
                                  "processAsynchronousMidi(MappedComposition)", data)) {
        SEQUENCER_DEBUG << "RosegardenSequencer::processAsynchronousEvents() - "
        << "can't call RosegardenGUI client" << endl;

        // Stop the sequencer so we can see if we can try again later
        //
        stop();
    }

    // Process any pending events (Note Offs or Audio) as part of
    // same procedure.
    //
    m_driver->processPending();
}


void
RosegardenSequencerApp::applyFiltering(MappedComposition *mC,
                                       MidiFilter filter,
                                       bool filterControlDevice)
{
    for (MappedComposition::iterator i = mC->begin();
            i != mC->end(); ) { // increment in loop
        MappedComposition::iterator j = i;
        ++j;
        if (((*i)->getType() & filter) ||
                (filterControlDevice && ((*i)->getRecordedDevice() ==
                                         Device::CONTROL_DEVICE))) {
            mC->erase(i);
        }
        i = j;
    }
}


int
RosegardenSequencerApp::record(const RealTime &time,
                               const RealTime &readAhead,
                               const RealTime &audioMix,
                               const RealTime &audioRead,
                               const RealTime &audioWrite,
                               long smallFileSize,
                               long recordMode)
{
    TransportStatus localRecordMode = (TransportStatus) recordMode;

    SEQUENCER_DEBUG << "RosegardenSequencerApp::record - recordMode is " << recordMode << ", transport status is " << m_transportStatus << endl;

    // punch in recording
    if (m_transportStatus == PLAYING) {
        if (localRecordMode == STARTING_TO_RECORD) {
            SEQUENCER_DEBUG << "RosegardenSequencerApp::record: punching in" << endl;
            localRecordMode = RECORDING; // no need to start playback
        }
    }

    // For audio recording we need to retrieve audio
    // file names from the GUI
    //
    if (localRecordMode == STARTING_TO_RECORD ||
            localRecordMode == RECORDING) {
        SEQUENCER_DEBUG << "RosegardenSequencerApp::record()"
        << " - starting to record" << endl;

        QValueVector<InstrumentId> armedInstruments;
        QValueVector<QString> audioFileNames;

        {
            QByteArray data, replyData;
            QCString replyType;
            QDataStream arg(data, IO_WriteOnly);

            if (!kapp->dcopClient()->call(ROSEGARDEN_GUI_APP_NAME,
                                          ROSEGARDEN_GUI_IFACE_NAME,
                                          "getArmedInstruments()",
                                          data, replyType, replyData, true)) {
                SEQUENCER_DEBUG << "RosegardenSequencer::record()"
                << " - can't call RosegardenGUI client for getArmedInstruments"
                << endl;
            }

            QDataStream reply(replyData, IO_ReadOnly);
            if (replyType == "QValueVector<InstrumentId>") {
                reply >> armedInstruments;
            } else {
                SEQUENCER_DEBUG << "RosegardenSequencer::record() - "
                << "unrecognised type returned for getArmedInstruments" << endl;
            }
        }

        QValueVector<InstrumentId> audioInstruments;

        for (unsigned int i = 0; i < armedInstruments.size(); ++i) {
            if (armedInstruments[i] >= AudioInstrumentBase &&
                    armedInstruments[i] < MidiInstrumentBase) {
                audioInstruments.push_back(armedInstruments[i]);
            }
        }

        if (audioInstruments.size() > 0) {

            QByteArray data, replyData;
            QCString replyType;
            QDataStream arg(data, IO_WriteOnly);

            arg << audioInstruments;

            if (!kapp->dcopClient()->call(ROSEGARDEN_GUI_APP_NAME,
                                          ROSEGARDEN_GUI_IFACE_NAME,
                                          "createRecordAudioFiles(QValueVector<InstrumentId>)",
                                          data, replyType, replyData, true)) {
                SEQUENCER_DEBUG << "RosegardenSequencer::record()"
                << " - can't call RosegardenGUI client for createNewAudioFiles"
                << endl;
            }

            QDataStream reply(replyData, IO_ReadOnly);
            if (replyType == "QValueVector<QString>") {
                reply >> audioFileNames;
            } else {
                SEQUENCER_DEBUG << "RosegardenSequencer::record() - "
                << "unrecognised type returned for createNewAudioFiles" << endl;
            }

            if (audioFileNames.size() != audioInstruments.size()) {
                std::cerr << "ERROR: RosegardenSequencer::record(): Failed to create correct number of audio files (wanted " << audioInstruments.size() << ", got " << audioFileNames.size() << ")" << std::endl;
                stop();
                return 0;
            }
        }

        std::vector<InstrumentId> armedInstrumentsVec;
        std::vector<QString> audioFileNamesVec;
        for (int i = 0; i < armedInstruments.size(); ++i) {
            armedInstrumentsVec.push_back(armedInstruments[i]);
        }
        for (int i = 0; i < audioFileNames.size(); ++i) {
            audioFileNamesVec.push_back(audioFileNames[i]);
        }

        // Get the Sequencer to prepare itself for recording - if
        // this fails we stop.
        //
        if (m_driver->record(RECORD_ON,
                             &armedInstrumentsVec,
                             &audioFileNamesVec) == false) {
            stop();
            return 0;
        }
    } else {
        // unrecognised type - return a problem
        return 0;
    }

    // Now set the local transport status to the record mode
    //
    //
    m_transportStatus = localRecordMode;

    if (localRecordMode == RECORDING) { // punch in
        return 1;
    } else {

        // Ensure that playback is initialised
        //
        m_driver->initialisePlayback(m_songPosition);

        return play(time, readAhead, audioMix, audioRead, audioWrite, smallFileSize);
    }
}

// We receive a starting time from the GUI which we use as the
// basis of our first fetch of events from the GUI core.  Assuming
// this works we set our internal state to PLAYING and go ahead
// and play the piece until we get a signal to stop.
//
// DCOP wants us to use an int as a return type instead of a bool.
//
int
RosegardenSequencerApp::play(const RealTime &time,
                             const RealTime &readAhead,
                             const RealTime &audioMix,
                             const RealTime &audioRead,
                             const RealTime &audioWrite,
                             long smallFileSize)
{
    if (m_transportStatus == PLAYING ||
            m_transportStatus == STARTING_TO_PLAY)
        return true;

    // Check for record toggle (punch out)
    //
    if (m_transportStatus == RECORDING) {
        m_transportStatus = PLAYING;
        return punchOut();
    }

    // To play from the given song position sets up the internal
    // play state to "STARTING_TO_PLAY" which is then caught in
    // the main event loop
    //
    m_songPosition = time;

    if (m_sequencerMapper.getSequencerDataBlock()) {
        m_sequencerMapper.getSequencerDataBlock()->setPositionPointer
        (m_songPosition);
    }

    if (m_transportStatus != RECORDING &&
            m_transportStatus != STARTING_TO_RECORD) {
        m_transportStatus = STARTING_TO_PLAY;
    }

    m_driver->stopClocks();

    // Set up buffer size
    //
    m_readAhead = readAhead;
    if (m_readAhead == RealTime::zeroTime)
        m_readAhead.sec = 1;

    m_audioMix = audioMix;
    m_audioRead = audioRead;
    m_audioWrite = audioWrite;
    m_smallFileSize = smallFileSize;

    m_driver->setAudioBufferSizes(m_audioMix, m_audioRead, m_audioWrite,
                                  m_smallFileSize);

    cleanupMmapData();

    // Map all segments
    //
    QDir segmentsDir(m_segmentFilesPath, "segment_*");
    for (unsigned int i = 0; i < segmentsDir.count(); ++i) {
        mmapSegment(m_segmentFilesPath + "/" + segmentsDir[i]);
    }

    QString tmpDir = KGlobal::dirs()->resourceDirs("tmp").last();

    // Map metronome
    //
    QString metronomeFileName = tmpDir + "/rosegarden_metronome";
    QFileInfo metronomeFileInfo(metronomeFileName);
    if (metronomeFileInfo.exists())
        mmapSegment(metronomeFileName);
    else
        SEQUENCER_DEBUG << "RosegardenSequencerApp::play() - no metronome found\n";

    // Map tempo segment
    //
    QString tempoSegmentFileName = tmpDir + "/rosegarden_tempo";
    QFileInfo tempoSegmentFileInfo(tempoSegmentFileName);
    if (tempoSegmentFileInfo.exists())
        mmapSegment(tempoSegmentFileName);
    else
        SEQUENCER_DEBUG << "RosegardenSequencerApp::play() - no tempo segment found\n";

    // Map time sig segment
    //
    QString timeSigSegmentFileName = tmpDir + "/rosegarden_timesig";
    QFileInfo timeSigSegmentFileInfo(timeSigSegmentFileName);
    if (timeSigSegmentFileInfo.exists())
        mmapSegment(timeSigSegmentFileName);
    else
        SEQUENCER_DEBUG << "RosegardenSequencerApp::play() - no time sig segment found\n";

    // Map control block if necessary
    //
    if (!m_controlBlockMmapper) {
        m_controlBlockMmapper = new ControlBlockMmapper(tmpDir + "/rosegarden_control_block");
        m_sequencerMapper.setControlBlock(m_controlBlockMmapper->getControlBlock());
    }

    initMetaIterator();

    // report
    //
    SEQUENCER_DEBUG << "RosegardenSequencerApp::play() - starting to play\n";

    // Test bits
    //     m_metaIterator = new MmappedSegmentsMetaIterator(m_mmappedSegments);
    //     MappedComposition testCompo;
    //     m_metaIterator->fillCompositionWithEventsUntil(&testCompo,
    //                                                    RealTime(2,0));

    //     dumpFirstSegment();

    // keep it simple
    return true;
}

int
RosegardenSequencerApp::punchOut()
{
    // Check for record toggle (punch out)
    //
    if (m_transportStatus == RECORDING) {
        m_driver->punchOut();
        m_transportStatus = PLAYING;
        return true;
    }
    return false;
}

MmappedSegment* RosegardenSequencerApp::mmapSegment(const QString& file)
{
    MmappedSegment* m = 0;

    try {
        m = new MmappedSegment(file);
    } catch (Exception e) {
        SEQUENCER_DEBUG << "RosegardenSequencerApp::mmapSegment() - couldn't map file " << file
        << " : " << e.getMessage().c_str() << endl;
        return 0;
    }


    m_mmappedSegments[file] = m;
    return m;
}

void RosegardenSequencerApp::initMetaIterator()
{
    delete m_metaIterator;
    m_metaIterator = new MmappedSegmentsMetaIterator(m_mmappedSegments, m_controlBlockMmapper);
}

void RosegardenSequencerApp::cleanupMmapData()
{
    for (MmappedSegmentsMetaIterator::mmappedsegments::iterator i =
                m_mmappedSegments.begin(); i != m_mmappedSegments.end(); ++i)
        delete i->second;

    m_mmappedSegments.clear();

    delete m_metaIterator;
    m_metaIterator = 0;
}

void RosegardenSequencerApp::remapSegment(const QString& filename, size_t newSize)
{
    if (m_transportStatus != PLAYING)
        return ;

    SEQUENCER_DEBUG << "RosegardenSequencerApp::remapSegment(" << filename << ")\n";

    MmappedSegment* m = m_mmappedSegments[filename];
    if (m->remap(newSize) && m_metaIterator)
        m_metaIterator->resetIteratorForSegment(filename);
}

void RosegardenSequencerApp::addSegment(const QString& filename)
{
    if (m_transportStatus != PLAYING)
        return ;

    SEQUENCER_DEBUG << "MmappedSegment::addSegment(" << filename << ")\n";

    MmappedSegment* m = mmapSegment(filename);

    if (m_metaIterator)
        m_metaIterator->addSegment(m);
}

void RosegardenSequencerApp::deleteSegment(const QString& filename)
{
    if (m_transportStatus != PLAYING)
        return ;

    SEQUENCER_DEBUG << "MmappedSegment::deleteSegment(" << filename << ")\n";

    MmappedSegment* m = m_mmappedSegments[filename];

    if (m_metaIterator)
        m_metaIterator->deleteSegment(m);

    delete m;

    // #932415
    m_mmappedSegments.erase(filename);
}

void RosegardenSequencerApp::closeAllSegments()
{
    SEQUENCER_DEBUG << "MmappedSegment::closeAllSegments()\n";

    for (MmappedSegmentsMetaIterator::mmappedsegments::iterator
            i = m_mmappedSegments.begin();
            i != m_mmappedSegments.end(); ++i) {
        if (m_metaIterator)
            m_metaIterator->deleteSegment(i->second);

        delete i->second;
    }

    m_mmappedSegments.clear();

    m_sequencerMapper.setControlBlock(0);
    delete m_controlBlockMmapper;
    m_controlBlockMmapper = 0;
}

void RosegardenSequencerApp::remapTracks()
{
//    SEQUENCER_DEBUG << "RosegardenSequencerApp::remapTracks" << endl;
    std::cout << "RosegardenSequencerApp::remapTracks" << std::endl;

    rationalisePlayingAudio();
}

// DCOP Wrapper for play(RealTime,
//                       RealTime,
//                       RealTime)
//
//
int
RosegardenSequencerApp::play(long timeSec,
                             long timeNSec,
                             long readAheadSec,
                             long readAheadNSec,
                             long audioMixSec,
                             long audioMixNsec,
                             long audioReadSec,
                             long audioReadNsec,
                             long audioWriteSec,
                             long audioWriteNsec,
                             long smallFileSize)

{
    return play(RealTime(timeSec, timeNSec),
                RealTime(readAheadSec, readAheadNSec),
                RealTime(audioMixSec, audioMixNsec),
                RealTime(audioReadSec, audioReadNsec),
                RealTime(audioWriteSec, audioWriteNsec),
                smallFileSize);
}



// Wrapper for record(RealTime,
//                    RealTime,
//                    RealTime,
//                    recordMode);
//
//
int
RosegardenSequencerApp::record(long timeSec,
                               long timeNSec,
                               long readAheadSec,
                               long readAheadNSec,
                               long audioMixSec,
                               long audioMixNsec,
                               long audioReadSec,
                               long audioReadNsec,
                               long audioWriteSec,
                               long audioWriteNsec,
                               long smallFileSize,
                               long recordMode)

{
    return record(RealTime(timeSec, timeNSec),
                  RealTime(readAheadSec, readAheadNSec),
                  RealTime(audioMixSec, audioMixNsec),
                  RealTime(audioReadSec, audioReadNsec),
                  RealTime(audioWriteSec, audioWriteNsec),
                  smallFileSize,
                  recordMode);
}


void
RosegardenSequencerApp::setLoop(const RealTime &loopStart,
                                const RealTime &loopEnd)
{
    m_loopStart = loopStart;
    m_loopEnd = loopEnd;

    m_driver->setLoop(loopStart, loopEnd);
}


void
RosegardenSequencerApp::setLoop(long loopStartSec,
                                long loopStartNSec,
                                long loopEndSec,
                                long loopEndNSec)
{
    setLoop(RealTime(loopStartSec, loopStartNSec),
            RealTime(loopEndSec, loopEndNSec));
}


// Return the status of the sound systems (audio and MIDI)
//
unsigned int
RosegardenSequencerApp::getSoundDriverStatus(const QString &guiVersion)
{
    unsigned int driverStatus = m_driver->getStatus();
    if (guiVersion == VERSION)
        driverStatus |= VERSION_OK;
    else {
        std::cerr << "WARNING: RosegardenSequencerApp::getSoundDriverStatus: "
        << "GUI version \"" << guiVersion
        << "\" does not match sequencer version \"" << VERSION
        << "\"" << std::endl;
    }
    return driverStatus;
}


// Add an audio file to the sequencer
int
RosegardenSequencerApp::addAudioFile(const QString &fileName, int id)
{
    return ((int)m_driver->addAudioFile(fileName.utf8().data(), id));
}

int
RosegardenSequencerApp::removeAudioFile(int id)
{
    return ((int)m_driver->removeAudioFile(id));
}

void
RosegardenSequencerApp::clearAllAudioFiles()
{
    m_driver->clearAudioFiles();
}

void
RosegardenSequencerApp::setMappedInstrument(int type, unsigned char channel,
        unsigned int id)
{
    InstrumentId mID = (InstrumentId)id;
    Instrument::InstrumentType mType =
        (Instrument::InstrumentType)type;
    MidiByte mChannel = (MidiByte)channel;

    m_driver->setMappedInstrument(
        new MappedInstrument (mType, mChannel, mID));

}

// Process a MappedComposition sent from Sequencer with
// immediate effect
//
void
RosegardenSequencerApp::processSequencerSlice(MappedComposition mC)
{
    // Use the "now" API
    //
    m_driver->processEventsOut(mC);
}

void
RosegardenSequencerApp::processMappedEvent(unsigned int id,
        int type,
        unsigned char pitch,
        unsigned char velocity,
        long absTimeSec,
        long absTimeNsec,
        long durationSec,
        long durationNsec,
        long audioStartMarkerSec,
        long audioStartMarkerNSec)
{
    MappedEvent *mE =
        new MappedEvent(
            (InstrumentId)id,
            (MappedEvent::MappedEventType)type,
            (MidiByte)pitch,
            (MidiByte)velocity,
            RealTime(absTimeSec, absTimeNsec),
            RealTime(durationSec, durationNsec),
            RealTime(audioStartMarkerSec, audioStartMarkerNSec));

    MappedComposition mC;

    //    SEQUENCER_DEBUG << "processMappedEvent(data) - sending out single event at time " << mE->getEventTime() << endl;

    /*
    std::cout << "ID = " << mE->getInstrument() << std::endl;
    std::cout << "TYPE = " << mE->getType() << std::endl;
    std::cout << "D1 = " << (int)mE->getData1() << std::endl;
    std::cout << "D2 = " << (int)mE->getData2() << std::endl;
    */

    mC.insert(mE);

    m_driver->processEventsOut(mC);
}

void
RosegardenSequencerApp::processMappedEvent(MappedEvent mE)
{
    MappedComposition mC;
    mC.insert(new MappedEvent(mE));
    SEQUENCER_DEBUG << "processMappedEvent(ev) - sending out single event at time " << mE.getEventTime() << endl;

    m_driver->processEventsOut(mC);
}

// Get the MappedDevice (DCOP wrapped vector of MappedInstruments)
//
MappedDevice
RosegardenSequencerApp::getMappedDevice(unsigned int id)
{
    return m_driver->getMappedDevice(id);
}

unsigned int
RosegardenSequencerApp::getDevices()
{
    return m_driver->getDevices();
}

int
RosegardenSequencerApp::canReconnect(int type)
{
    return m_driver->canReconnect((Device::DeviceType)type);
}

unsigned int
RosegardenSequencerApp::addDevice(int type, unsigned int direction)
{
    return m_driver->addDevice((Device::DeviceType)type,
                               (MidiDevice::DeviceDirection)direction);
}

void
RosegardenSequencerApp::removeDevice(unsigned int deviceId)
{
    m_driver->removeDevice(deviceId);
}

void
RosegardenSequencerApp::renameDevice(unsigned int deviceId, QString name)
{
    m_driver->renameDevice(deviceId, name);
}

unsigned int
RosegardenSequencerApp::getConnections(int type, unsigned int direction)
{
    return m_driver->getConnections((Device::DeviceType)type,
                                    (MidiDevice::DeviceDirection)direction);
}

QString
RosegardenSequencerApp::getConnection(int type, unsigned int direction,
                                      unsigned int connectionNo)
{
    return m_driver->getConnection((Device::DeviceType)type,
                                   (MidiDevice::DeviceDirection)direction,
                                   connectionNo);
}

void
RosegardenSequencerApp::setConnection(unsigned int deviceId,
                                      QString connection)
{
    m_driver->setConnection(deviceId, connection);
}

void
RosegardenSequencerApp::setPlausibleConnection(unsigned int deviceId,
        QString connection)
{
    m_driver->setPlausibleConnection(deviceId, connection);
}

unsigned int
RosegardenSequencerApp::getTimers()
{
    return m_driver->getTimers();
}

QString
RosegardenSequencerApp::getTimer(unsigned int n)
{
    return m_driver->getTimer(n);
}

QString
RosegardenSequencerApp::getCurrentTimer()
{
    return m_driver->getCurrentTimer();
}

void
RosegardenSequencerApp::setCurrentTimer(QString timer)
{
    m_driver->setCurrentTimer(timer);
}

void
RosegardenSequencerApp::setLowLatencyMode(bool ll)
{
    m_driver->setLowLatencyMode(ll);
}

void
RosegardenSequencerApp::sequencerAlive()
{
    if (!kapp->dcopClient()->
            isApplicationRegistered(QCString(ROSEGARDEN_GUI_APP_NAME))) {
        SEQUENCER_DEBUG << "RosegardenSequencerApp::sequencerAlive() - "
        << "waiting for GUI to register" << endl;
        return ;
    }

    QByteArray data;

    if (!kapp->dcopClient()->send(ROSEGARDEN_GUI_APP_NAME,
                                  ROSEGARDEN_GUI_IFACE_NAME,
                                  "alive()",
                                  data)) {
        SEQUENCER_DEBUG << "RosegardenSequencer::alive()"
        << " - can't call RosegardenGUI client"
        << endl;
    }

    SEQUENCER_DEBUG << "RosegardenSequencerApp::sequencerAlive() - "
    << "trying to tell GUI that we're alive" << endl;
}

MappedRealTime
RosegardenSequencerApp::getAudioPlayLatency()
{
    return MappedRealTime(m_driver->getAudioPlayLatency());
}

MappedRealTime
RosegardenSequencerApp::getAudioRecordLatency()
{
    return MappedRealTime(m_driver->getAudioRecordLatency());
}

// Initialise the virtual studio with a few audio faders and
// create a plugin manager.  For the moment this is pretty
// arbitrary but eventually we'll drive this from the gui
// and rg file "Studio" entries.
//
void
RosegardenSequencerApp::initialiseStudio()
{
    // clear down the studio before we start adding anything
    //
    m_studio->clear();
}


void
RosegardenSequencerApp::setMappedProperty(int id,
        const QString &property,
        float value)
{

    //    SEQUENCER_DEBUG << "setProperty: id = " << id
    //                    << " : property = \"" << property << "\""
    //		    << ", value = " << value << endl;


    MappedObject *object = m_studio->getObjectById(id);

    if (object)
        object->setProperty(property, value);
}

void
RosegardenSequencerApp::setMappedProperties(const MappedObjectIdList &ids,
        const MappedObjectPropertyList &properties,
        const MappedObjectValueList &values)
{
    MappedObject *object = 0;
    MappedObjectId prevId = 0;

    for (size_t i = 0;
            i < ids.size() && i < properties.size() && i < values.size();
            ++i) {

        if (i == 0 || ids[i] != prevId) {
            object = m_studio->getObjectById(ids[i]);
            prevId = ids[i];
        }

        if (object) {
            object->setProperty(properties[i], values[i]);
        }
    }
}

void
RosegardenSequencerApp::setMappedProperty(int id,
        const QString &property,
        const QString &value)
{

    SEQUENCER_DEBUG << "setProperty: id = " << id
    << " : property = \"" << property << "\""
    << ", value = " << value << endl;


    MappedObject *object = m_studio->getObjectById(id);

    if (object)
        object->setProperty(property, value);
}

void
RosegardenSequencerApp::setMappedPropertyList(int id, const QString &property,
        const MappedObjectPropertyList &values)
{
    SEQUENCER_DEBUG << "setPropertyList: id = " << id
    << " : property list size = \"" << values.size()
    << "\"" << endl;

    MappedObject *object = m_studio->getObjectById(id);

    if (object) {
        try {
            object->setPropertyList(property, values);
        } catch (QString err) {
            QByteArray data;
            QDataStream arg(data, IO_WriteOnly);
            arg << err;
            kapp->dcopClient()->send(ROSEGARDEN_GUI_APP_NAME,
                                     ROSEGARDEN_GUI_IFACE_NAME,
                                     "showError(QString)",
                                     data);
        }
    }
}


int
RosegardenSequencerApp::getMappedObjectId(int type)
{
    int value = -1;

    MappedObject *object =
        m_studio->getObjectOfType(
            MappedObject::MappedObjectType(type));

    if (object) {
        value = int(object->getId());
    }

    return value;
}


std::vector<QString>
RosegardenSequencerApp::getPropertyList(int id,
                                        const QString &property)
{
    std::vector<QString> list;

    MappedObject *object =
        m_studio->getObjectById(id);

    if (object) {
        list = object->getPropertyList(property);
    }

    SEQUENCER_DEBUG << "getPropertyList - return " << list.size()
    << " items" << endl;

    return list;
}

std::vector<QString>
RosegardenSequencerApp::getPluginInformation()
{
    std::vector<QString> list;

    PluginFactory::enumerateAllPlugins(list);

    return list;
}

QString
RosegardenSequencerApp::getPluginProgram(int id, int bank, int program)
{
    MappedObject *object = m_studio->getObjectById(id);

    if (object) {
        MappedPluginSlot *slot =
            dynamic_cast<MappedPluginSlot *>(object);
        if (slot) {
            return slot->getProgram(bank, program);
        }
    }

    return QString();
}

unsigned long
RosegardenSequencerApp::getPluginProgram(int id, const QString &name)
{
    MappedObject *object = m_studio->getObjectById(id);

    if (object) {
        MappedPluginSlot *slot =
            dynamic_cast<MappedPluginSlot *>(object);
        if (slot) {
            return slot->getProgram(name);
        }
    }

    return 0;
}

unsigned int
RosegardenSequencerApp::getSampleRate() const
{
    if (m_driver)
        return m_driver->getSampleRate();

    return 0;
}

// Creates an object of a type
//
int
RosegardenSequencerApp::createMappedObject(int type)
{
    MappedObject *object =
        m_studio->createObject(
            MappedObject::MappedObjectType(type));

    if (object) {
        SEQUENCER_DEBUG << "createMappedObject - type = "
        << type << ", object id = "
        << object->getId() << endl;
        return object->getId();
    }

    return 0;
}

// Destroy an object
//
int
RosegardenSequencerApp::destroyMappedObject(int id)
{
    return (int(m_studio->destroyObject(MappedObjectId(id))));
}

// Connect two objects
//
void
RosegardenSequencerApp::connectMappedObjects(int id1, int id2)
{
    m_studio->connectObjects(MappedObjectId(id1),
                             MappedObjectId(id2));

    // When this happens we need to resynchronise our audio processing,
    // and this is the easiest (and most brutal) way to do it.
    if (m_transportStatus == PLAYING ||
            m_transportStatus == RECORDING) {
        RealTime seqTime = m_driver->getSequencerTime();
        jumpTo(seqTime.sec, seqTime.nsec);
    }
}

// Disconnect two objects
//
void
RosegardenSequencerApp::disconnectMappedObjects(int id1, int id2)
{
    m_studio->disconnectObjects(MappedObjectId(id1),
                                MappedObjectId(id2));
}

// Disconnect an object from everything
//
void
RosegardenSequencerApp::disconnectMappedObject(int id)
{
    m_studio->disconnectObject(MappedObjectId(id));
}


void
RosegardenSequencerApp::clearStudio()
{
    SEQUENCER_DEBUG << "clearStudio()" << endl;
    m_studio->clear();
    m_sequencerMapper.getSequencerDataBlock()->clearTemporaries();

}

void
RosegardenSequencerApp::setMappedPort(int pluginId,
                                      unsigned long portId,
                                      float value)
{
    MappedObject *object =
        m_studio->getObjectById(pluginId);

    MappedPluginSlot *slot =
        dynamic_cast<MappedPluginSlot *>(object);

    if (slot) {
        slot->setPort(portId, value);
    } else {
        SEQUENCER_DEBUG << "no such slot" << endl;
    }
}

float
RosegardenSequencerApp::getMappedPort(int pluginId,
                                      unsigned long portId)
{
    MappedObject *object =
        m_studio->getObjectById(pluginId);

    MappedPluginSlot *slot =
        dynamic_cast<MappedPluginSlot *>(object);

    if (slot) {
        return slot->getPort(portId);
    } else {
        SEQUENCER_DEBUG << "no such slot" << endl;
    }

    return 0;
}

void
RosegardenSequencerApp::slotCheckForNewClients()
{
    // Don't do this check if any of these conditions hold
    //
    if (m_transportStatus == PLAYING ||
            m_transportStatus == RECORDING)
        return ;

    if (m_driver->checkForNewClients()) {
        SEQUENCER_DEBUG << "client list changed" << endl;
    }
}


// Set the MIDI Clock period in microseconds
//
void
RosegardenSequencerApp::setQuarterNoteLength(long timeSec, long timeNSec)
{
    SEQUENCER_DEBUG << "RosegardenSequencerApp::setQuarterNoteLength"
    << RealTime(timeSec, timeNSec) << endl;

    m_driver->setMIDIClockInterval(
        RealTime(timeSec, timeNSec) / 24);
}

QString
RosegardenSequencerApp::getStatusLog()
{
    return m_driver->getStatusLog();
}


void RosegardenSequencerApp::dumpFirstSegment()
{
    SEQUENCER_DEBUG << "Dumping 1st segment data :\n";

    unsigned int i = 0;
    MmappedSegment* firstMappedSegment = (*(m_mmappedSegments.begin())).second;

    MmappedSegment::iterator it(firstMappedSegment);

    for (; !it.atEnd(); ++it) {

        MappedEvent evt = (*it);
        SEQUENCER_DEBUG << i << " : inst = " << evt.getInstrument()
        << " - type = " << evt.getType()
        << " - data1 = " << (unsigned int)evt.getData1()
        << " - data2 = " << (unsigned int)evt.getData2()
        << " - time = " << evt.getEventTime()
        << " - duration = " << evt.getDuration()
        << " - audio mark = " << evt.getAudioStartMarker()
        << endl;

        ++i;
    }

    SEQUENCER_DEBUG << "Dumping 1st segment data - done\n";

}


void
RosegardenSequencerApp::rationalisePlayingAudio()
{
    std::vector<MappedEvent> audioEvents;
    m_metaIterator->getAudioEvents(audioEvents);
    m_driver->initialiseAudioQueue(audioEvents);
}


ExternalTransport::TransportToken
RosegardenSequencerApp::transportChange(TransportRequest request)
{
    TransportPair pair(request, RealTime::zeroTime);
    m_transportRequests.push_back(pair);

    std::cout << "RosegardenSequencerApp::transportChange: " << request << std::endl;

    if (request == TransportNoChange)
        return m_transportToken;
    else
        return m_transportToken + 1;
}

ExternalTransport::TransportToken
RosegardenSequencerApp::transportJump(TransportRequest request,
                                      RealTime rt)
{
    TransportPair pair(request, rt);
    m_transportRequests.push_back(pair);

    std::cout << "RosegardenSequencerApp::transportJump: " << request << ", " << rt << std::endl;

    if (request == TransportNoChange)
        return m_transportToken + 1;
    else
        return m_transportToken + 2;
}

bool
RosegardenSequencerApp::isTransportSyncComplete(TransportToken token)
{
    std::cout << "RosegardenSequencerApp::isTransportSyncComplete: token " << token << ", current token " << m_transportToken << std::endl;
    return m_transportToken >= token;
}

bool
RosegardenSequencerApp::checkExternalTransport()
{
    bool rv = (!m_transportRequests.empty());

    while (!m_transportRequests.empty()) {

        TransportPair pair = *m_transportRequests.begin();
        m_transportRequests.pop_front();

        QByteArray data;

        switch (pair.first) {

        case TransportNoChange:
            break;

        case TransportStop:
            kapp->dcopClient()->send(ROSEGARDEN_GUI_APP_NAME,
                                     ROSEGARDEN_GUI_IFACE_NAME,
                                     "stop()",
                                     data);
            break;

        case TransportStart:
            kapp->dcopClient()->send(ROSEGARDEN_GUI_APP_NAME,
                                     ROSEGARDEN_GUI_IFACE_NAME,
                                     "play()",
                                     data);
            break;

        case TransportPlay:
            kapp->dcopClient()->send(ROSEGARDEN_GUI_APP_NAME,
                                     ROSEGARDEN_GUI_IFACE_NAME,
                                     "play()",
                                     data);
            break;

        case TransportRecord:
            kapp->dcopClient()->send(ROSEGARDEN_GUI_APP_NAME,
                                     ROSEGARDEN_GUI_IFACE_NAME,
                                     "record()",
                                     data);
            break;

        case TransportJumpToTime: {
                QDataStream arg(data, IO_WriteOnly);
                arg << (int)pair.second.sec;
                arg << (int)pair.second.usec();

                kapp->dcopClient()->send(ROSEGARDEN_GUI_APP_NAME,
                                         ROSEGARDEN_GUI_IFACE_NAME,
                                         "jumpToTime(int, int)",
                                         data);

                if (m_transportStatus == PLAYING ||
                        m_transportStatus != RECORDING) {
                    jumpTo(pair.second.sec, pair.second.usec() * 1000);
                }

                incrementTransportToken();
                break;
            }

        case TransportStartAtTime: {
                QDataStream arg(data, IO_WriteOnly);
                arg << (int)pair.second.sec;
                arg << (int)pair.second.usec();

                kapp->dcopClient()->send(ROSEGARDEN_GUI_APP_NAME,
                                         ROSEGARDEN_GUI_IFACE_NAME,
                                         "startAtTime(int, int)",
                                         data);
                break;
            }

        case TransportStopAtTime: {
                kapp->dcopClient()->send(ROSEGARDEN_GUI_APP_NAME,
                                         ROSEGARDEN_GUI_IFACE_NAME,
                                         "stop()",
                                         data);

                QDataStream arg(data, IO_WriteOnly);
                arg << (int)pair.second.sec;
                arg << (int)pair.second.usec();

                kapp->dcopClient()->send(ROSEGARDEN_GUI_APP_NAME,
                                         ROSEGARDEN_GUI_IFACE_NAME,
                                         "jumpToTime(int, int)",
                                         data);
                break;
            }
        }
    }

    return rv;
}

void
RosegardenSequencerApp::incrementTransportToken()
{
    ++m_transportToken;
    SEQUENCER_DEBUG << "RosegardenSequencerApp::incrementTransportToken: incrementing to " << m_transportToken << endl;
}

}

#include "RosegardenSequencerApp.moc"
