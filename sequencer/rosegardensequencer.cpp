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

#include <iostream>
#include "rosegardensequencer.h"
#include "Sequencer.h"


using std::cerr;
using std::endl;
using std::cout;

RosegardenSequencerApp::RosegardenSequencerApp():
    DCOPObject("RosegardenSequencerIface"),
    m_sequencer(0)
{
  m_sequencer = new Rosegarden::Sequencer();

  if (!m_sequencer)
  {
    cerr << "Rosegarden::Sequencer object could not be allocated";
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



  return true;
}

// DCOP wants us to use an int as a return type instead of a bool
//
int
RosegardenSequencerApp::stop()
{
  cout << "CALLED STOP" << endl;
  return true;
}
