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
    m_sequencer(0)
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
}


// We receive a starting time from the GUI which we use as the
// basis of our first fetch of events from the GUI core.  Assuming
// this works we set our internal state to PLAYING and go ahead
// and play the piece until we get a signal to stop.
// 
// DCOP wants us to use an int as a return type instead of a bool.
//
int
RosegardenSequencerApp::play(const Rosegarden::timeT &position)
{

  if (m_sequencer->isPlaying())
    return true;

  // play from the given song position - this sets
  // up the internal play state that is caught in
  // the main event loop
  //
  m_sequencer->play(position);

  return true;
}

// DCOP wants us to use an int as a return type instead of a bool
//
int
RosegardenSequencerApp::stop()
{
  cout << "MEEP" << endl;
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
    cerr << "RosegardenSequencer - can't call RosegardenGUI client" << endl;
  }
  else
  {
    QDataStream reply(replyData, IO_ReadOnly);
    if (replyType == "QString")
    {
      QString result;
      reply >> result;
    }
    else if (replyType = "Rosegarden::MappedComposition")
    {
      reply >> mappedComp;
    }
    else
    {
      cerr << "RosegardenSequencer::fetchEvents - unrecognised type returned"
           << endl;
    }
  }

  return mappedComp;
}



