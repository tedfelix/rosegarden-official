// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-
/*
  Rosegarden-4
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

#include <iostream>

#include <klocale.h>

#include <dcopclient.h>
#include <qdatetime.h>
#include <qstring.h>

#include "rosedebug.h"
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
    m_sendAlive(true),
    m_guiCount(0),       // how many GUIs have we known?
    m_clearToSend(false),
    m_studio(new Rosegarden::MappedStudio())
{
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

}

RosegardenSequencerApp::~RosegardenSequencerApp()
{
    if (m_sequencer)
    {
        SEQUENCER_DEBUG << "RosegardenSequencer - shutting down" << endl;
        delete m_sequencer;
    }

    if (m_studio)
        delete m_studio;
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
    SEQUENCER_DEBUG << "RosegardenSequencerApp::play() - stopping" << endl;

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
    arg << long(firstFetch);

    Rosegarden::MappedComposition *mC = new Rosegarden::MappedComposition();

    // Loop timing
    //
    //QTime t;
    //t.start();

    if (!kapp->dcopClient()->call(ROSEGARDEN_GUI_APP_NAME,
                                  ROSEGARDEN_GUI_IFACE_NAME,
                                  "getSequencerSlice(long int, long int, long int, long int, long int)",
                                  data, replyType, replyData, true))
    {
        SEQUENCER_DEBUG << "RosegardenSequencer::getSlice()"
                        << " - can't call RosegardenGUI client" << endl;

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
            SEQUENCER_DEBUG << "RosegardenSequencer::getSlice() - "
                            << "unrecognised type returned" << endl;
        }
    }

    // We've completed a call - clear to send now for a while
    //
    m_clearToSend = true;

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
            unsigned int slices = 
                (gapLength/sliceSize == 0) ? 1 : gapLength/sliceSize;

            QString plural = "";

            if (slices > 1) plural = QString("S");

            SEQUENCER_DEBUG << "RosegardenSequencerApp::keepPlaying() - "
                            << "GUI COULDN'T SERVICE SLICE REQUEST(S)\n" 
                            << "                                        "
                            << "      -- DROPPED "
                            << slices
                            << " SLICE"
                            << plural
                            <<"! --\n";

            QByteArray data;
            QDataStream arg(data, IO_WriteOnly);
        
            arg << slices;
            if (!kapp->dcopClient()->send(ROSEGARDEN_GUI_APP_NAME,
                                          ROSEGARDEN_GUI_IFACE_NAME,
                                          "skippedSlices(unsigned int)",
                                          data)) 
            {
                SEQUENCER_DEBUG << "RosegardenSequencer::keepPlaying()"
                     << " - can't send to RosegardenGUI client" << endl;
            }
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
RosegardenSequencerApp::updateClocks(bool clearToSend)
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
    arg << long(clearToSend);
    
    if (!kapp->dcopClient()->send(ROSEGARDEN_GUI_APP_NAME,
                      ROSEGARDEN_GUI_IFACE_NAME,
                      "setPointerPosition(long int, long int, long int)",
                      data))
    {
        SEQUENCER_DEBUG << "RosegardenSequencer::updateClocks()"
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
        SEQUENCER_DEBUG << "RosegardenSequencer::notifySequencerStatus()"
                        << " - can't send to RosegardenGUI client"
                        << endl;

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
        SEQUENCER_DEBUG << "RosegardenSequencer::processRecordedMidi() - " 
                        <<   "can't call RosegardenGUI client" 
                        << endl;

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

    if (!kapp->dcopClient()->send(ROSEGARDEN_GUI_APP_NAME,
                                  ROSEGARDEN_GUI_IFACE_NAME,
                                 "processRecordedAudio(long int, long int)",
                                  data/*, replyType, replyData, true*/))
    {
        SEQUENCER_DEBUG << "RosegardenSequencer::processRecordedMidi() - " 
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
        SEQUENCER_DEBUG << "RosegardenSequencer::processAsynchronousEvents() - "
                        << "can't call RosegardenGUI client" << endl;

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
        SEQUENCER_DEBUG << "RosegardenSequencerApp::record()"
                        << " - starting to record MIDI" << endl;

        // Get the Sequencer to prepare itself for recording
        //
        m_sequencer->record(Rosegarden::RECORD_MIDI);
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

    /*

    // Work out the record latency
    Rosegarden::RealTime recordLatency = playLatency;
    if (m_audioRecordLatency > recordLatency)
        recordLatency = m_audioRecordLatency;
    */

    // Ensure that playback is initialised
    //
    m_sequencer->initialisePlayback(m_songPosition);

    return play(time, playLatency, fetchLatency, readAhead);
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
    /*
    if (m_audioPlayLatency > m_playLatency)
        m_playLatency = m_audioPlayLatency;
        */

    // report
    //
    SEQUENCER_DEBUG << "RosegardenSequencerApp::play() - starting to play"
                    << endl;

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

    SEQUENCER_DEBUG << "processMappedEvent() - sending out single event"
                    << endl;

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
    SEQUENCER_DEBUG << "RosegardenSequencerApp::alive() - "
                    << "GUI (count = " << ++m_guiCount
                    << ") is alive, instruments synced" << endl;

    // now turn off the automatic sendalive
    m_sendAlive = false;
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
    SEQUENCER_DEBUG << "setProperty: id = " << id
                    << " : property = \"" << property << "\"" << endl;

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

    return object->getId();
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
RosegardenSequencerApp::reinitialiseStudio()
{
    SEQUENCER_DEBUG << "reinitialiseStudio()" << endl;
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

bool
RosegardenSequencerApp::checkForNewClients()
{
    return m_sequencer->checkForNewClients();
}



