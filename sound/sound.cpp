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
#include <arts/artsmidi.h>
#include <arts/soundserver.h>
#include "MidiArts.h"

#include "MidiFile.h"
#include "Composition.h"
#include "Track.h"
#include "Event.h"
#include "MidiRecord.h"
#include "Sequencer.h"



int
main(int argc, char **argv)
{
  // Create a Rosegarden MIDI file
  //
  Rosegarden::MidiFile *midiFile = new Rosegarden::MidiFile("glazunov.mid");

  // Create a Rosegarden composition
  Rosegarden::Composition comp;
  //comp.addTrack();


  /////  OK, test the MIDI file opens and parses

  // open the MIDI file
  midiFile->open();

  // and return the Rosegarden Composition
  comp = midiFile->convertToRosegarden();


  // Create and initialize MIDI
  //
  Rosegarden::Sequencer sequencer;


  // pause and wait for notes to queue
  //
  unsigned long i;
  vector<Arts::MidiEvent>::iterator midiQueueIt;
  vector<Arts::MidiEvent> *midiQueue;

  cout << "MIDI Recording ready" << endl;

  Arts::TimeStamp midiTime;
  Arts::MidiEvent event;
  
  int noteVal = 50;

  // record MIDI events
  //
  sequencer.record(Rosegarden::Sequencer::RECORD_MIDI);

  while(sequencer.isPlaying())
  {

    // pause
    for (i = 0; i < 100000000; i++);

    midiQueue = sequencer.getMidiQueue();

    if (midiQueue->size() > 0)
    {
      cout << "SONG POSITION = " << sequencer.songPositionSeconds() << endl;
      cout << endl;
      cout << midiQueue->size() << " MIDI events read" << endl;

      for (midiQueueIt = midiQueue->begin();
           midiQueueIt != midiQueue->end();
           midiQueueIt++)
      {
        cout << "MIDI COMMAND" << endl;
        cout << midiQueueIt->time.usec << endl;
        cout << "Data1 = " << (Rosegarden::MidiByte) midiQueueIt->command.data1 << endl;
        cout << "Data2 = " << (Rosegarden::MidiByte) midiQueueIt->command.data2 << endl << endl;
        cout << "Time = " << (long)(midiQueueIt->time.sec) << " : " << (long)(midiQueueIt->time.usec) << endl;
      }
      cout << endl;
   }

   //sequencer.incrementSongPosition(60000);

   //midiQueueIt = midiQueue->begin();
  

  }

}
