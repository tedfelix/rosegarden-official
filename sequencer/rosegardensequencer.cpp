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

using std::cerr;
using std::endl;
using std::cout;

static Rosegarden::MappedComposition *mappedComp = 0;

RosegardenSequencerApp::RosegardenSequencerApp():
    DCOPObject("RosegardenSequencerIface"),
    m_sequencer(0),
    m_transportStatus(STOPPED),
    m_songPosition(0, 0),
    m_lastFetchSongPosition(0, 0),
    m_fetchLatency(0, 100000),
    m_playLatency(0, 200000),
    m_readAhead(0, 80000)
{
    // Without DCOP we are nothing
    QCString realAppId = kapp->dcopClient()->registerAs(kapp->name(), false);

    if (realAppId.isNull())
    {
        cerr << "RosegardenSequencer cannot register with DCOP server" << endl;
        close();
    }

    // creating this object also initializes the Rosegarden
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

    if (mappedComp == 0)
        mappedComp = new Rosegarden::MappedComposition();

    // set this here and now so we can accept async midi events
    //
    m_sequencer->record(Rosegarden::Sequencer::RECORD_MIDI);
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
                                    const Rosegarden::RealTime &end)
{

    // Always return an empty fetch if we're stopping or stopped
    //
    if ( m_transportStatus == STOPPED || m_transportStatus == STOPPING )
    {
        mappedComp->clear();
        return mappedComp;
    }

    QByteArray data, replyData;
    QCString replyType;
    QDataStream arg(data, IO_WriteOnly);

    arg << start.sec;
    arg << start.usec;
    arg << end.sec;
    arg << end.usec;

    QTime t;
    t.start();

    if (!kapp->dcopClient()->call(ROSEGARDEN_GUI_APP_NAME,
                                  ROSEGARDEN_GUI_IFACE_NAME,
                    "getSequencerSlice(long int, long int, long int, long int)",
                                  data, replyType, replyData, true))
    {
        cerr <<
        "RosegardenSequencer::fetchEvents() - can't call RosegardenGUI client"
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
            mappedComp->clear();
            reply >> *mappedComp;
        }
        else
        {
            cerr << "RosegardenSequencer::fetchEvents() - unrecognised type returned"
             << endl;
        }
    }

    return mappedComp;
}


// The first fetch of events from the core/ and initialization for
// this session of playback.  We fetch up to m_playLatency microseconds/
// seconds ahead at first at then top up once we're within m_fetchLatency
// of the end of the last fetch.
//
bool
RosegardenSequencerApp::startPlaying()
{
    // Fetch up to m_playLatency ahead
    //
    m_lastFetchSongPosition = m_songPosition + m_readAhead;

    // This will reset the Sequencer's internal clock
    // ready for new playback
    m_sequencer->initializePlayback(m_songPosition);

    // Send the first events (starting the clock)
    m_sequencer->processMidiOut( *fetchEvents(m_songPosition,
                                              m_songPosition + m_readAhead),
                                 m_playLatency );

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
    if (m_songPosition > ( m_lastFetchSongPosition - m_fetchLatency ) )
    {
    
        // increment past last song fetch position by one microsecond
        //m_lastFetchSongPosition.usec++;

        m_sequencer->processMidiOut( *fetchEvents(m_lastFetchSongPosition,
                              m_lastFetchSongPosition + m_readAhead),
                         m_playLatency);
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

    // Using RealTime boundaries for the update check
    //
    Rosegarden::RealTime updateBound(0, 1000); // 1000 microseconds

    if (newPosition > ( m_songPosition + updateBound ) ||
        newPosition < ( m_songPosition - updateBound ) )
    {
        m_songPosition = newPosition;

        // Now use newPosition to work out if we need to move the
        // GUI pointer.
        //
        if (m_songPosition > m_sequencer->getStartPosition() + m_playLatency)
            newPosition = newPosition - m_playLatency;
        else
            newPosition = m_sequencer->getStartPosition();

        arg << newPosition.sec;
        arg << newPosition.usec;
    
        //std::cerr << "updateClocks() - m_songPosition = " << m_songPosition.sec << "s " << m_songPosition.usec << "us" << endl;

        if (!kapp->dcopClient()->send(ROSEGARDEN_GUI_APP_NAME,
                          ROSEGARDEN_GUI_IFACE_NAME,
                          "setPointerPosition(long int, long int)",
                          data))
        {
            cerr <<
            "RosegardenSequencer::updateClocks() - can't send to RosegardenGUI client"
             << endl;

            // Stop the sequencer so we can see if we can try again later
            //
            m_transportStatus = STOPPING;
    
        }
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
    cerr <<
        "RosegardenSequencer::notifySequencerStatus() - can't send to RosegardenGUI client"
         << endl;

    // Stop the sequencer so we can see if we can try again later
    //
    m_transportStatus = STOPPING;

    }
}

// Simple conglomeration of already exposed functions for
// the moment - this may get more complex later when we
// want this to be more efficient.
//
void
RosegardenSequencerApp::jumpTo(const long &posSec, const long &posUsec)
{
    if (posSec < 0 && posUsec < 0)
        return;

    stop();
    play(Rosegarden::RealTime(posSec, posUsec), m_playLatency, m_fetchLatency);
  
    return;
}

void
RosegardenSequencerApp::processRecordedMidi()
{
    QByteArray data, replyData;
    QCString replyType;
    QDataStream arg(data, IO_WriteOnly);

    arg << m_sequencer->getMappedComposition();

    if (!kapp->dcopClient()->call(ROSEGARDEN_GUI_APP_NAME,
                                  ROSEGARDEN_GUI_IFACE_NAME,
                                 "processRecordedMidi(Rosegarden::MappedComposition)",
                                  data, replyType, replyData, true))
    {
    cerr << "RosegardenSequencer::processRecordedMidi() - " <<
                "can't call RosegardenGUI client" << endl;

    // Stop the sequencer so we can see if we can try again later
    //
    m_transportStatus = STOPPING;
    }
}


void
RosegardenSequencerApp::processRecordedAudio()
{
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

/*
    if(m_sequencer->getMappedComposition().size() == 0)
        return;
*/

    arg << m_sequencer->getMappedComposition();

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

}



int
RosegardenSequencerApp::record(const Rosegarden::RealTime &time,
                               const Rosegarden::RealTime &playLatency,
                               const Rosegarden::RealTime &fetchLatency,
                               const int &recordMode)
{
    TransportStatus localRecordMode = (TransportStatus) recordMode;

    if (localRecordMode == STARTING_TO_RECORD_MIDI)
    {
        std::cout << "RosegardenSequencerApp::record() - starting to record MIDI" << endl;
    }
    else if (localRecordMode == STARTING_TO_RECORD_AUDIO)
    {
        std::cout << "RosegardenSequencerApp::record() - starting to record Audio" << endl;
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

    return play(time, playLatency, fetchLatency);
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
                             const Rosegarden::RealTime &fetchLatency)
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
RosegardenSequencerApp::play(const long &timeSec,
                             const long &timeUsec,
                             const long &playLatencySec,
                             const long &playLatencyUSec,
                             const long &fetchLatencySec,
                             const long &fetchLatencyUSec)

{
    return play(Rosegarden::RealTime(timeSec, timeUsec),
                Rosegarden::RealTime(playLatencySec, playLatencyUSec),
                Rosegarden::RealTime(fetchLatencySec, fetchLatencyUSec));
}



// Wrapper for record(Rosegarden::RealTime,
//                    Rosegarden::RealTime,
//                    Rosegarden::RealTime,
//                    recordMode);
//
//
int
RosegardenSequencerApp::record(const long &timeSec,
                               const long &timeUSec,
                               const long &playLatencySec,
                               const long &playLatencyUSec,
                               const long &fetchLatencySec,
                               const long &fetchLatencyUSec,
                               const int &recordMode)

{
    return record(Rosegarden::RealTime(timeSec, timeUSec),
                  Rosegarden::RealTime(playLatencySec, playLatencyUSec),
                  Rosegarden::RealTime(fetchLatencySec, fetchLatencyUSec),
                  recordMode);
}



