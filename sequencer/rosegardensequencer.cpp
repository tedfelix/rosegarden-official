/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2001
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

#include "rosegardensequencer.h"
#include "rosegardendcop.h"
#include "Sequencer.h"

using std::cerr;
using std::endl;
using std::cout;

RosegardenSequencerApp::RosegardenSequencerApp():
    DCOPObject("RosegardenSequencerIface"),
    m_sequencer(0),
    m_transportStatus(STOPPED),
    m_songPosition(0),
    m_lastFetchSongPosition(0),
    m_fetchLatency(20),
    m_playLatency(100)
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


// We receive a starting time from the GUI which we use as the
// basis of our first fetch of events from the GUI core.  Assuming
// this works we set our internal state to PLAYING and go ahead
// and play the piece until we get a signal to stop.
// 
// DCOP wants us to use an int as a return type instead of a bool.
//
int
RosegardenSequencerApp::play(const Rosegarden::timeT &position,
                             const Rosegarden::timeT &latency)
{
  if (m_transportStatus == PLAYING || m_transportStatus == STARTING_TO_PLAY)
    return true;

  // To play from the given song position sets up the internal
  // play state to "STARTING_TO_PLAY" which is then caught in
  // the main event loop
  //
  m_songPosition = position;
  m_transportStatus = STARTING_TO_PLAY;
  m_playLatency = latency;

  // keep it simple
  return true;
}

// DCOP wants us to use an int as a return type instead of a bool
//
int
RosegardenSequencerApp::stop()
{
  // process pending NOTE OFFs and stop the Sequencer
  m_sequencer->stopPlayback();

  // set our state at this level to STOPPING (pending any
  // unfinished NOTES)
  m_transportStatus = STOPPING;

  // the Sequencer doesn't need to know these once
  // we've stopped
  m_songPosition = 0;
  m_lastFetchSongPosition = 0;

  return true;
}



// Get a slice of events from the GUI
//
Rosegarden::MappedComposition
RosegardenSequencerApp::fetchEvents(const Rosegarden::timeT &start,
                                    const Rosegarden::timeT &end)
{
  QByteArray data, replyData;
  QCString replyType;
  QDataStream arg(data, IO_WriteOnly);

  Rosegarden::MappedComposition mappedComp;

  arg << start;
  arg << end;

  if (!kapp->dcopClient()->call(ROSEGARDEN_GUI_APP_NAME,
                                ROSEGARDEN_GUI_IFACE_NAME,
                                "getSequencerSlice(int, int)",
                                data, replyType, replyData))
  {
    cerr <<
     "RosegardenSequencer::fetchEvents() - can't call RosegardenGUI client"
         << endl;
  }
  else
  {
    QDataStream reply(replyData, IO_ReadOnly);
    if (replyType == "Rosegarden::MappedComposition")
    {
      reply >> mappedComp;
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
// this session of playback.  We fetch up to m_playLatency timeT
// ticks ahead at first at then top up once we're within
// m_fetchLatency of the end of the last fetch.
//
bool
RosegardenSequencerApp::startPlaying()
{
  Rosegarden::MappedComposition mappedComp;

  // Fetch up to m_playLatency ahead
  //
  mappedComp = fetchEvents(m_songPosition, m_songPosition + m_playLatency);
  m_lastFetchSongPosition = m_songPosition + m_playLatency;

  // This will reset the Sequencer's internal clock
  // ready for new playback
  m_sequencer->initializePlayback(m_songPosition);

  // Send the first events (starting the clock)
  m_sequencer->processMidiOut(&mappedComp, m_playLatency);

  return true;
}

// Keep playing our fetched events, only top up the queued events
// once we're within m_fetchLatency of the last fetch.
//
bool
RosegardenSequencerApp::keepPlaying()
{

  if (m_songPosition > m_lastFetchSongPosition - m_fetchLatency )
  {
    Rosegarden::MappedComposition mappedComp;
    mappedComp = fetchEvents(m_lastFetchSongPosition,
                             m_lastFetchSongPosition + m_playLatency);
 
    m_lastFetchSongPosition = m_lastFetchSongPosition + m_playLatency;
    m_sequencer->processMidiOut(&mappedComp, m_playLatency);
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

  Rosegarden::timeT newPosition = m_sequencer->getSequencerTime();

  // Sequencer time is a subset of MIDI time so GUI song
  // position won't be updating every pass through a tight
  // loop.
  //
  if (newPosition != m_songPosition)
  {
    m_songPosition = newPosition;

    // Now use newPosition to work out if we need to move the
    // GUI pointer.
    //
    if (m_songPosition > m_sequencer->getStartPosition() + m_playLatency)
      newPosition -= m_playLatency;
    else
      newPosition = m_sequencer->getStartPosition();

    arg << newPosition;

    cout << "updateClocks() - m_songPosition = " << m_songPosition << endl;
    if (!kapp->dcopClient()->send(ROSEGARDEN_GUI_APP_NAME,
                                  ROSEGARDEN_GUI_IFACE_NAME,
                                  "setPointerPosition(int)",
                                  data)) //, replyType, replyData))
    {
      cerr <<
       "RosegardenSequencer::updateClocks() - can't send to RosegardenGUI client"
           << endl;
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
                                data)) //, replyType, replyData))
  {
    cerr <<
     "RosegardenSequencer::notifySequencerStatus() - can't send to RosegardenGUI client"
         << endl;
  }
}

