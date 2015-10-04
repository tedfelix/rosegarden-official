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


#ifndef RG_MIDIFILE_H
#define RG_MIDIFILE_H

#include "base/Composition.h"
#include "SoundFile.h"

#include <QObject>

#include <fstream>
#include <string>
#include <vector>
#include <map>

namespace Rosegarden
{


class MidiEvent;
class Studio;

/// Conversion class for Composition to and from MIDI Files.
/**
 * Despite the fact you can reuse this object it's probably safer just
 * to create it for a single way conversion and then throw it away (MIDI
 * to Composition conversion invalidates the internal MIDI model).
 *
 * Derived from SoundFile but still had some features in common with it
 * which could theoretically be moved up into the base for use in other
 * derived classes.
 */
class MidiFile : public QObject, public SoundFile
{
    Q_OBJECT
public:
    MidiFile(const QString &filename, Studio *studio);
    virtual ~MidiFile();

    // *** Standard MIDI File to Rosegarden

    // See RosegardenMainWindow::createDocumentFromMIDIFile().

    /// Call convertToRosegarden() after calling this.
    virtual bool open();

    enum ConversionType {
        CONVERT_REPLACE,
        //CONVERT_AUGMENT,
        CONVERT_APPEND  // ??? Unused
    };

    /**
     * Convert a MIDI file to a Rosegarden composition.  Return true
     * for success.
     *
     * Call this after calling open().
     *
     * ??? Why not combine open() and convertToRosegarden() into one?
     *     Sure it doesn't conform to SoundFile, but is SoundFile's
     *     interface really the right way to do this?  After all, the
     *     close() function is unused.
     *
     * ??? The only caller of this,
     *     RosegardenMainWindow::createDocumentFromMIDIFile(), only calls
     *     with type = CONVERT_REPLACE.
     */
    bool convertToRosegarden(Composition &c, ConversionType type);

    // *** Rosegarden to Standard MIDI File

    // See RosegardenMainWindow::exportMIDIFile().

    /**
     * Convert a Rosegarden composition to MIDI format, storing the
     * result internally for later writing.
     *
     * Call write() after this to write the file.
     *
     * ??? Why not combine convertToMidi() and write() into one?
     *     Sure it doesn't conform to SoundFile, but is SoundFile's
     *     interface really the right way to do this?  After all, the
     *     close() function is unused.
     */
    void convertToMidi(Composition &comp);

    /// Call this after convertToMidi().
    virtual bool write();

    // *** Miscellaneous

    /// Error message when open() or write() fail.
    std::string getError() const  { return m_error; }

    /// Unused.  Required by SoundFile.
    virtual void close()  { }

signals:
    /// Progress in percent.  Connect to ProgressDialog::setValue(int).
    void progress(int);

private:
    // convertToMidi() uses MidiInserter.
    // MidiInserter uses:
    // - m_timingDivision
    // - m_format
    // - m_numberOfTracks
    // - m_midiComposition
    // - clearMidiComposition()
    friend class MidiInserter;

    enum FileFormatType {
        MIDI_SINGLE_TRACK_FILE          = 0x00,
        MIDI_SIMULTANEOUS_TRACK_FILE    = 0x01,
        MIDI_SEQUENTIAL_TRACK_FILE      = 0x02,
        MIDI_CONVERTED_TO_APPLICATION   = 0xFE,
        MIDI_FILE_NOT_LOADED            = 0xFF
    };
    FileFormatType m_format;

    unsigned m_numberOfTracks;

    enum TimingFormat {
        MIDI_TIMING_PPQ_TIMEBASE,
        MIDI_TIMING_SMPTE
    };
    TimingFormat m_timingFormat;

    /// Pulses Per Quarter note.  (m_timingFormat == MIDI_TIMING_PPQ_TIMEBASE)
    int m_timingDivision;

    // For SMPTE.
    int m_fps;
    int m_subframes;

    /// Number of bytes left to read in the current track.
    long m_trackByteCount;
    /// Allow decrementing of m_trackByteCount while reading.
    /**
     * Set to true while processing a track.  false while processing other
     * parts of the file.
     */
    bool m_decrementCount;

    /**
     * Our internal MIDI structure is just a list of MidiEvents.
     * We use a list and not a set because we want the order of
     * the events to be arbitrary until we explicitly sort them
     * (necessary when converting Composition absolute times to
     * MIDI delta times).
     */
    typedef std::vector<MidiEvent *> MidiTrack;
    typedef std::map<unsigned int, MidiTrack> MidiComposition;

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

    // For Instrument stuff.
    Studio *m_studio;

    std::string m_error;

    // UNUSED
    // ??? Set but never used.
    bool m_containsTimeChanges;
};

}

#endif // RG_MIDIFILE_H
