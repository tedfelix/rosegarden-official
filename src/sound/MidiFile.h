/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */
/*
  Rosegarden
  A sequencer and musical notation editor.
  Copyright 2000-2015 the Rosegarden development team.

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of the
  License, or (at your option) any later version.  See the file
  COPYING included with this distribution for more information.
*/


#ifndef RG_MIDI_FILE_H
#define RG_MIDI_FILE_H

#include <QObject>

#include "Midi.h"
#include "MidiEvent.h"
#include "base/Composition.h"
#include "SoundFile.h"

#include <fstream>
#include <string>
#include <list>
#include <map>

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
    friend class MidiInserter;
    enum FileFormatType {
        MIDI_SINGLE_TRACK_FILE          = 0x00,
        MIDI_SIMULTANEOUS_TRACK_FILE    = 0x01,
        MIDI_SEQUENTIAL_TRACK_FILE      = 0x02,
        MIDI_CONVERTED_TO_APPLICATION   = 0xFE,
        MIDI_FILE_NOT_LOADED            = 0xFF
    };

    enum TimingFormat {
        MIDI_TIMING_PPQ_TIMEBASE,
        MIDI_TIMING_SMPTE
    };

    enum ConversionType {
        CONVERT_REPLACE,
        CONVERT_AUGMENT,  // ??? Unused
        CONVERT_APPEND    // ??? Unused
    };

    MidiFile(Studio *studio);
    MidiFile (const QString &fn, Studio *studio);
    ~MidiFile();

    // Declare our virtuals
    //
    virtual bool open();
    virtual bool write();
    virtual void close();

    TimingFormat timingFormat() { return m_timingFormat; }
    int timingDivision() { return m_timingDivision; } // only valid if timing format is PPQ timebase
    void smpteFrameSpecification(int &fps, int &subframes) { fps = m_fps; subframes = m_subframes; } // only valid if timing format is SMPTE
    FileFormatType fileFormat() { return m_format; }
    unsigned int numberOfTracks() { return m_numberOfTracks; }
    bool hasTimeChanges() { return m_containsTimeChanges; }

    // If a file open or save failed
    std::string getError() { return m_error; }

    /**
     * Convert a MIDI file to a Rosegarden composition.  Return true
     * for success.
     *
     * ??? The only caller of this,
     *     RosegardenMainWindow::createDocumentFromMIDIFile(), only calls
     *     with type = CONVERT_REPLACE.
     */
    bool convertToRosegarden(Composition &c, ConversionType type);

    /**
     * Convert a Rosegarden composition to MIDI format, storing the
     * result internally for later writing.
     */
    void convertToMidi(Composition &comp);

signals:
    void setValue(int);
    
private:

    TimingFormat           m_timingFormat;
    int                    m_timingDivision;   // pulses per quarter note
    int                    m_fps;
    int                    m_subframes;
    FileFormatType         m_format;
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

#endif // RG_MIDI_FILE_H
