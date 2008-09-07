// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-
/*
  Rosegarden
  A sequencer and musical notation editor.
  Copyright 2000-2008 the Rosegarden development team.

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

#include <QObject>

#include "Midi.h"
#include "MidiEvent.h"
#include "Composition.h"
#include "SoundFile.h"

// Conversion class for Composition to and
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
typedef std::vector<MidiEvent *> MidiTrack;
typedef std::map<unsigned int, MidiTrack> MidiComposition;

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

    typedef enum
    {
        CONVERT_REPLACE,
        CONVERT_AUGMENT,
        CONVERT_APPEND
    } ConversionType;

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

    /**
     * Convert a MIDI file to a Rosegarden composition.  Return true
     * for success.
     */
    bool convertToRosegarden(Composition &c, ConversionType type);

    /**
     * Convert a Rosegarden composition to MIDI format, storing the
     * result internally for later writing.
     */
    void convertToMidi(Composition &comp);

signals:
    void setValue(int);
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
    std::map<int, int>    m_trackChannelMap;

    // Clear the m_midiComposition
    //
    void clearMidiComposition();

    // Split the tasks up with these top level private methods
    //
    bool parseHeader(const std::string& midiHeader);
    bool parseTrack(std::ifstream* midiFile, unsigned int &trackNum);
    bool writeHeader(std::ofstream* midiFile);
    bool writeTrack(std::ofstream* midiFile, unsigned int trackNum);

    bool consolidateNoteOffEvents(TrackId track);

    // Internal convenience functions
    //
    int midiBytesToInt(const std::string& bytes);
    long midiBytesToLong(const std::string& bytes);
    long getNumberFromMidiBytes(std::ifstream* midiFile, int firstByte = -1);
    MidiByte getMidiByte(std::ifstream* midiFile);
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
