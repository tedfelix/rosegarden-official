// -*- c-basic-offset: 4 -*-
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
#include "Track.h"
#include "Event.h"
#include "MidiRecord.h"
#include "Sequencer.h"


using std::vector;
using std::endl;
using std::cout;


int
main(int argc, char **argv)
{
    // Create a Rosegarden MIDI file
    //
    //Rosegarden::MidiFile *midiFile = new Rosegarden::MidiFile("glazunov.mid");
    Rosegarden::MidiFile *midiFile = new Rosegarden::MidiFile("Kathzy.mid");
    //Rosegarden::MidiFile *midiFile = new Rosegarden::MidiFile("outfile.mid");

    // open the MIDI file
    midiFile->open();

    // Create a Rosegarden composition from the file
    Rosegarden::Composition *comp = midiFile->convertToRosegarden();

    Rosegarden::MidiFile *outMidiFile = new Rosegarden::MidiFile("outfile.mid");

    outMidiFile->convertToMidi(*comp);
    outMidiFile->write();


    /*
    // initialize MIDI and audio subsystems
    //
    Rosegarden::Sequencer sequencer;

    // set the tempo - fudge this for the moment
    //sequencer.tempo(comp->getTempo());
    sequencer.tempo(40);

    unsigned long long i;
    vector<Arts::MidiEvent>::iterator midiQueueIt;
    vector<Arts::MidiEvent> *midiQueue;
    Arts::TimeStamp midiTime;
    Arts::MidiEvent event;

    cout << "Number of Tracks in Composition = " << comp->getNbTracks() << endl;
    cout << "Number of Ticks per Bar = " << comp->getNbTicksPerBar() << endl;


    // allow for a pause while we connect the inputs and outputs
    // in the MidiManager
    for (i = 0; i < 2000000000; i++);

    // turn MIDI recording on
    //
    sequencer.record(Rosegarden::Sequencer::RECORD_MIDI);
    // turn on playing
    sequencer.play();

    while(true)
    {
    if (sequencer.isPlaying())
    {
    Arts::TimeStamp ts = sequencer.convertToTimeStamp(sequencer.songPosition());
    cout << "CLOCK @ " << sequencer.songPosition() << " TIME = " << ts.sec << " and " << ts.usec << endl;
    sequencer.processMidiOut(comp);
    }
    
    // pause - to keep things in check
    for (i = 0; i < 10000000; i++);

    // set the song position to the current time
    sequencer.updateSongPosition();

 
    // the recording section
    switch(sequencer.recordStatus())
    {
    case Rosegarden::Sequencer::RECORD_MIDI:
    midiQueue = sequencer.getMidiQueue();

    if (midiQueue->size() > 0)
    {
    for (midiQueueIt = midiQueue->begin();
    midiQueueIt != midiQueue->end();
    midiQueueIt++)
    {
    // want to send the event both ways - to the track and to the GUI
    // so we process the midi commands individually from the small clump
    // we've received back in the queue.  Dependent on performance
    // this may allow us to update our GUI and keep reading in the
    // events from a single thread.
    //
    sequencer.processMidiIn(midiQueueIt->command,
    sequencer.recordTime(midiQueueIt->time));
    }
    }
    break;

    case Rosegarden::Sequencer::ASYNCHRONOUS_MIDI:
    // send asynchronous MIDI events up to the GUI
    break;

    default:
    break;
    }

    //sequencer.incrementSongPosition(60000);
    }
    */

}
