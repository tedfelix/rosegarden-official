// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-
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

#include <iostream>

#include <klocale.h>
#include <kstandarddirs.h>

#include <dcopclient.h>
#include <qdatetime.h>
#include <qstring.h>
#include <qdir.h>
#include <qbuffer.h>

#include "rosedebug.h"
#include "rosegardensequencer.h"
#include "mmappedcontrolblock.h"
#include "mmappedsegment.h"
#include "rosegardendcop.h"
#include "ControlBlock.h"
#include "Sequencer.h"
#include "MappedInstrument.h"

// The default latency and read-ahead values are actually sent
// down from the GUI every time playback or recording starts
// so the local values are kind of meaningless.
//
//
RosegardenSequencerApp::RosegardenSequencerApp(
        const std::vector<std::string> &jackArgs):
    DCOPObject("RosegardenSequencerIface"),
    m_sequencer(0),
    m_transportStatus(STOPPED),
    m_songPosition(0, 0),
    m_lastFetchSongPosition(0, 0),
    m_fetchLatency(0, 30000),      // default value
    m_playLatency(0, 50000),       // default value
    m_readAhead(0, 40000),         // default value
    //m_audioPlayLatency(0, 0),
    //m_audioRecordLatency(0, 0),
    m_loopStart(0, 0),
    m_loopEnd(0, 0),
    m_studio(new Rosegarden::MappedStudio()),
    m_oldSliceSize(0, 0),
    m_segmentFilesPath(KGlobal::dirs()->resourceDirs("tmp").first()),
    m_metaIterator(0),
    m_controlBlockMmapper(0),
    m_isEndOfCompReached(false)
{
    SEQUENCER_DEBUG << "Registering with DCOP server" << endl;

    // Without DCOP we are nothing
    QCString realAppId = kapp->dcopClient()->registerAs(kapp->name(), false);

    if (realAppId.isNull())
    {
        SEQUENCER_DEBUG << "RosegardenSequencer cannot register "
                        << "with DCOP server" << endl;
        close();
    }

    // Initialise the MappedStudio
    //
    initialiseStudio();

    // Creating this object also initialises the Rosegarden aRts or
    // ALSA/JACK interface for both playback and recording. MappedStudio
    // aduio faders are also created.
    //
    m_sequencer = new Rosegarden::Sequencer(m_studio, jackArgs);
    m_studio->setSequencer(m_sequencer);

    if (!m_sequencer)
    {
        SEQUENCER_DEBUG << "RosegardenSequencer object could not be allocated"
                        << endl;
        close();
    }

    // set this here and now so we can accept async midi events
    //
    m_sequencer->record(Rosegarden::ASYNCHRONOUS_MIDI);

    // Setup the slice timer
    //
    m_sliceTimer = new QTimer(this);
//    connect(m_sliceTimer, SIGNAL(timeout()), this, SLOT(slotRevertSliceSize()));

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
    delete m_sequencer;
    delete m_studio;
    delete m_controlBlockMmapper;
}

void
RosegardenSequencerApp::quit()
{
    close();

    // and break out of the loop next time around
    m_transportStatus = QUIT;
}


// We just "send" to this - no call (removed post 0.1
// to prevent hangs)
//
void
RosegardenSequencerApp::stop()
{
    // set our state at this level to STOPPING (pending any
    // unfinished NOTES)
    m_transportStatus = STOPPING;

    // process pending NOTE OFFs and stop the Sequencer
    m_sequencer->stopPlayback();

    // the Sequencer doesn't need to know these once
    // we've stopped.
    //
    m_songPosition.sec = 0;
    m_songPosition.usec = 0;
    m_lastFetchSongPosition.sec = 0;
    m_lastFetchSongPosition.usec = 0;


    cleanupMmapData();

    // report
    //
    SEQUENCER_DEBUG << "RosegardenSequencerApp::stop() - stopping" << endl;

}

// Get a slice of events from the GUI
//
Rosegarden::MappedComposition*
RosegardenSequencerApp::fetchEvents(const Rosegarden::RealTime &start,
                                    const Rosegarden::RealTime &end,
                                    bool firstFetch)
{
    // Always return nothing if we're stopped
    //
    if ( m_transportStatus == STOPPED || m_transportStatus == STOPPING )
        return 0;

    // If we're looping then we should get as much of the rest of
    // the right hand of the loop as possible and also events from
    // the beginning of the loop.  We can do this in two fetches.
    // Make sure that we delete all returned pointers when we've
    // finished with them.
    //
    //
    if (isLooping() == true && end >= m_loopEnd)
    {
        Rosegarden::RealTime loopOverlap = end - m_loopEnd;

        Rosegarden::MappedComposition *endLoop = 0;

	if (m_loopEnd > start) {
	    endLoop = getSlice(start, m_loopEnd, firstFetch);
	}

	if (loopOverlap > Rosegarden::RealTime::zeroTime) { 

	    Rosegarden::MappedComposition *beginLoop =
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
	else return new Rosegarden::MappedComposition();
    }
    else
        return getSlice(start, end, firstFetch);
}


Rosegarden::MappedComposition*
RosegardenSequencerApp::getSlice(const Rosegarden::RealTime &start,
                                 const Rosegarden::RealTime &end,
                                 bool firstFetch)
{
    Rosegarden::MappedComposition *mC = new Rosegarden::MappedComposition();

    SEQUENCER_DEBUG << "RosegardenSequencerApp::getSlice (" << start << " -> " << end << ", " << firstFetch << ")" << endl;

    if (firstFetch || (start < m_lastStartTime)) {
	SEQUENCER_DEBUG << "[calling jumpToTime on start]" << endl;
        m_metaIterator->jumpToTime(start);
    }

    (void)m_metaIterator->fillCompositionWithEventsUntil(firstFetch, mC, start, end);

//     setEndOfCompReached(eventsRemaining); // don't do that, it breaks recording because
// playing stops right after it starts.

    m_lastStartTime = start;

    return mC;
}


// The first fetch of events from the core/ and initialisation for
// this session of playback.  We fetch up to m_playLatency microseconds/
// seconds ahead at first at then top up once we're within m_fetchLatency
// of the end of the last fetch.
//
bool
RosegardenSequencerApp::startPlaying()
{
    // Fetch up to m_readHead microseconds worth of events
    //
    m_lastFetchSongPosition = m_songPosition + m_readAhead;

    // This will reset the Sequencer's internal clock
    // ready for new playback
    m_sequencer->initialisePlayback(m_songPosition, m_playLatency);

    // Ensure that the audio playing checks are cleared down
    //
    m_metaIterator->clearPlayingAudioSegments();

    m_mC.clear();
    m_mC = *fetchEvents(m_songPosition, m_songPosition + m_readAhead, true);

    // process whether we need to or not as this also processes
    // the audio queue for us
    //
    m_sequencer->processEventsOut(m_mC, m_playLatency, false);

    // tell the gui about this slice of events
    notifyVisuals(&m_mC);

    return true; // !isEndOfCompReached();
}

void
RosegardenSequencerApp::notifyVisuals(Rosegarden::MappedComposition *mC)
{
    // Tell the gui that we're processing these events next
    //
    QByteArray data;
    QDataStream arg(data, IO_WriteOnly);
    arg << *mC;

    if (!kapp->dcopClient()->send(ROSEGARDEN_GUI_APP_NAME,
                                  ROSEGARDEN_GUI_IFACE_NAME,
                                 "showVisuals(Rosegarden::MappedComposition)",
                                  data))
    {
        SEQUENCER_DEBUG << "RosegardenSequencer::showVisuals()"
                        << " - can't call RosegardenGUI client"
                        << endl;
    }
}

bool
RosegardenSequencerApp::keepPlaying()
{
    if (m_songPosition > ( m_lastFetchSongPosition - m_fetchLatency)) {

        m_mC.clear();
        m_mC = *fetchEvents(m_lastFetchSongPosition,
                            m_lastFetchSongPosition + m_readAhead,
                            false);

        // Again, process whether we need to or not to keep
        // the Sequencer up-to-date with audio events
        //
        m_sequencer->processEventsOut(m_mC, m_playLatency, false);

        // tell the gui about this slice of events
        notifyVisuals(&m_mC);

        m_lastFetchSongPosition = m_lastFetchSongPosition + m_readAhead;

        // Ensure that the audio we're playing is the audio we should be playing
        //
        if (m_metaIterator)
        {
            rationalisePlayingAudio(m_metaIterator->getPlayingAudioSegments(m_songPosition));
        }

    }

    return true; // !isEndOfCompReached(); - until we sort this out, we don't stop at end of comp.
}

// Return current Sequencer time in GUI compatible terms
// remembering that our playback is delayed by m_playLatency
// ticks from our current m_songPosition.
//
void
RosegardenSequencerApp::updateClocks()
{
    // Attempt to send MIDI clock 
    //
    m_sequencer->sendMidiClock(m_playLatency);

    // If we're not playing etc. then that's all we need to do

    if (m_transportStatus != PLAYING &&
        m_transportStatus != RECORDING_MIDI &&
        m_transportStatus != RECORDING_AUDIO)
        return;

    QByteArray data, replyData;
    QCString replyType;
    QDataStream arg(data, IO_WriteOnly);

    Rosegarden::RealTime newPosition = m_sequencer->getSequencerTime();

    // Go around the loop if we've reached the end
    //
    if (isLooping() && newPosition > m_loopEnd + m_playLatency)
    {

        // Remove the loop width from the song position and send
        // this position (minus m_playLatency) to the GUI
        /*
        m_songPosition = newPosition - (m_loopEnd - m_loopStart);
        newPosition = m_songPosition - m_playLatency;
        m_lastFetchSongPosition =
                m_lastFetchSongPosition - (m_loopEnd - m_loopStart);
                */

        // forgetting the fancy stuff brings superior results
        //
        newPosition = m_songPosition = m_lastFetchSongPosition = m_loopStart;

        // Reset playback using this jump
        //
        m_sequencer->resetPlayback(m_loopStart, m_playLatency);


    }
    else
    {
        m_songPosition = newPosition;

        if (m_songPosition > m_sequencer->getStartPosition() + m_playLatency)
            newPosition = newPosition - m_playLatency;
        else
            newPosition = m_sequencer->getStartPosition();
    }

    arg << newPosition.sec;
    arg << newPosition.usec;

//    std::cerr << "Calling setPointerPosition(" << newPosition.sec << "," << newPosition.usec << "," << long(clearToSend) << ")" << std::endl;
    
    if (!kapp->dcopClient()->send(ROSEGARDEN_GUI_APP_NAME,
                      ROSEGARDEN_GUI_IFACE_NAME,
                      "setPointerPosition(long int, long int)",
                      data))
    {
        SEQUENCER_DEBUG << "RosegardenSequencer::updateClocks() - can't send to RosegardenGUI client\n";

        // Stop the sequencer so we can see if we can try again later
        //
        stop();
    }

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
                                  data)) 
    {
        SEQUENCER_DEBUG << "RosegardenSequencer::notifySequencerStatus()"
                        << " - can't send to RosegardenGUI client"
                        << endl;

        // Stop the sequencer
        //
        stop();
    }
}

// Sets the Sequencer object and this object to the new time 
// from where playback can continue.
//
//
void
RosegardenSequencerApp::jumpTo(long posSec, long posUsec)
{
    SEQUENCER_DEBUG << "RosegardenSequencerApp::jumpTo(" << posSec << ", " << posUsec << ")\n";

    if (posSec < 0 && posUsec < 0)
        return;

    m_songPosition = m_lastFetchSongPosition =
            Rosegarden::RealTime(posSec, posUsec);

    m_sequencer->resetPlayback(m_songPosition,
                               m_playLatency);
    return;
}

// Send the last recorded MIDI block
//
void
RosegardenSequencerApp::processRecordedMidi()
{

    QByteArray data; //, replyData;
    //QCString replyType;
    QDataStream arg(data, IO_WriteOnly);

    arg << m_sequencer->getMappedComposition(m_playLatency);


    if (!kapp->dcopClient()->send(ROSEGARDEN_GUI_APP_NAME,
                                  ROSEGARDEN_GUI_IFACE_NAME,
                                 "processRecordedMidi(Rosegarden::MappedComposition)",
                                  data/*, replyType, replyData, true*/))
    {
        SEQUENCER_DEBUG << "RosegardenSequencer::processRecordedMidi() - " 
                        <<   "can't call RosegardenGUI client" 
                        << endl;

        // Stop the sequencer
        //
        stop();
    }
}


// Send an update
//
void
RosegardenSequencerApp::processRecordedAudio()
{
    QByteArray data; //, replyData;
    //QCString replyType;
    QDataStream arg(data, IO_WriteOnly);

    Rosegarden::RealTime time = m_sequencer->getSequencerTime();

    // Send out current time and last audio level
    //
    arg << time.sec;
    arg << time.usec;

    if (!kapp->dcopClient()->send(ROSEGARDEN_GUI_APP_NAME,
                                  ROSEGARDEN_GUI_IFACE_NAME,
                                 "processRecordedAudio(long int, long int)",
                                  data/*, replyType, replyData, true*/))
    {
        SEQUENCER_DEBUG << "RosegardenSequencer::processRecordedMidi() - " 
                        <<   "can't call RosegardenGUI client" << endl;

        // Stop the sequencer
        //
        stop();
    }
}


// This method is called during STOPPED or PLAYING operations
// to mop up any async (unexpected) incoming MIDI or Audio events
// and forward them to the GUI for display
//
//
void
RosegardenSequencerApp::processAsynchronousEvents()
{
    QByteArray data;
    QDataStream arg(data, IO_WriteOnly);

    arg << m_sequencer->getMappedComposition(m_playLatency);

    if (!kapp->dcopClient()->send(ROSEGARDEN_GUI_APP_NAME,
                                 ROSEGARDEN_GUI_IFACE_NAME,
                                 "processAsynchronousMidi(Rosegarden::MappedComposition)", data))
    {
        SEQUENCER_DEBUG << "RosegardenSequencer::processAsynchronousEvents() - "
                        << "can't call RosegardenGUI client" << endl;

        // Stop the sequencer so we can see if we can try again later
        //
        stop();
    }

    // Process any pending events (Note Offs or Audio) as part of
    // same procedure.
    //
    m_sequencer->processPending(m_playLatency);
}



int
RosegardenSequencerApp::record(const Rosegarden::RealTime &time,
                               const Rosegarden::RealTime &playLatency,
                               const Rosegarden::RealTime &readAhead,
                               int recordMode)
{
    TransportStatus localRecordMode = (TransportStatus) recordMode;

    // punch in recording
    if (m_transportStatus == PLAYING)
    {
        if (localRecordMode == STARTING_TO_RECORD_MIDI)
        {
            if(m_sequencer->record(Rosegarden::RECORD_MIDI) == false)
            {
                stop();
                return 0;
            }

            m_transportStatus = RECORDING_MIDI;
            return 1;
        }
        else if (localRecordMode == STARTING_TO_RECORD_AUDIO)
        {
            // do something
        }
    }

    // For audio recording we need to retrieve the input ports
    // we're connected to from the Studio.
    //
    if (localRecordMode == STARTING_TO_RECORD_MIDI)
    {
        SEQUENCER_DEBUG << "RosegardenSequencerApp::record()"
                        << " - starting to record MIDI" << endl;

        // Get the Sequencer to prepare itself for recording - if
        // this fails we stop.
        //
        if(m_sequencer->record(Rosegarden::RECORD_MIDI) == false)
        {
            stop();
            return 0;
        }

    }
    else if (localRecordMode == STARTING_TO_RECORD_AUDIO)
    {
        SEQUENCER_DEBUG << "RosegardenSequencerApp::record()"
                        << " - starting to record Audio" << endl;

        QByteArray data, replyData;
        QCString replyType;
        QDataStream arg(data, IO_WriteOnly);

        if (!kapp->dcopClient()->call(ROSEGARDEN_GUI_APP_NAME,
                                      ROSEGARDEN_GUI_IFACE_NAME,
                                      "createNewAudioFile()",
                                      data, replyType, replyData, true))
        {
            SEQUENCER_DEBUG << "RosegardenSequencer::record()"
                            << " - can't call RosegardenGUI client"
                            << endl;
        }

        QDataStream reply(replyData, IO_ReadOnly);
        QString audioFileName;
        if (replyType == "QString")
        {
            reply >> audioFileName;
        }
        else
        {
            SEQUENCER_DEBUG << "RosegardenSequencer::record() - "
                            << "unrecognised type returned" << endl;
        }

        // set recording filename
        m_sequencer->setRecordingFilename(std::string(audioFileName.data()));

        // set recording
        if (m_sequencer->record(Rosegarden::RECORD_AUDIO) == false)
        {
            SEQUENCER_DEBUG << "couldn't start recording - "
                            << "perhaps audio file path wrong?"
                            << endl;

            stop();
            return 0;
        }
    }
    else
    {
        // unrecognised type - return a problem
        return 0;
    }

    // Now set the local transport status to the record mode
    //
    //
    m_transportStatus = localRecordMode;

    /*

    // Work out the record latency
    Rosegarden::RealTime recordLatency = playLatency;
    if (m_audioRecordLatency > recordLatency)
        recordLatency = m_audioRecordLatency;
    */

    // Ensure that playback is initialised
    //
    m_sequencer->initialisePlayback(m_songPosition, playLatency);

    return play(time, playLatency, readAhead);
}

// We receive a starting time from the GUI which we use as the
// basis of our first fetch of events from the GUI core.  Assuming
// this works we set our internal state to PLAYING and go ahead
// and play the piece until we get a signal to stop.
// 
// DCOP wants us to use an int as a return type instead of a bool.
//
int
RosegardenSequencerApp::play(const Rosegarden::RealTime &time,
                             const Rosegarden::RealTime &playLatency, 
                             const Rosegarden::RealTime &readAhead)
{
    if (m_transportStatus == PLAYING || m_transportStatus == STARTING_TO_PLAY)
        return true;


    // Check for record toggle (punch out)
    //
    if (m_transportStatus == RECORDING_MIDI || m_transportStatus == RECORDING_AUDIO)
    {
        m_transportStatus = PLAYING;
        return true;
    }

    // To play from the given song position sets up the internal
    // play state to "STARTING_TO_PLAY" which is then caught in
    // the main event loop
    //
    m_songPosition = time;

    if (m_transportStatus != RECORDING_MIDI &&
        m_transportStatus != RECORDING_AUDIO &&
        m_transportStatus != STARTING_TO_RECORD_MIDI &&
        m_transportStatus != STARTING_TO_RECORD_AUDIO)
    {
        m_transportStatus = STARTING_TO_PLAY;
    }

    // Set up latencies
    //
    m_playLatency = playLatency;
    m_readAhead = readAhead;
    if (m_readAhead == Rosegarden::RealTime::zeroTime)
        m_readAhead.sec = 1;

    // Ensure that we have time for audio synchronisation
    //
    /*
    if (m_audioPlayLatency > m_playLatency)
        m_playLatency = m_audioPlayLatency;
        */


    cleanupMmapData();

    // Map all segments
    //
    QDir segmentsDir(m_segmentFilesPath, "segment_*");
    for (unsigned int i = 0; i < segmentsDir.count(); ++i) {
        mmapSegment(m_segmentFilesPath + "/" + segmentsDir[i]);
    }

    // Map metronome
    //
    QString metronomeFileName = KGlobal::dirs()->resourceDirs("tmp").first() + "/rosegarden_metronome";
    QFileInfo metronomeFileInfo(metronomeFileName);
    if (metronomeFileInfo.exists())
        mmapSegment(metronomeFileName);
    else 
        SEQUENCER_DEBUG << "RosegardenSequencerApp::play() - no metronome found\n";

    // Map tempo segment
    //
    QString tempoSegmentFileName = KGlobal::dirs()->resourceDirs("tmp").first() + "/rosegarden_tempo";
    QFileInfo tempoSegmentFileInfo(tempoSegmentFileName);
    if (tempoSegmentFileInfo.exists())
        mmapSegment(tempoSegmentFileName);
    else 
        SEQUENCER_DEBUG << "RosegardenSequencerApp::play() - no tempo segment found\n";

    // Map time sig segment
    //
    QString timeSigSegmentFileName = KGlobal::dirs()->resourceDirs("tmp").first() + "/rosegarden_timesig";
    QFileInfo timeSigSegmentFileInfo(timeSigSegmentFileName);
    if (timeSigSegmentFileInfo.exists())
        mmapSegment(timeSigSegmentFileName);
    else 
        SEQUENCER_DEBUG << "RosegardenSequencerApp::play() - no time sig segment found\n";

    // Map control block
    //
    m_controlBlockMmapper = new ControlBlockMmapper(KGlobal::dirs()->resourceDirs("tmp").first() + "/rosegarden_control_block");

    initMetaIterator();

    // report
    //
    SEQUENCER_DEBUG << "RosegardenSequencerApp::play() - starting to play\n";

    // Test bits
//     m_metaIterator = new MmappedSegmentsMetaIterator(m_mmappedSegments);
//     Rosegarden::MappedComposition testCompo;
//     m_metaIterator->fillCompositionWithEventsUntil(&testCompo,
//                                                    Rosegarden::RealTime(2,0));

//     dumpFirstSegment();

    // keep it simple
    return true;
}

MmappedSegment* RosegardenSequencerApp::mmapSegment(const QString& file)
{
    MmappedSegment* m = 0;
    
    try {
        m = new MmappedSegment(file);
    } catch (Rosegarden::Exception e) {
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
    for(MmappedSegmentsMetaIterator::mmappedsegments::iterator i = 
            m_mmappedSegments.begin(); i != m_mmappedSegments.end(); ++i)
        delete i->second;

    m_mmappedSegments.clear();

    delete m_metaIterator;
    m_metaIterator = 0;

    delete m_controlBlockMmapper;
    m_controlBlockMmapper = 0;
}

void RosegardenSequencerApp::remapSegment(const QString& filename)
{
    if (m_transportStatus != PLAYING) return;

    SEQUENCER_DEBUG << "RosegardenSequencerApp::remapSegment(" << filename << ")\n";

    MmappedSegment* m = m_mmappedSegments[filename];
    if (m->remap() && m_metaIterator)
        m_metaIterator->resetIteratorForSegment(filename);
}

void RosegardenSequencerApp::addSegment(const QString& filename)
{
    if (m_transportStatus != PLAYING) return;

    SEQUENCER_DEBUG << "MmappedSegment::addSegment(" << filename << ")\n";

    MmappedSegment* m = mmapSegment(filename);

    if (m_metaIterator)
        m_metaIterator->addSegment(m);
}

void RosegardenSequencerApp::deleteSegment(const QString& filename)
{
    if (m_transportStatus != PLAYING) return;

    SEQUENCER_DEBUG << "MmappedSegment::deleteSegment(" << filename << ")\n";

    MmappedSegment* m = m_mmappedSegments[filename];

    if (m_metaIterator)
        m_metaIterator->deleteSegment(m);

    delete m;
}

void RosegardenSequencerApp::closeAllSegments()
{
    SEQUENCER_DEBUG << "MmappedSegment::closeAllSegments()\n";

    for(MmappedSegmentsMetaIterator::mmappedsegments::iterator 
            i = m_mmappedSegments.begin();
            i != m_mmappedSegments.end(); ++i) {
        if (m_metaIterator)
            m_metaIterator->deleteSegment(i->second);

        delete i->second;
    }

    m_mmappedSegments.clear();
    
}

void RosegardenSequencerApp::remapControlBlock()
{
    if (m_transportStatus != PLAYING) return;

    SEQUENCER_DEBUG << "MmappedSegment::remapControlBlock()\n";

    m_controlBlockMmapper->refresh();
}

// DCOP Wrapper for play(Rosegarden::RealTime,
//                       Rosegarden::RealTime,
//                       Rosegarden::RealTime)
//
//
int
RosegardenSequencerApp::play(long timeSec,
                             long timeUsec,
                             long playLatencySec,
                             long playLatencyUSec,
                             long readAheadSec,
                             long readAheadUSec)

{
    return play(Rosegarden::RealTime(timeSec, timeUsec),
                Rosegarden::RealTime(playLatencySec, playLatencyUSec),
                Rosegarden::RealTime(readAheadSec, readAheadUSec));
}



// Wrapper for record(Rosegarden::RealTime,
//                    Rosegarden::RealTime,
//                    Rosegarden::RealTime,
//                    recordMode);
//
//
int
RosegardenSequencerApp::record(long timeSec,
                               long timeUSec,
                               long playLatencySec,
                               long playLatencyUSec,
                               long readAheadSec,
                               long readAheadUSec,
                               int recordMode)

{
    return record(Rosegarden::RealTime(timeSec, timeUSec),
                  Rosegarden::RealTime(playLatencySec, playLatencyUSec),
                  Rosegarden::RealTime(readAheadSec, readAheadUSec),
                  recordMode);
}


void
RosegardenSequencerApp::setLoop(const Rosegarden::RealTime &loopStart,
                                const Rosegarden::RealTime &loopEnd)
{
    m_loopStart = loopStart;
    m_loopEnd = loopEnd;

    m_sequencer->setLoop(loopStart, loopEnd);
}


void
RosegardenSequencerApp::setLoop(long loopStartSec,
                                long loopStartUSec,
                                long loopEndSec,
                                long loopEndUSec)
{
    setLoop(Rosegarden::RealTime(loopStartSec, loopStartUSec),
            Rosegarden::RealTime(loopEndSec, loopEndUSec));
}


// Return the status of the sound systems (audio and MIDI)
//
unsigned int
RosegardenSequencerApp::getSoundDriverStatus()
{
    return m_sequencer->getSoundDriverStatus();
}


// Add an audio file to the sequencer
int
RosegardenSequencerApp::addAudioFile(const QString &fileName, int id)
{
    return((int)m_sequencer->addAudioFile(fileName.utf8().data(), id));
}

int
RosegardenSequencerApp::removeAudioFile(int id)
{
    return((int)m_sequencer->removeAudioFile(id));
}

void
RosegardenSequencerApp::clearAllAudioFiles()
{
    m_sequencer->clearAudioFiles();
}

void
RosegardenSequencerApp::setMappedInstrument(int type, unsigned char channel,
                                            unsigned int id)
{
    InstrumentId mID = (InstrumentId)id;
    Rosegarden::Instrument::InstrumentType mType = 
        (Rosegarden::Instrument::InstrumentType)type;
    Rosegarden::MidiByte mChannel = (Rosegarden::MidiByte)channel;

    m_sequencer->setMappedInstrument(
            new Rosegarden::MappedInstrument (mType, mChannel, mID));

}

// Process a MappedComposition sent from Sequencer with
// immediate effect
//
void
RosegardenSequencerApp::processSequencerSlice(Rosegarden::MappedComposition mC)
{
    // Use the "now" API
    //
    m_sequencer->processEventsOut(mC, Rosegarden::RealTime::zeroTime, true);
}

void
RosegardenSequencerApp::processMappedEvent(unsigned int id,
                                           int type,
                                           unsigned char pitch,
                                           unsigned char velocity,
                                           long absTimeSec,
                                           long absTimeUsec,
                                           long durationSec,
                                           long durationUsec,
                                           long audioStartMarkerSec,
                                           long audioStartMarkerUSec)
{
    MappedEvent *mE =
        new MappedEvent(
            (InstrumentId)id,
            (MappedEvent::MappedEventType)type,
            (Rosegarden::MidiByte)pitch,
            (Rosegarden::MidiByte)velocity,
            Rosegarden::RealTime(absTimeSec, absTimeUsec),
            Rosegarden::RealTime(durationSec, durationUsec),
            Rosegarden::RealTime(audioStartMarkerSec, audioStartMarkerUSec));

    Rosegarden::MappedComposition mC;

    SEQUENCER_DEBUG << "processMappedEvent() - sending out single event"
                    << endl;

    /*
    std::cout << "ID = " << mE->getInstrument() << std::endl;
    std::cout << "TYPE = " << mE->getType() << std::endl;
    std::cout << "D1 = " << (int)mE->getData1() << std::endl;
    std::cout << "D2 = " << (int)mE->getData2() << std::endl;
    */

    mC.insert(mE);

    m_sequencer->processEventsOut(mC, Rosegarden::RealTime::zeroTime, true);
}

void
RosegardenSequencerApp::processMappedEvent(MappedEvent mE)
{
    Rosegarden::MappedComposition mC;
    mC.insert(new MappedEvent(mE));
    m_sequencer->processEventsOut(mC, Rosegarden::RealTime::zeroTime, true);
}

// Get the MappedDevice (DCOP wrapped vector of MappedInstruments)
//
Rosegarden::MappedDevice
RosegardenSequencerApp::getMappedDevice(unsigned int id)
{
    return m_sequencer->getMappedDevice(id);
}

unsigned int
RosegardenSequencerApp::getDevices()
{
    return m_sequencer->getDevices();
}

int
RosegardenSequencerApp::canReconnect(int type)
{
    return m_sequencer->canReconnect(type);
}

unsigned int
RosegardenSequencerApp::addDevice(int type, unsigned int direction)
{
    return m_sequencer->addDevice(type, direction);
}

void
RosegardenSequencerApp::removeDevice(unsigned int deviceId)
{
    m_sequencer->removeDevice(deviceId);
}

unsigned int
RosegardenSequencerApp::getConnections(int type, unsigned int direction)
{
    return m_sequencer->getConnections(type, direction);
}

QString
RosegardenSequencerApp::getConnection(int type, unsigned int direction,
				      unsigned int connectionNo)
{
    return m_sequencer->getConnection(type, direction, connectionNo);
}

void
RosegardenSequencerApp::setConnection(unsigned int deviceId,
				      QString connection)
{
    m_sequencer->setConnection(deviceId, connection);
}

void
RosegardenSequencerApp::setPlausibleConnection(unsigned int deviceId,
					       QString connection)
{
    m_sequencer->setPlausibleConnection(deviceId, connection);
}

void
RosegardenSequencerApp::sequencerAlive()
{
    if (!kapp->dcopClient()->
        isApplicationRegistered(QCString(ROSEGARDEN_GUI_APP_NAME)))
    {
        SEQUENCER_DEBUG << "RosegardenSequencerApp::sequencerAlive() - "
                        << "waiting for GUI to register" << endl;
        return;
    }

    QByteArray data;

    if (!kapp->dcopClient()->send(ROSEGARDEN_GUI_APP_NAME,
                                  ROSEGARDEN_GUI_IFACE_NAME,
                                 "alive()",
                                  data))
    {
        SEQUENCER_DEBUG << "RosegardenSequencer::alive()"
                        << " - can't call RosegardenGUI client"
                        << endl;
    }

    SEQUENCER_DEBUG << "RosegardenSequencerApp::sequencerAlive() - "
                    << "trying to tell GUI that we're alive" << endl;
}

void
RosegardenSequencerApp::setAudioMonitoring(long value)
{
    bool bValue = (bool)value;
    std::vector<unsigned int> inputPorts;

    if (bValue &&
            m_sequencer->getRecordStatus() == Rosegarden::ASYNCHRONOUS_MIDI)
    {
        m_sequencer->record(Rosegarden::ASYNCHRONOUS_AUDIO);
        SEQUENCER_DEBUG << "RosegardenSequencerApp::setAudioMonitoring - "
                        << "monitoring audio input" << endl;
        return;
    }

    if (bValue == false &&
            m_sequencer->getRecordStatus() == Rosegarden::ASYNCHRONOUS_AUDIO)
    {
        SEQUENCER_DEBUG << "RosegardenSequencerApp::setAudioMonitoring - "
                        << "monitoring MIDI input" << endl;
        m_sequencer->record(Rosegarden::ASYNCHRONOUS_MIDI);
    }
    
}

void
RosegardenSequencerApp::setAudioMonitoringInstrument(unsigned int id)
{
    m_sequencer->setAudioMonitoringInstrument(id);
}


Rosegarden::MappedRealTime
RosegardenSequencerApp::getAudioPlayLatency()
{
    return Rosegarden::MappedRealTime(m_sequencer->getAudioPlayLateny());
}

Rosegarden::MappedRealTime
RosegardenSequencerApp::getAudioRecordLatency()
{
    return Rosegarden::MappedRealTime(m_sequencer->getAudioRecordLateny());
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

    // Create a plugin manager
    //
    Rosegarden::MappedAudioPluginManager *pM =
      dynamic_cast<Rosegarden::MappedAudioPluginManager*>(
        m_studio->createObject(
            Rosegarden::MappedObject::AudioPluginManager, true)); // read-only

    if (pM)
        SEQUENCER_DEBUG << "created plugin manager" << endl;

#ifdef HAVE_LADSPA 
    pM->getenvLADSPAPath();
#endif

    // This creates new MappedPlugin objects under the studio
    //
    pM->discoverPlugins(m_studio);
}


void
RosegardenSequencerApp::setMappedProperty(int id,
                                          const QString &property,
                                          float value)
{
    /*
    SEQUENCER_DEBUG << "setProperty: id = " << id
                    << " : property = \"" << property << "\"" << endl;
                    */

    Rosegarden::MappedObject *object = m_studio->getObject(id);

    if (object)
        object->setProperty(property, value);

    /*
    Rosegarden::MappedAudioFader *fader = 
        dynamic_cast<Rosegarden::MappedAudioFader*>(object);

    if (fader)
    {
        if (property == Rosegarden::MappedAudioFader::FaderLevel)
            fader->setLevel(value);
    }
    */
}

void
RosegardenSequencerApp::setMappedProperty(int id, const QString &property,
        Rosegarden::MappedObjectValueList value)
{
    SEQUENCER_DEBUG << "setProperty: id = " << id
                    << " : property list size = \"" << value.size()
                    << "\"" << endl;

    Rosegarden::MappedObject *object = m_studio->getObject(id);

    if (object)
        object->setProperty(property, value);
}


int
RosegardenSequencerApp::getMappedObjectId(int type)
{
    int value = -1;

    Rosegarden::MappedObject *object =
        m_studio->getObjectOfType(
                Rosegarden::MappedObject::MappedObjectType(type));

    if (object)
    {
        value = int(object->getId());
    }

    return value;
}


std::vector<QString>
RosegardenSequencerApp::getPropertyList(int id,
                                        const QString &property)
{
    std::vector<QString> list;

    Rosegarden::MappedObject *object =
        m_studio->getObject(id);

    if (object)
    {
        list = object->getPropertyList(property);
    }

    SEQUENCER_DEBUG << "getPropertyList - return " << list.size()
                    << " items" << endl;

    return list;
}

unsigned int
RosegardenSequencerApp::getSampleRate() const
{
    if (m_sequencer)
        return m_sequencer->getSampleRate();

    return 0;
}

// Creates an object of a type
//
int 
RosegardenSequencerApp::createMappedObject(int type)
{
    Rosegarden::MappedObject *object =
              m_studio->createObject(
                      Rosegarden::MappedObject::MappedObjectType(type), false);

    if (object)
    {
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
#ifdef HAVE_LADSPA
    Rosegarden::MappedLADSPAPlugin *plugin =
        dynamic_cast<Rosegarden::MappedLADSPAPlugin*>(m_studio->getObject(id));

    if (plugin)
    {
        std::cout << "RosegardenSequencerApp::destroyMappedObject - "
                  << "removing plugin instance" << std::endl;
        m_sequencer->removePluginInstance(plugin->getInstrument(),
                                          plugin->getPosition());
    }
#endif

    return(int(m_studio->destroyObject(Rosegarden::MappedObjectId(id))));
}


void
RosegardenSequencerApp::clearStudio()
{
    SEQUENCER_DEBUG << "clearStudio()" << endl;
    m_sequencer->removePluginInstances();
    m_studio->clearTemporaries();
} 

void
RosegardenSequencerApp::setMappedPort(int pluginId,
                                      unsigned long portId,
                                      float value)
{
    Rosegarden::MappedObject *object =
        m_studio->getObject(pluginId);

#ifdef HAVE_LADSPA

    Rosegarden::MappedLADSPAPlugin *plugin = 
        dynamic_cast<Rosegarden::MappedLADSPAPlugin*>(object);

    if (plugin)
    {
        plugin->setPort(portId, value);
    }


#endif // HAVE_LADSPA

}

void
RosegardenSequencerApp::slotCheckForNewClients()
{
    // Don't do this check if any of these conditions hold
    //
    if (m_transportStatus == PLAYING ||
        m_transportStatus == RECORDING_MIDI ||
        m_transportStatus == RECORDING_AUDIO)
        return;

    if (m_sequencer->checkForNewClients())
    {
        SEQUENCER_DEBUG << "client list changed" << endl;
    }
}



void
RosegardenSequencerApp::setSliceSize(long timeSec, long timeUSec)
{
    int msecs = (timeSec * 1000) + (timeUSec / 1000);
    SEQUENCER_DEBUG << "set slice size = " << msecs << "ms" << endl;

    Rosegarden::RealTime newReadAhead(timeSec, timeUSec);

    m_readAhead = newReadAhead;

    /*
    if (newReadAhead > m_readAhead)
    else // shrinking slice, we have to refetch sooner
    {
        // for the moment we just keep it simple
        m_readAhead = newReadAhead;
    }
    */
}

// Set the MIDI Clock period in microseconds
//
void
RosegardenSequencerApp::setQuarterNoteLength(long timeSec, long timeUSec)
{
    long usecs =
        long((double(timeSec) * 1000000.0 + double(timeUSec)) / 24.0);

    //SEQUENCER_DEBUG << "sending MIDI clock every " << usecs << " usecs" << endl;
    m_sequencer->setMIDIClockInterval(usecs);
}

QString
RosegardenSequencerApp::getStatusLog()
{
    return m_sequencer->getStatusLog();
}


void RosegardenSequencerApp::dumpFirstSegment()
{
    SEQUENCER_DEBUG << "Dumping 1st segment data :\n";

    unsigned int i = 0;
    MmappedSegment* firstMappedSegment = (*(m_mmappedSegments.begin())).second;

    MmappedSegment::iterator it(firstMappedSegment);

    for (; !it.atEnd(); ++it) {

        MappedEvent evt = (*it);
        SEQUENCER_DEBUG << i << " : inst = "  << evt.getInstrument()
                        << " - type = "       << evt.getType()
                        << " - data1 = "      << (unsigned int)evt.getData1()
                        << " - data2 = "      << (unsigned int)evt.getData2()
                        << " - time = "       << evt.getEventTime()
                        << " - duration = "   << evt.getDuration()
                        << " - audio mark = " << evt.getAudioStartMarker()
                        << endl;

        ++i;
    }

    SEQUENCER_DEBUG << "Dumping 1st segment data - done\n";

}


void RosegardenSequencerApp::rationalisePlayingAudio(const std::vector<int> &segmentAudio)
{
    std::vector<int> driverAudio = m_sequencer->getPlayingAudioFiles();

    //std::cout << "DRIVER FILES  = " << driverAudio.size() << std::endl;
    //std::cout << "SEGMENT FILES = " << segmentAudio.size() << std::endl << std::endl;

    // Check for playing audio that shouldn't be
    //
    for (std::vector<int>::const_iterator it = driverAudio.begin(); it != driverAudio.end(); ++it)
    {
        bool segment = false;
        for (std::vector<int>::const_iterator sIt = segmentAudio.begin();
             sIt != segmentAudio.end(); ++sIt)
        {
            if ((*it) == (*sIt))
            {
                segment = true;
                break;
            }
        }

        if (segment == false)
        {
            // We're found an audio segment that shouldn't be playing - stop it
            // through the normal channels.  Send a cancel event to the driver.
            //
            MappedEvent *mE = new MappedEvent();
            mE->setType(Rosegarden::MappedEvent::AudioCancel);
            mE->setRuntimeSegmentId(*it);
            processMappedEvent(*mE);
            delete mE;
        }
    }

    // Check for audio that should be that isn't
    //
    for (std::vector<int>::const_iterator sIt = segmentAudio.begin();
         sIt != segmentAudio.end(); ++sIt)
    {
        bool driver = false;
        for (std::vector<int>::const_iterator it = driverAudio.begin();
             it != driverAudio.end(); ++it)
        {
            if ((*it) == (*sIt))
            {
                driver = true;
                break;
            }
        }

        if (driver == false)
        {
            // There's an audio event that should be playing that isn't - start
            // it adjusting for current position.
            //
            MappedEvent *audioSegment = m_metaIterator->getAudioSegment(*sIt);

            if (audioSegment)
            {
                audioSegment->setAudioStartMarker
                    (audioSegment->getAudioStartMarker() + m_songPosition);

                // Reset duration firstly
                //
                audioSegment->
                    setDuration(audioSegment->getDuration() - 
                                (m_songPosition - audioSegment->getEventTime()));

                // Set start time to now
                //
                audioSegment->setEventTime(m_songPosition);
                processMappedEvent(*audioSegment);
                delete audioSegment;
            }
        }
    }
}


