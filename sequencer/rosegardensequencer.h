
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

#ifndef _ROSEGARDEN_SEQUENCER_APP_H_
#define _ROSEGARDEN_SEQUENCER_APP_H_
 
// RosegardenSequencerApp is the sequencer application for Rosegarden.
// It owns a Rosegarden::Sequencer object which wraps the aRTS level
// funtionality.  At this level we deal with comms with the Rosegarden
// GUI application, the high level marshalling of data and main event
// loop of the sequencer.  [rwb]
//
//


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// include files for Qt
#include <qstrlist.h>

// include files for KDE 
#include <kapp.h>
#include <kmainwindow.h>
#include <kaccel.h>

#include "rosegardendcop.h"
#include "rosegardensequenceriface.h"
#include "MappedComposition.h"
#include "Sequencer.h"
#include "Event.h"

class KURL;
class KRecentFilesAction;

// forward declaration of the RosegardenGUI classes
class RosegardenGUIDoc;
class RosegardenGUIView;

class RosegardenSequencerApp : public KMainWindow,
                               virtual public RosegardenSequencerIface
{
  Q_OBJECT

public:
  RosegardenSequencerApp();
  ~RosegardenSequencerApp();

protected:

public slots:
  virtual void quit();

  // DCOP doesn't currently like to stream bools so we have to
  // use ints for the return types of these slots.  This is
  // the GUI level control interface for the Sequencer.
  //
  virtual int play(const Rosegarden::timeT &position,
                   const Rosegarden::timeT &playLatency,
                   const Rosegarden::timeT &fetchLatency);
  virtual int stop();

  // Any sudden moves
  virtual void jumpTo(const Rosegarden::timeT &position);

  void setStatus(const TransportStatus &status) { m_transportStatus = status; }
  TransportStatus getStatus() { return m_transportStatus; }
   
  // Process the first chunk of Sequencer events
  bool startPlaying();

  // Process all subsequent events
  bool keepPlaying();

  // Update internal clock and send GUI position pointer movement
  void updateClocks();

  // Sends status changes up to GUI
  void notifySequencerStatus();

private:
  Rosegarden::MappedComposition* fetchEvents(const Rosegarden::timeT &start,
                                            const Rosegarden::timeT &end);

  Rosegarden::Sequencer *m_sequencer;
  TransportStatus m_transportStatus;

  // Position pointer
  Rosegarden::timeT m_songPosition;
  Rosegarden::timeT m_lastFetchSongPosition;

  // Latency - m_fetchLatency - when we should fetch new events and
  //                            spool them onto aRTS
  //
  //         - m_playLatency  - how long we add to all events to make
  //                            sure they play in a synchonised manner
  //                            
  //
  // We can throttle these values internally at first and see how
  // we get on.
  //
  Rosegarden::timeT m_fetchLatency;
  Rosegarden::timeT m_playLatency;

};
 
#endif // _ROSEGARDEN_SEQUENCER_APP_H_
