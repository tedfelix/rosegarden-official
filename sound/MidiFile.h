// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-
/*
  Rosegarden-4
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


#ifndef _ROSEGARDEN_MIDI_FILE_H_
#define _ROSEGARDEN_MIDI_FILE_H_

#include <fstream>
#include <string>
#include <list>
#include <map>

#include <qobject.h>

#include "Midi.h"
#include "MidiEvent.h"
#include "Composition.h"
#include "SoundFile.h"

// Conversion class for Rosegarden::Composition to and
// from MIDI Files.  Despite the fact you can reuse this
// object it's probably safer just to create it for a
// single way conversion and then throw it away (MIDI
// to Composition conversion invalidates the internal
// MIDI model).
//
// Derived from SoundFile but still had some features
// in common with it which could theoretically be moved
// up into the base for use in other derived classes.
//
// [rwb]
//
//

namespace Rosegarden
{

// Our internal MIDI structure is just a list of MidiEvents.
// We use a list and not a set because we want the order of
// the events to be arbitrary until we explicitly sort them
// (necessary when converting Composition absolute times to
// MIDI delta times).
//
typedef std::map<unsigned int, std::vector<MidiEvent*> > MidiComposition;
typedef std::vector<MidiEvent*>::iterator MidiTrackIterator;

class Studio;

class MidiFile : public QObject, public SoundFile
{
    Q_OBJECT
public:

    typedef enum
    {
	MIDI_SINGLE_TRACK_FILE          = 0x00,
	MIDI_SIMULTANEOUS_TRACK_FILE    = 0x01,
	MIDI_SEQUENTIAL_TRACK_FILE      = 0x02,
	MIDI_CONVERTED_TO_APPLICATION   = 0xFE,
	MIDI_FILE_NOT_LOADED            = 0xFF
    } MIDIFileFormatType;

    MidiFile(Studio *studio);
    MidiFile (const std::string &fn, Studio *studio);
    ~MidiFile();

    // Declare our virtuals
    //
    virtual bool open();
    virtual bool write();
    virtual void close();

    int timingDivision() { return m_timingDivision; }
    MIDIFileFormatType format() { return m_format; }
    unsigned int numberOfTracks() { return m_numberOfTracks; }
    bool hasTimeChanges() { return m_containsTimeChanges; }

    // If a file open or save failed
    std::string getError() { return m_error; }

    // Conversion in and out of Rosegarden
    //
    Rosegarden::Composition* convertToRosegarden(Rosegarden::Composition *
						 preexistingComposition = 0,
						 bool append = false);
    void convertToMidi(Rosegarden::Composition &comp);

signals:
    void setProgress(int);
    void incrementProgress(int);
    
private:

    int                    m_timingDivision;   // pulses per quarter note
    MIDIFileFormatType     m_format;
    unsigned int           m_numberOfTracks;
    bool                   m_containsTimeChanges;

    // Internal counters
    //
    long                  m_trackByteCount;
    bool                  m_decrementCount;

    // Internal MidiComposition
    //
    MidiComposition       m_midiComposition;

    // Clear the m_midiComposition
    //
    void clearMidiComposition();

    // Split the tasks up with these top level private methods
    //
    bool parseHeader(const std::string& midiHeader);
    bool parseTrack(std::ifstream* midiFile, unsigned int &trackNum);
    bool writeHeader(std::ofstream* midiFile);
    bool writeTrack(std::ofstream* midiFile, unsigned int trackNum);

    bool consolidateNoteOffEvents(Rosegarden::TrackId track);

    // Internal convenience functions
    //
    int midiBytesToInt(const std::string& bytes);
    long midiBytesToLong(const std::string& bytes);
    long getNumberFromMidiBytes(std::ifstream* midiFile);
    std::string getMidiBytes(std::ifstream* midiFile,
                                   unsigned long bytes);
    bool skipToNextTrack(std::ifstream *midiFile);
    void intToMidiBytes(std::ofstream* midiFile, int number);
    void longToMidiBytes(std::ofstream* midiFile, unsigned long number);
    std::string longToVarBuffer(unsigned long number);

    // The pointer to the Studio for Instrument stuff
    //
    Studio   *m_studio;

    std::string m_error;
};

}

#endif // _ROSEGARDEN_MIDI_FILE_H_
