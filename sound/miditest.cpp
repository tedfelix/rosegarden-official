// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-
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

// TEST APPLICATION - this is only used for testing out
// bits and bobs of the sound system from time to time.
//
// [rwb]

#include <iostream>
#include <arts/artsmidi.h>
#include <arts/soundserver.h>
#include "MidiArts.h"

#include "MidiFile.h"
#include "Composition.h"
#include "Segment.h"
#include "Event.h"
#include "MidiRecord.h"
#include "Sequencer.h"


using std::vector;
using std::endl;
using std::cout;


int
main(int argc, char **argv)
{
    Rosegarden::Sequencer sequencer;

    // turn MIDI recording on
    //
    sequencer.record(Rosegarden::Sequencer::RECORD_MIDI);

    int i;
    int count;

    while(true)
    {
        // pause - to keep things in check
        for (i = 0; i < 10000000; i++);

        // the recording section
        switch(sequencer.recordStatus())
        {
            case Rosegarden::Sequencer::RECORD_MIDI:
                count = sequencer.getMappedComposition().size();
                if (count)
                    std::cout << "Got " << count << " MIDI events" << endl;
                break;

            case Rosegarden::Sequencer::ASYNCHRONOUS_MIDI:
                // send asynchronous MIDI events up to the GUI
                break;

            default:
                break;
        }
    }

}
