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



int
main(int argc, char **argv)
{
  // Create a Rosegarden MIDI file
  //
  Rosegarden::MidiFile *midiFile = new Rosegarden::MidiFile("glazunov.mid");

  // Create a Rosegarden composition
  Rosegarden::Composition comp;
  comp.addTrack();


  /////  OK, test the MIDI file opens and parses

  // open the MIDI file
  midiFile->open();

  // and return the Rosegarden Composition
  comp = midiFile->convertToRosegarden();


  ///// NOW test the arts recording interface - note that at
  ///// the moment this code appears to work but I haven't
  ///// actually mangaed to capture any notes yet.. (RB 6.01)

  Arts::Dispatcher dispatcher;
  Arts::MidiManager midiManager;
  Arts::MidiClient midiClient;
  Arts::MidiClient midiClientRecord;
  Arts::MidiPort midiPort;
  Arts::SoundServer soundServer;
  RosegardenMidiRecord midiRecorder;

  midiManager = Arts::Reference("global:Arts_MidiManager");
  if (midiManager.isNull())
  {
    cerr << "Can't get MidiManager" << endl;
    exit(1);
  }

  soundServer = Arts::Reference("global:Arts_SoundServer");
  if (soundServer.isNull())
  {
    cerr << "Can't start SoundServer" << endl;
    exit(1);
  }

  midiRecorder = Arts::DynamicCast(soundServer.createObject("RosegardenMidiRecord"));
  if (midiRecorder.isNull())
  {
    cerr << "Can't create MidiRecorder" << endl;
    exit(1);
  }

  midiClient = midiManager.addClient(Arts::mcdPlay,Arts::mctApplication,
                                     "Rosegarden (play)","Rosegarden");

  midiPort = midiClient.addOutputPort();
  if (midiPort.isNull())
  {
    cerr << "Can't create Midi Output Port" << endl;
  }

  if (midiClient.isNull())
  {
    cerr << "Can't create MidiClient" << endl;
  }

  midiClientRecord = midiManager.addClient(Arts::mcdRecord,Arts::mctApplication,
                                     "Rosegarden (record)","Rosegarden");

  if (midiClientRecord.isNull())
  {
    cerr << "Can't create MidiClient" << endl;
    exit(1);
  }

  // Create our recording midi port
  //
  midiClientRecord.addInputPort(midiRecorder);

  // MIDI THRU
  //
  midiRecorder.setMidiThru(midiPort);

  // Turn on recording
  //
  midiRecorder.record(true);


  // pause and wait for notes to queue
  //
  unsigned long i;
  vector<Arts::MidiEvent>::iterator midiQueueIt;
  vector<Arts::MidiEvent> *midiQueue;

  while(true)
  {
    // pause and collect events from MIDI
    //
    cout << "waiting..." << endl;
    for (i = 0; i < 100000000; i++);

    midiQueue = midiRecorder.getQueue();

    cout << "Events Read: " << midiQueue->size() << endl;


    for (midiQueueIt = midiQueue->begin();
         midiQueueIt != midiQueue->end();
         midiQueueIt++)
    {
      cout << "MIDI COMMAND" << endl;
      cout << midiQueueIt->time.usec << endl;
      cout << "Data1 = " << midiQueueIt->command.data1 << endl;
      cout << "Data2 = " << midiQueueIt->command.data2 << endl << endl;
    }

  }

}
