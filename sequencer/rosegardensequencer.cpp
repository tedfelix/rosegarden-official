// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-
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
#include <dcopclient.h>
#include <iostream>
#include <qdatetime.h>

#include "rosegardensequencer.h"
#include "rosegardendcop.h"
#include "Sequencer.h"
#include "MappedInstrument.h"

using std::cerr;
using std::endl;
using std::cout;

// The default latency and read-ahead values are actually sent
// down from the GUI every time playback or recording starts
// so the local values are kind of meaningless.
//
//
RosegardenSequencerApp::RosegardenSequencerApp():
    DCOPObject("RosegardenSequencerIface"),
    m_sequencer(0),
    m_transportStatus(STOPPED),
    m_songPosition(0, 0),
    m_lastFetchSongPosition(0, 0),
    m_fetchLatency(0, 30000),      // default value
    m_playLatency(0, 50000),       // default value
    m_readAhead(0, 40000),         // default value
    m_audioPlayLatency(0, 0),
    m_audioRecordLatency(0, 0),
    m_loopStart(0, 0),
    m_loopEnd(0, 0),
    m_sendAlive(true),
    m_guiCount(0)       // how many GUIs have we known?
{
    // Without DCOP we are nothing
    QCString realAppId = kapp->dcopClient()->registerAs(kapp->name(), false);

    if (realAppId.isNull())
    {
        cerr << "RosegardenSequencer cannot register with DCOP server" << endl;
        close();
    }

    // creating this object also initialises the Rosegarden
    // aRTS interface for both playback and recording.  We
    // will fall over at this point if the sequencer can't
    // initialise.
    //
    m_sequencer = new Rosegarden::Sequencer();

    if (!m_sequencer)
    {
        cerr << "RosegardenSequencer object could not be allocated";
        close();
    }

    // set this here and now so we can accept async midi events
    //
    m_sequencer->record(Rosegarden::ASYNCHRONOUS_MIDI);

}

RosegardenSequencerApp::~RosegardenSequencerApp()
{
    delete m_sequencer;
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

    // report
    //
    std::cout << "RosegardenSequencerApp::play() - stopping" << endl;

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
    if (isLooping() == true && end > m_loopEnd)
    {

        Rosegarden::RealTime loopOverlap = end - m_loopEnd;
        Rosegarden::MappedComposition *endLoop, *beginLoop;

        endLoop = getSlice(start, m_loopEnd, firstFetch);
        beginLoop = getSlice(m_loopStart,
                             m_loopStart + loopOverlap, true);

        // move the start time of the begin section one loop width
        // into the future and ensure that we keep the clocks level
        // until this time has passed
        //
        beginLoop->moveStartTime(m_loopEnd - m_loopStart);

        (*endLoop) = (*endLoop) + (*beginLoop);
        delete beginLoop;
        return endLoop;
    }
    else
        return getSlice(start, end, firstFetch);
}


Rosegarden::MappedComposition*
RosegardenSequencerApp::getSlice(const Rosegarden::RealTime &start,
                                 const Rosegarden::RealTime &end,
                                 bool firstFetch)
{
    QByteArray data, replyData;
    QCString replyType;
    QDataStream arg(data, IO_WriteOnly);

    arg << start.sec;
    arg << start.usec;
    arg << end.sec;
    arg << end.usec;
    arg << (unsigned char)firstFetch;

    Rosegarden::MappedComposition *mC = new Rosegarden::MappedComposition();

    // Loop timing
    //
    //QTime t;
    //t.start();

    if (!kapp->dcopClient()->call(ROSEGARDEN_GUI_APP_NAME,
                                  ROSEGARDEN_GUI_IFACE_NAME,
                                  "getSequencerSlice(long int, long int, long int, long int, unsigned char)",
                                  data, replyType, replyData, true))
    {
        cerr << "RosegardenSequencer::getSlice()"
             << " - can't call RosegardenGUI client"
             << endl;

        // Stop the sequencer so we can see if we can try again later
        //
        m_transportStatus = STOPPING;
    }
    else
    {
        //cerr << "getSequencerSlice TIME = " << t.elapsed() << " ms " << endl;

        QDataStream reply(replyData, IO_ReadOnly);
        if (replyType == "Rosegarden::MappedComposition")
        {
            reply >> mC;
        }
        else
        {
            cerr <<
                 "RosegardenSequencer::getSlice() - unrecognised type returned"
                 << endl;
        }
    }

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
    m_sequencer->initialisePlayback(m_songPosition);

    // Send the first events (starting the clock)
    Rosegarden::MappedComposition *mC =
        fetchEvents(m_songPosition, m_songPosition + m_readAhead, true);

    // process whether we need to or not as this also processes
    // the audio queue for us
    //
    m_sequencer->processEventsOut(*mC, m_playLatency, false);

    return true;
}

// Keep playing our fetched events, only top up the queued events
// once we're within m_fetchLatency of the last fetch.  Make sure
// that we're fetching *past* the end of what we've already fetched
// and ensure that we don't duplicate events fetch unnecessarily
// by incrementing m_lastFetchSongPosition before we do anything.
//
//
bool
RosegardenSequencerApp::keepPlaying()
{
    if (m_songPosition > ( m_lastFetchSongPosition - m_fetchLatency))
    {

        // Check to make sure that we haven't got ahead of the GUI
        // and adjust as necessary (drop some "slices" if you like)
        //
        Rosegarden::RealTime sequencerTime = m_sequencer->getSequencerTime();
        Rosegarden::RealTime dropBoundary = m_lastFetchSongPosition
                                            + m_readAhead + m_readAhead;

        if (sequencerTime > dropBoundary &&  // we've overstepped boundary
            !isLooping() &&                  // we're not looping or recording
            m_transportStatus != RECORDING_MIDI &&
            m_transportStatus != RECORDING_AUDIO)
        {
            // Catch up
            m_lastFetchSongPosition = sequencerTime;

            // Comment on droppage
            //
            Rosegarden::RealTime gapTime = sequencerTime - dropBoundary;
            int gapLength = gapTime.sec * 1000000 + gapTime.usec;
            int sliceSize = m_readAhead.sec * 1000000 + m_readAhead.usec;
            int slices = (gapLength/sliceSize == 0) ? 1 : gapLength/sliceSize;

            std::cerr << "RosegardenSequencerApp::keepPlaying() - "
                      << "GUI COULDN'T SERVICE SLICE REQUEST(S)" << std::endl
                      << "                                        "
                      << "      -- DROPPED "
                      << slices
                      << " SLICE";
            if (slices > 1) std::cerr << "S";
            std::cerr <<"! --" << std::endl;

        }

        Rosegarden::MappedComposition *mC =
                        fetchEvents(m_lastFetchSongPosition,
                                    m_lastFetchSongPosition + m_readAhead,
                                    false);

        // Again, process whether we need to or not to keep
        // the Sequencer up-to-date with audio events
        //
        m_sequencer->processEventsOut(*mC, m_playLatency, false);
        delete mC;

        m_lastFetchSongPosition = m_lastFetchSongPosition + m_readAhead;
    }

    return true;
}

// Return current Sequencer time in GUI compatible terms
// remembering that our playback is delayed by m_playLatency
// ticks from our current m_songPosition.
//
void
RosegardenSequencerApp::updateClocks()
{
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
    
    if (!kapp->dcopClient()->send(ROSEGARDEN_GUI_APP_NAME,
                      ROSEGARDEN_GUI_IFACE_NAME,
                      "setPointerPosition(long int, long int)",
                      data))
    {
        cerr << "RosegardenSequencer::updateClocks()"
             << " - can't send to RosegardenGUI client"
             << endl;

        // Stop the sequencer so we can see if we can try again later
        //
        m_transportStatus = STOPPING;

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
        cerr << "RosegardenSequencer::notifySequencerStatus()"
             << " - can't send to RosegardenGUI client" << endl;

        // Stop the sequencer
        //
        m_transportStatus = STOPPING;
    }
}

// Sets the Sequencer object and this object to the new time 
// from where playback can continue.
//
//
void
RosegardenSequencerApp::jumpTo(long posSec, long posUsec)
{
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
        cerr << "RosegardenSequencer::processRecordedMidi() - " 
             <<   "can't call RosegardenGUI client" << endl;

        // Stop the sequencer
        //
        m_transportStatus = STOPPING;
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
    arg << m_sequencer->getLastRecordedAudioLevel();

    if (!kapp->dcopClient()->send(ROSEGARDEN_GUI_APP_NAME,
                                  ROSEGARDEN_GUI_IFACE_NAME,
                                 "processRecordedAudio(long int, long int, float)",
                                  data/*, replyType, replyData, true*/))
    {
        cerr << "RosegardenSequencer::processRecordedMidi() - " 
             <<   "can't call RosegardenGUI client" << endl;

        // Stop the sequencer
        //
        m_transportStatus = STOPPING;
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
        cerr << "RosegardenSequencer::processAsynchronousEvents() - " <<
                "can't call RosegardenGUI client" << endl;

        // Stop the sequencer so we can see if we can try again later
        //
        m_transportStatus = STOPPING;
    }

    // Process any pending events (Note Offs or Audio) as part of
    // same procedure.
    //
    m_sequencer->processPending(m_playLatency);
}



int
RosegardenSequencerApp::record(const Rosegarden::RealTime &time,
                               const Rosegarden::RealTime &playLatency,
                               const Rosegarden::RealTime &fetchLatency,
                               const Rosegarden::RealTime &readAhead,
                               int recordMode)
{
    TransportStatus localRecordMode = (TransportStatus) recordMode;

    if (localRecordMode == STARTING_TO_RECORD_MIDI)
    {
        std::cout << "RosegardenSequencerApp::record()"
                  << " - starting to record MIDI" << endl;

        // Get the Sequencer to prepare itself for recording
        //
        m_sequencer->record(Rosegarden::RECORD_MIDI);
    }
    else if (localRecordMode == STARTING_TO_RECORD_AUDIO)
    {
        std::cout << "RosegardenSequencerApp::record()"
                  << " - starting to record Audio" << endl;

        QByteArray data, replyData;
        QCString replyType;
        QDataStream arg(data, IO_WriteOnly);

        if (!kapp->dcopClient()->call(ROSEGARDEN_GUI_APP_NAME,
                                      ROSEGARDEN_GUI_IFACE_NAME,
                                      "createNewAudioFile()",
                                      data, replyType, replyData, true))
        {
            std::cerr << "RosegardenSequencer::record()"
                      << " - can't call RosegardenGUI client"
                      << std::endl;
        }

        QDataStream reply(replyData, IO_ReadOnly);
        QString audioFileName;
        if (replyType == "QString")
        {
            reply >> audioFileName;
        }
        else
        {
            std::cerr << "RosegardenSequencer::record() - "
                      << "unrecognised type returned" << std::endl;
        }

        // set recording filename
        m_sequencer->setRecordingFilename(std::string(audioFileName.data()));

        // set recording
        m_sequencer->record(Rosegarden::RECORD_AUDIO);
    }
    else
    {
        // unrecognised type - return a problem
        return 1;
    }

    // Now set the local transport status to the record mode
    //
    //
    m_transportStatus = localRecordMode;

    // Work out the record latency
    Rosegarden::RealTime recordLatency = playLatency;
    if (m_audioRecordLatency > recordLatency)
        recordLatency = m_audioRecordLatency;

    // Ensure that playback is initialised
    //
    m_sequencer->initialisePlayback(m_songPosition);

    return play(time, recordLatency, fetchLatency, readAhead);
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
                             const Rosegarden::RealTime &fetchLatency,
                             const Rosegarden::RealTime &readAhead)
{
    if (m_transportStatus == PLAYING || m_transportStatus == STARTING_TO_PLAY)
        return true;

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
    m_fetchLatency = fetchLatency;
    m_readAhead = readAhead;

    // Ensure that we have time for audio synchronisation
    //
    if (m_audioPlayLatency > m_playLatency)
        m_playLatency = m_audioPlayLatency;

    // report
    //
    std::cout << "RosegardenSequencerApp::play() - starting to play" << endl;

    // keep it simple
    return true;
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
                             long fetchLatencySec,
                             long fetchLatencyUSec,
                             long readAheadSec,
                             long readAheadUSec)

{
    return play(Rosegarden::RealTime(timeSec, timeUsec),
                Rosegarden::RealTime(playLatencySec, playLatencyUSec),
                Rosegarden::RealTime(fetchLatencySec, fetchLatencyUSec),
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
                               long fetchLatencySec,
                               long fetchLatencyUSec,
                               long readAheadSec,
                               long readAheadUSec,
                               int recordMode)

{
    return record(Rosegarden::RealTime(timeSec, timeUSec),
                  Rosegarden::RealTime(playLatencySec, playLatencyUSec),
                  Rosegarden::RealTime(fetchLatencySec, fetchLatencyUSec),
                  Rosegarden::RealTime(readAheadSec, readAheadUSec),
                  recordMode);
}


void
RosegardenSequencerApp::setLoop(const Rosegarden::RealTime &loopStart,
                                const Rosegarden::RealTime &loopEnd)
{
    m_loopStart = loopStart;
    m_loopEnd = loopEnd;
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
int
RosegardenSequencerApp::getSoundSystemStatus()
{
    return m_sequencer->getDriverStatus();
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
    Rosegarden::InstrumentId mID = (Rosegarden::InstrumentId)id;
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
    m_sequencer->processEventsOut(mC, Rosegarden::RealTime(0, 0), true);
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
    Rosegarden::MappedEvent *mE =
        new Rosegarden::MappedEvent(
            (Rosegarden::InstrumentId)id,
            (Rosegarden::MappedEvent::MappedEventType)type,
            (Rosegarden::MidiByte)pitch,
            (Rosegarden::MidiByte)velocity,
            Rosegarden::RealTime(absTimeSec, absTimeUsec),
            Rosegarden::RealTime(durationSec, durationUsec),
            Rosegarden::RealTime(audioStartMarkerSec, audioStartMarkerUSec));

    Rosegarden::MappedComposition mC;

    std::cout << "processMappedEvent() - sending out single event"
              << std::endl;

    /*
    std::cout << "ID = " << mE->getInstrument() << std::endl;
    std::cout << "TYPE = " << mE->getType() << std::endl;
    std::cout << "D1 = " << (int)mE->getData1() << std::endl;
    std::cout << "D2 = " << (int)mE->getData2() << std::endl;
    */

    mC.insert(mE);

    m_sequencer->processEventsOut(mC, Rosegarden::RealTime(0, 0), true);
}

void
RosegardenSequencerApp::processMappedEvent(Rosegarden::MappedEvent mE)
{
    Rosegarden::MappedComposition mC;
    mC.insert(new Rosegarden::MappedEvent(mE));
    m_sequencer->processEventsOut(mC, Rosegarden::RealTime(0, 0), true);
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


// The GUI lets us know it's alive - so all we ever respond with 
// is the same call back.  We lock out loops of this at the
// sequencer end using sendAlive() (m_sendAlive).
//
void
RosegardenSequencerApp::alive()
{
    std::cout << "RosegardenSequencerApp::alive() - "
              << "GUI (count = " << ++m_guiCount
              << ") is alive, instruments synced" << std::endl;

    // now turn off the automatic sendalive
    m_sendAlive = false;
}


void
RosegardenSequencerApp::sequencerAlive()
{
    if (!kapp->dcopClient()->
        isApplicationRegistered(QCString(ROSEGARDEN_GUI_APP_NAME)))
    {
        std::cout << "RosegardenSequencerApp::sequencerAlive() - "
                  << "waiting for GUI to register" << std::endl;
        return;
    }

    QByteArray data;

    if (!kapp->dcopClient()->send(ROSEGARDEN_GUI_APP_NAME,
                                  ROSEGARDEN_GUI_IFACE_NAME,
                                 "alive()",
                                  data))
    {
        std::cerr << "RosegardenSequencer::alive()"
                  << " - can't call RosegardenGUI client"
                  << std::endl;
    }

    std::cout << "RosegardenSequencerApp::sequencerAlive() - "
              << "trying to tell GUI that we're alive" << std::endl;
}

void
RosegardenSequencerApp::setAudioLatencies(long playTimeSec,
                                          long playTimeUsec,
                                          long recordTimeSec,
                                          long recordTimeUsec)
{
    m_audioPlayLatency = Rosegarden::RealTime(playTimeSec, playTimeUsec);
    m_audioRecordLatency = Rosegarden::RealTime(recordTimeSec, recordTimeUsec);

    std::cout << "RosegardenSequencerApp::setAudioLatencies - " 
              << "playback latency = " << m_audioPlayLatency << std::endl;

    std::cout << "RosegardenSequencerApp::setAudioLatencies - "
              << "record latency = " << m_audioRecordLatency << std::endl;
}



