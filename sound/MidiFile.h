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


#ifndef _ROSEGARDEN_MIDI_FILE_H_
#define _ROSEGARDEN_MIDI_FILE_H_

#include "Midi.h"
#include "MidiEvent.h"
#include "Composition.h"

#include <fstream>
#include <string>
#include <list>
#include <map>

namespace Rosegarden
{

  // Our internal MIDI structure is just a list of MidiEvents
  // vector or list?
  //
  typedef std::map<unsigned int, std::list<MidiEvent> > MidiComposition;
  typedef std::list<MidiEvent>::iterator MidiTrackIterator;

  class MidiFile
  {
  public:

    typedef enum
    {
         MIDI_SINGLE_TRACK_FILE          = 0x00,
         MIDI_SIMULTANEOUS_TRACK_FILE    = 0x01,
         MIDI_SEQUENTIAL_TRACK_FILE      = 0x02,
         MIDI_CONVERTED_TO_APPLICATION   = 0xFE,
         MIDI_FILE_NOT_LOADED            = 0xFF
    } MIDIFileFormatType;

    MidiFile();
    MidiFile (char *fn);
    ~MidiFile();

    MidiFile& operator=(const MidiFile& mF)
    {
      _filename = mF._filename;
      _timingDivision = mF._timingDivision;
      _numberOfTracks = mF._numberOfTracks;
      _format = mF._format;
      return *this;
    }

    bool open();
    //const MidiEvent& getMidiEvent() {;}

    //bool writeHeader();
    //bool writeEvent();

    const string& filename() { return _filename; }
    const int& timingDivision() { return _timingDivision; }
    const MIDIFileFormatType& format() { return _format; }
    const unsigned int& numberOfTracks() { return _numberOfTracks; }

    // Conversion in and out of Rosegarden
    //
    Rosegarden::Composition* convertToRosegarden();
    void convertToMidi(const Rosegarden::Composition &comp);

  private:
    string _filename;
    int _timingDivision;   // pulses per quarter note
    MIDIFileFormatType _format;
    unsigned int _numberOfTracks;

    // Internal counters
    //
    unsigned long _trackByteCount;
    bool _decrementCount;

    // Internal representation
    //
    MidiComposition _midiComposition;

    // Split the tasks up
    //
    bool parseHeader(const string& midiHeader);
    bool parseTrack(ifstream* midiFile, const unsigned int &trackNum);
    bool writeHeader();
    bool writeTrack();

    // Internal convenience functions
    //
    const unsigned int midiBytesToInt(const string& bytes);
    const unsigned long midiBytesToLong(const string& bytes);
    const unsigned int getNumberFromMidiBytes(ifstream* midiFile);
    const string getMidiBytes(ifstream* midiFile, const unsigned int &bytes);
    bool skipToNextTrack(ifstream *midiFile);

  };

}

#endif // _ROSEGARDEN_MIDI_FILE_H_
