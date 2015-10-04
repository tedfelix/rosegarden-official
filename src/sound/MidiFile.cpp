/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.
    See the AUTHORS file for more details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "MidiFile.h"

#include "Midi.h"
#include "MidiEvent.h"
#include "base/Segment.h"
#include "base/NotationTypes.h"
#include "base/BaseProperties.h"
#include "base/SegmentNotationHelper.h"
#include "base/SegmentPerformanceHelper.h"
#include "base/Track.h"
#include "base/Instrument.h"
#include "base/Studio.h"
#include "base/MidiTypes.h"
#include "base/Profiler.h"
#include "gui/application/RosegardenMainWindow.h"
#include "gui/seqmanager/SequenceManager.h"
#include "misc/Debug.h"
#include "misc/Strings.h"
#include "sound/MappedBufMetaIterator.h"
#include "sound/MidiInserter.h"
#include "sound/SortingInserter.h"

#include <QApplication>

#include <iostream>
#include <fstream>
#include <string>
#include <cstdio>
#include <algorithm>
#include <limits.h>
#include <sstream>

#define MIDI_DEBUG 1

namespace Rosegarden
{

using std::string;
using std::ifstream;
using std::stringstream;
using std::cerr;
using std::endl;
using std::ends;
using std::ios;

MidiFile::MidiFile(const QString &filename, Studio *studio) :
    SoundFile(filename),
    m_format(MIDI_FILE_NOT_LOADED),
    m_numberOfTracks(0),
    m_timingFormat(MIDI_TIMING_PPQ_TIMEBASE),
    m_timingDivision(0),
    m_fps(0),
    m_subframes(0),
    m_trackByteCount(0),
    m_decrementCount(false),
    m_studio(studio),
    m_containsTimeChanges(false)
{}

// Make sure we clear away the m_midiComposition
//
MidiFile::~MidiFile()
{
    clearMidiComposition();
}


// A couple of convenience functions.   Watch the byte conversions out
// of the STL strings.
//
//
long
MidiFile::midiBytesToLong(const string& bytes)
{
    if (bytes.length() != 4) {
#ifdef MIDI_DEBUG
        std::cerr << "WARNING: Wrong length for long data (" << bytes.length()
        << ", should be 4)" << endl;
#endif

        // TRANSLATOR: "long" is a C++ data type
        throw (Exception(qstrtostr(QObject::tr("Wrong length for long data in MIDI stream"))));
    }

    long longRet = ((long)(((MidiByte)bytes[0]) << 24)) |
                   ((long)(((MidiByte)bytes[1]) << 16)) |
                   ((long)(((MidiByte)bytes[2]) << 8)) |
                   ((long)((MidiByte)(bytes[3])));

    std::cerr << "midiBytesToLong(" << int((MidiByte)bytes[0]) << "," << int((MidiByte)bytes[1]) << "," << int((MidiByte)bytes[2]) << "," << int((MidiByte)bytes[3]) << ") -> " << longRet << std::endl;

    return longRet;
}

int
MidiFile::midiBytesToInt(const string& bytes)
{
    if (bytes.length() != 2) {
#ifdef MIDI_DEBUG
        std::cerr << "WARNING: Wrong length for int data (" << bytes.length()
        << ", should be 2)" << endl;
#endif

        // TRANSLATOR: "int" is a C++ data type
        throw (Exception(qstrtostr(QObject::tr("Wrong length for int data in MIDI stream"))));
    }

    int intRet = ((int)(((MidiByte)bytes[0]) << 8)) |
                 ((int)(((MidiByte)bytes[1])));
    return (intRet);
}



// Gets a single byte from the MIDI byte stream.  For each track
// section we can read only a specified number of bytes held in
// m_trackByteCount.
//
MidiByte
MidiFile::getMidiByte(ifstream* midiFile)
{
    static int bytesGot = 0; // purely for progress reporting purposes

    if (midiFile->eof()) {
        throw(Exception(qstrtostr(QObject::tr("End of MIDI file encountered while reading"))));
    }

    if (m_decrementCount && m_trackByteCount <= 0) {
        throw(Exception(qstrtostr(QObject::tr("Attempt to get more bytes than expected on Track"))));
    }

    char byte;
    if (midiFile->read(&byte, 1)) {

        --m_trackByteCount;

        // update a progress dialog if we have one
        //
        ++bytesGot;
        if (bytesGot % 2000 == 0) {

            emit progress((int)(double(midiFile->tellg()) /
                                   double(m_fileSize) * 20.0));
            qApp->processEvents( QEventLoop::AllEvents );  // note: was qApp->processEvents(50)
        }

        return (MidiByte)byte;
    }

    throw(Exception(qstrtostr(QObject::tr("Attempt to read past MIDI file end"))));
}


// Gets a specified number of bytes from the MIDI byte stream.  For
// each track section we can read only a specified number of bytes
// held in m_trackByteCount.
//
string
MidiFile::getMidiBytes(ifstream* midiFile, unsigned long numberOfBytes)
{
    string stringRet;
    char fileMidiByte;
    static int bytesGot = 0; // purely for progress reporting purposes

    if (midiFile->eof()) {
#ifdef MIDI_DEBUG
        std::cerr << "MIDI file EOF - got "
        << stringRet.length() << " bytes out of "
        << numberOfBytes << endl;
#endif

        throw(Exception(qstrtostr(QObject::tr("End of MIDI file encountered while reading"))));

    }

    if (m_decrementCount && (numberOfBytes > (unsigned long)m_trackByteCount)) {
#ifdef MIDI_DEBUG
        std::cerr << "Attempt to get more bytes than allowed on Track ("
        << numberOfBytes
        << " > "
        << m_trackByteCount << endl;
#endif

        //!!! Investigate -- I'm seeing this on new-notation-quantization
        // branch: load glazunov.rg, run Interpret on first segment, export
        // and attempt to import again

        throw(Exception(qstrtostr(QObject::tr("Attempt to get more bytes than expected on Track"))));
    }

    while (stringRet.length() < numberOfBytes &&
            midiFile->read(&fileMidiByte, 1)) {
        stringRet += fileMidiByte;
    }

    // if we've reached the end of file without fulfilling the
    // quota then panic as our parsing has performed incorrectly
    //
    if (stringRet.length() < numberOfBytes) {
        stringRet = "";
#ifdef MIDI_DEBUG

        cerr << "Attempt to read past file end - got "
        << stringRet.length() << " bytes out of "
        << numberOfBytes << endl;
#endif

        throw(Exception(qstrtostr(QObject::tr("Attempt to read past MIDI file end"))));

    }

    // decrement the byte count
    if (m_decrementCount)
        m_trackByteCount -= stringRet.length();

    // update a progress dialog if we have one
    //
    bytesGot += numberOfBytes;
    if (bytesGot % 2000 == 0) {
        emit progress((int)(double(midiFile->tellg()) /
                               double(m_fileSize) * 20.0));
        qApp->processEvents(QEventLoop::AllEvents);
    }

    return stringRet;
}


// Get a long number of variable length from the MIDI byte stream.
//
//
long
MidiFile::getNumberFromMidiBytes(ifstream* midiFile, int firstByte)
{
    long longRet = 0;
    MidiByte midiByte;

    if (firstByte >= 0) {
        midiByte = (MidiByte)firstByte;
    } else if (midiFile->eof()) {
        return longRet;
    } else {
        midiByte = getMidiByte(midiFile);
    }

    longRet = midiByte;
    if (midiByte & 0x80 ) {
        longRet &= 0x7F;
        do {
            midiByte = getMidiByte(midiFile);
            longRet = (longRet << 7) + (midiByte & 0x7F);
        } while (!midiFile->eof() && (midiByte & 0x80));
    }

    return longRet;
}



// Seeks to the next track in the midi file and sets the number
// of bytes to be read in the counter m_trackByteCount.
//
bool
MidiFile::skipToNextTrack(ifstream *midiFile)
{
    string buffer, buffer2;
    m_trackByteCount = -1;
    m_decrementCount = false;

    while (!midiFile->eof() && (m_decrementCount == false )) {
        buffer = getMidiBytes(midiFile, 4);

        if (buffer.compare(0, 4, MIDI_TRACK_HEADER) == 0) {
            m_trackByteCount = midiBytesToLong(getMidiBytes(midiFile, 4));
            m_decrementCount = true;
        }

    }

    if ( m_trackByteCount == -1 ) // we haven't found a track
        return (false);
    else
        return (true);
}


// Read in a MIDI file.  The parsing process throws string
// exceptions back up here if we run into trouble which we
// can then pass back out to whoever called us using a nice
// bool.
//
//
bool
MidiFile::open()
{
    bool retOK = true;
    m_error = "";

#ifdef MIDI_DEBUG

    std::cerr << "MidiFile::open() : fileName = " << m_fileName << endl;
#endif

    clearMidiComposition();
    // Open the file
    ifstream *midiFile = new ifstream(m_fileName.toLocal8Bit(), ios::in | ios::binary);

    try {
        if (*midiFile) {

            // Set file size so we can count it off
            //
            midiFile->seekg(0, std::ios::end);
            m_fileSize = midiFile->tellg();
            midiFile->seekg(0, std::ios::beg);

            // Parse the MIDI header first.  The first 14 bytes of the file.
            if (!parseHeader(getMidiBytes(midiFile, 14))) {
                m_format = MIDI_FILE_NOT_LOADED;
                m_error = "Not a MIDI file.";
                return (false);
            }

            m_containsTimeChanges = false;

            TrackId i = 0;

            for (unsigned int j = 0; j < m_numberOfTracks; ++j) {

//#ifdef MIDI_DEBUG
                std::cerr << "Parsing Track " << j << endl;
//#endif

                if (!skipToNextTrack(midiFile)) {
#ifdef MIDI_DEBUG
                    cerr << "Couldn't find Track " << j << endl;
#endif

                    m_error = "File corrupted or in non-standard format?";
                    m_format = MIDI_FILE_NOT_LOADED;
                    return (false);
                }

#ifdef MIDI_DEBUG
                std::cerr << "Track has " << m_trackByteCount << " bytes" << std::endl;
#endif

                // Run through the events taking them into our internal
                // representation.
                if (!parseTrack(midiFile, i)) {
//#ifdef MIDI_DEBUG
                    std::cerr << "Track " << j << " parsing failed" << endl;
//#endif

                    m_error = "File corrupted or in non-standard format?";
                    m_format = MIDI_FILE_NOT_LOADED;
                    return (false);
                }

                ++i; // j is the source track number, i the destination
            }

            m_numberOfTracks = i;
        } else {
            m_error = "File not found or not readable.";
            m_format = MIDI_FILE_NOT_LOADED;
            return (false);
        }

        // Close the file now
        midiFile->close();
    } catch (Exception e) {
#ifdef MIDI_DEBUG
        std::cerr << "MidiFile::open() - caught exception - "
        << e.getMessage() << endl;
#endif

        m_error = e.getMessage();
        retOK = false;
    }

    return (retOK);
}

// Parse and ensure the MIDI Header is legitimate
//
//
bool
MidiFile::parseHeader(const string &midiHeader)
{
    if (midiHeader.size() < 14) {
#ifdef MIDI_DEBUG
        std::cerr << "MidiFile::parseHeader() - file header undersized" << endl;
#endif

        return (false);
    }

    if (midiHeader.compare(0, 4, MIDI_FILE_HEADER) != 0) {
#ifdef MIDI_DEBUG
        std::cerr << "MidiFile::parseHeader()"
        << "- file header not found or malformed"
        << endl;
#endif
        return (false);
    }

    if (midiBytesToLong(midiHeader.substr(4, 4)) != 6L) {
#ifdef MIDI_DEBUG
        std::cerr << "MidiFile::parseHeader()"
        << " - header length incorrect"
        << endl;
#endif

        return (false);
    }

    m_format = (FileFormatType)midiBytesToInt(midiHeader.substr(8, 2));
    m_numberOfTracks = midiBytesToInt(midiHeader.substr(10, 2));
    m_timingDivision = midiBytesToInt(midiHeader.substr(12, 2));
    m_timingFormat = MIDI_TIMING_PPQ_TIMEBASE;

    if (m_format == MIDI_SEQUENTIAL_TRACK_FILE) {
#ifdef MIDI_DEBUG
        std::cerr << "MidiFile::parseHeader()"
                  << "- can't load sequential track file"
                  << endl;
#endif
        return (false);
    }

    if (m_timingDivision > 32767) {
#ifdef MIDI_DEBUG
        std::cerr << "MidiFile::parseHeader() - file uses SMPTE timing" << endl;
#endif
        m_timingFormat = MIDI_TIMING_SMPTE;
        m_fps = 256 - (m_timingDivision >> 8);
        m_subframes = (m_timingDivision & 0xff);
    }

    return true;
}


// Extract the contents from a MIDI file track and places it into
// our local map of MIDI events.
//
bool
MidiFile::parseTrack(ifstream* midiFile, TrackId &lastTrackNum)
{
    MidiByte midiByte, metaEventCode, data1, data2;
    MidiByte eventCode = 0x80;
    std::string metaMessage;
    unsigned int messageLength;
    unsigned long deltaTime;
    unsigned long accumulatedTime = 0;

    // The trackNum passed in to this method is the default track for
    // all events provided they're all on the same channel.  If we find
    // events on more than one channel, we increment trackNum and record
    // the mapping from channel to trackNum in this channelTrackMap.
    // We then return the new trackNum by reference so the calling
    // method knows we've got more tracks than expected.

    // This would be a vector<TrackId> but TrackId is unsigned
    // and we need -1 to indicate "not yet used"
    std::vector<int> channelTrackMap(16, -1);

    // This is used to store the last absolute time found on each track,
    // allowing us to modify delta-times correctly when separating events
    // out from one to multiple tracks
    //
    std::map<int, unsigned long> trackTimeMap;

    // Meta-events don't have a channel, so we place them in a fixed
    // track number instead
    TrackId metaTrack = lastTrackNum;

    // Remember the last non-meta status byte (-1 if we haven't seen one)
    int runningStatus = -1;

    bool firstTrack = true;

    std::cerr << "Parse track: last track number is " << lastTrackNum << std::endl;

    // Since no event and its associated delta time can fit in just one
    // byte, a single remaining byte in the track has to be padding.
    // This obscure and non-standard, but such files do exist; ordinarily
    // there should be no bytes in the track after the last event.
    while (!midiFile->eof() && ( m_trackByteCount > 1 ) ) {
        if (eventCode < 0x80) {
#ifdef MIDI_DEBUG
            cerr << "WARNING: Invalid event code " << eventCode
            << " in MIDI file" << endl;
#endif

            throw (Exception(qstrtostr(QObject::tr("Invalid event code found"))));
        }

        deltaTime = getNumberFromMidiBytes(midiFile);

#ifdef MIDI_DEBUG
        cerr << "read delta time " << deltaTime << endl;
#endif

        // Get a single byte
        midiByte = getMidiByte(midiFile);

        if (!(midiByte & MIDI_STATUS_BYTE_MASK)) {
            if (runningStatus < 0) {
                throw (Exception(qstrtostr(QObject::tr("Running status used for first event in track"))));
            }

            eventCode = (MidiByte)runningStatus;
            data1 = midiByte;

#ifdef MIDI_DEBUG
            std::cerr << "using running status (byte " << int(midiByte) << " found)" << std::endl;
#endif

        } else {
#ifdef MIDI_DEBUG
            std::cerr << "have new event code " << int(midiByte) << std::endl;
#endif

            eventCode = midiByte;
            data1 = getMidiByte(midiFile);
        }

        if (eventCode == MIDI_FILE_META_EVENT) // meta events
        {
            //            metaEventCode = getMidiByte(midiFile);
            metaEventCode = data1;
            messageLength = getNumberFromMidiBytes(midiFile);

#ifdef MIDI_DEBUG

            std::cerr << "Meta event of type " << int(metaEventCode) << " and " << messageLength << " bytes found" << std::endl;
#endif

            metaMessage = getMidiBytes(midiFile, messageLength);

            if (metaEventCode == MIDI_TIME_SIGNATURE ||
                    metaEventCode == MIDI_SET_TEMPO)
            {
                m_containsTimeChanges = true;
            }

            long gap = accumulatedTime - trackTimeMap[metaTrack];
            accumulatedTime += deltaTime;
            deltaTime += gap;
            trackTimeMap[metaTrack] = accumulatedTime;

            MidiEvent *e = new MidiEvent(deltaTime,
                                         MIDI_FILE_META_EVENT,
                                         metaEventCode,
                                         metaMessage);

            m_midiComposition[metaTrack].push_back(e);

        } else { // the rest

            runningStatus = eventCode;

            MidiEvent *midiEvent;

            int channel = (eventCode & MIDI_CHANNEL_NUM_MASK);
            if (channelTrackMap[channel] == -1) {
                if (!firstTrack) {
                    ++lastTrackNum;
                } else {
                    firstTrack = false;
                }
                std::cerr << "MidiFile: new channel map entry: channel " << channel << " -> track " << lastTrackNum << std::endl;
                channelTrackMap[channel] = lastTrackNum;
                m_trackChannelMap[lastTrackNum] = channel;
            }

            TrackId trackNum = channelTrackMap[channel];

            {
                static int prevTrackNum = -1, prevChannel = -1;
                if (prevTrackNum != (int) trackNum ||
                    prevChannel != (int) channel) {
                    std::cerr << "MidiFile: track number for channel " << channel << " is " << trackNum << std::endl;
                    prevTrackNum = trackNum;
                    prevChannel = channel;
                }
            }

            // accumulatedTime is abs time of last event on any track;
            // trackTimeMap[trackNum] is that of last event on this track

            long gap = accumulatedTime - trackTimeMap[trackNum];
            accumulatedTime += deltaTime;
            deltaTime += gap;
            trackTimeMap[trackNum] = accumulatedTime;

            switch (eventCode & MIDI_MESSAGE_TYPE_MASK) {
            case MIDI_NOTE_ON:
            case MIDI_NOTE_OFF:
            case MIDI_POLY_AFTERTOUCH:
            case MIDI_CTRL_CHANGE:
                data2 = getMidiByte(midiFile);

                // create and store our event
                midiEvent = new MidiEvent(deltaTime, eventCode, data1, data2);

#ifdef MIDI_DEBUG
                std::cerr << "MIDI event for channel " << channel + 1 << " (track "
                << trackNum << ")" << std::endl;
                midiEvent->print();
#endif


                m_midiComposition[trackNum].push_back(midiEvent);
                break;

            case MIDI_PITCH_BEND:
                data2 = getMidiByte(midiFile);

                // create and store our event
                midiEvent = new MidiEvent(deltaTime, eventCode, data1, data2);
                m_midiComposition[trackNum].push_back(midiEvent);
                break;

            case MIDI_PROG_CHANGE:
            case MIDI_CHNL_AFTERTOUCH:
                // create and store our event
                std::cerr << "Program change or channel aftertouch: time " << deltaTime << ", code " << (int)eventCode << ", data " << (int) data1  << " going to track " << trackNum << std::endl;
                midiEvent = new MidiEvent(deltaTime, eventCode, data1);
                m_midiComposition[trackNum].push_back(midiEvent);
                break;

            case MIDI_SYSTEM_EXCLUSIVE:
                messageLength = getNumberFromMidiBytes(midiFile, data1);

#ifdef MIDI_DEBUG

                std::cerr << "SysEx of " << messageLength << " bytes found" << std::endl;
#endif

                metaMessage = getMidiBytes(midiFile, messageLength);

                if (MidiByte(metaMessage[metaMessage.length() - 1]) !=
                        MIDI_END_OF_EXCLUSIVE) {
#ifdef MIDI_DEBUG
                    std::cerr << "MidiFile::parseTrack() - "
                    << "malformed or unsupported SysEx type"
                    << std::endl;
#endif

                    continue;
                }

                // chop off the EOX
                // length fixed by Pedro Lopez-Cabanillas (20030523)
                //
                metaMessage = metaMessage.substr(0, metaMessage.length() - 1);

                midiEvent = new MidiEvent(deltaTime,
                                          MIDI_SYSTEM_EXCLUSIVE,
                                          metaMessage);
                m_midiComposition[trackNum].push_back(midiEvent);
                break;

            case MIDI_END_OF_EXCLUSIVE:
#ifdef MIDI_DEBUG

                std::cerr << "MidiFile::parseTrack() - "
                << "Found a stray MIDI_END_OF_EXCLUSIVE" << std::endl;
#endif

                break;

            default:
#ifdef MIDI_DEBUG

                std::cerr << "MidiFile::parseTrack()"
                << " - Unsupported MIDI Event Code:  "
                << (int)eventCode << endl;
#endif

                break;
            }
        }
    }

    // If the track has a padding byte, read and discard it to make sure that
    // the stream is positioned at the beginning of the following
    // track (if there is one.)
    if (m_trackByteCount == 1) {
        midiFile->ignore();
    }

    return (true);
}

// borrowed from ALSA pcm_timer.c
//
static unsigned long gcd(unsigned long a, unsigned long b)
{
    unsigned long r;
    if (a < b) {
        r = a;
        a = b;
        b = r;
    }
    while ((r = a % b) != 0) {
        a = b;
        b = r;
    }
    return b;
}

// If we wanted to abstract the MidiFile class to make it more useful to
// other applications (and formats) we'd make this method and its twin
// pure virtual.
//
bool
MidiFile::convertToRosegarden(Composition &composition, ConversionType type)
{
    Profiler profiler("MidiFile::convertToRosegarden");

    MidiTrack::iterator midiEvent;
    Segment *rosegardenSegment;
    Segment *conductorSegment = 0;
    Event *rosegardenEvent;
    string trackName;

    // Time conversions
    //
    timeT rosegardenTime = 0;
    timeT rosegardenDuration = 0;
    timeT maxTime = 0;

    // To create rests
    //
    timeT endOfLastNote;

    // Event specific vars
    //
    int numerator = 4;
    int denominator = 4;
    timeT segmentTime;

    // keys
    int accidentals;
    bool isMinor;
    bool isSharp;

    if (type == CONVERT_REPLACE)
        composition.clear();

    timeT origin = 0;
    if (type == CONVERT_APPEND && composition.getDuration() > 0) {
        origin = composition.getBarEndForTime(composition.getDuration());
    }

    TrackId compTrack = 0;
    for (Composition::iterator ci = composition.begin();
            ci != composition.end(); ++ci) {
        if ((*ci)->getTrack() >= compTrack)
            compTrack = (*ci)->getTrack() + 1;
    }

    Track *track = 0;

    // precalculate the timing factor
    //
    // [cc] -- attempt to avoid floating-point rounding errors
    timeT crotchetTime = Note(Note::Crotchet).getDuration();
    int divisor = m_timingDivision ? m_timingDivision : 96;

    unsigned long multiplier = crotchetTime;
    int g = (int)gcd(crotchetTime, divisor);
    multiplier /= g;
    divisor /= g;

    timeT maxRawTime = LONG_MAX;
    if ((long)multiplier > (long)divisor)
        maxRawTime = (maxRawTime / multiplier) * divisor;

    //bool haveTimeSignatures = false;
    InstrumentId compInstrument = MidiInstrumentBase;

    // Clear down the assigned Instruments we already have
    //
    if (type == CONVERT_REPLACE) {
        m_studio->unassignAllInstruments();
    }

    std::vector<Segment *> addedSegments;

#ifdef MIDI_DEBUG
    std::cerr << "NUMBER OF TRACKS = " << m_numberOfTracks << endl;
    std::cerr << "MIDI COMP SIZE = " << m_midiComposition.size() << endl;
#endif

    if (m_timingFormat == MIDI_TIMING_SMPTE) {
        
        // If we have SMPTE timecode (i.e. seconds and frames, roughly
        // equivalent to RealTime timestamps) then we need to add any
        // tempo change events _first_ before we can do any conversion
        // from SMPTE to musical time, because this conversion depends
        // on tempo.  Also we need to add tempo changes in time order,
        // not track order, because their own timestamps depend on all
        // prior tempo changes.

        // In principle there's no harm in doing this for non-SMPTE
        // files as well, but there's no gain and it's probably not a
        // good idea to mess with file loading just for the sake of
        // slightly simpler code.  So, SMPTE only.

        std::map<int, tempoT> tempi;
        for (TrackId i = 0; i < m_numberOfTracks; ++i) {
            for (midiEvent = m_midiComposition[i].begin();
                 midiEvent != m_midiComposition[i].end();
                 ++midiEvent) {        
                if ((*midiEvent)->isMeta()) {
                    if ((*midiEvent)->getMetaEventCode() == MIDI_SET_TEMPO) {
                        MidiByte m0 = (*midiEvent)->getMetaMessage()[0];
                        MidiByte m1 = (*midiEvent)->getMetaMessage()[1];
                        MidiByte m2 = (*midiEvent)->getMetaMessage()[2];
                        long tempo = (((m0 << 8) + m1) << 8) + m2;
                        if (tempo != 0) {
                            double qpm = 60000000.0 / double(tempo);
                            tempoT rgt(Composition::getTempoForQpm(qpm));
                            tempi[(*midiEvent)->getTime()] = rgt;
                        }
                    }
                }
            }
        }
        for (std::map<int, tempoT>::const_iterator i = tempi.begin();
             i != tempi.end(); ++i) {
            timeT t = composition.getElapsedTimeForRealTime
                (RealTime::frame2RealTime(i->first, m_fps * m_subframes));
            composition.addTempoAtTime(t, i->second);
        }
    }

    for (TrackId i = 0; i < m_numberOfTracks; ++i) {

        segmentTime = 0;
        trackName = string("Imported MIDI");

        // progress - 20% total in file import itself and then 80%
        // split over these tracks
        emit progress(20 +
                      (int)((80.0 * double(i) / double(m_numberOfTracks))));
        qApp->processEvents(QEventLoop::AllEvents);

        // Convert the deltaTime to an absolute time since
        // the start of the segment.  The addTime method
        // returns the sum of the current Midi Event delta
        // time plus the argument.
        //
        for (midiEvent = m_midiComposition[i].begin();
             midiEvent != m_midiComposition[i].end();
             ++midiEvent) {
            segmentTime = (*midiEvent)->addTime(segmentTime);
        }

        // Consolidate NOTE ON and NOTE OFF events into a NOTE ON with
        // a duration.
        //
        consolidateNoteOffEvents(i);

        if (m_trackChannelMap.find(i) != m_trackChannelMap.end()) {
            compInstrument = MidiInstrumentBase + m_trackChannelMap[i];
        } else {
            compInstrument = MidiInstrumentBase;
        }

        rosegardenSegment = new Segment;
        rosegardenSegment->setTrack(compTrack);
        rosegardenSegment->setStartTime(0);

        track = new Track(compTrack,         // id
                          compInstrument,    // instrument
                          compTrack,         // position
                          trackName,         // name
                          false);           // muted

        std::cerr << "New Rosegarden track: id = " << compTrack << ", instrument = " << compInstrument << ", name = " << trackName << std::endl;

        // rest creation token needs to be reset here
        //
        endOfLastNote = 0;

        int msb = -1, lsb = -1; // for bank selects
        Instrument *instrument = 0;

        for (midiEvent = m_midiComposition[i].begin();
             midiEvent != m_midiComposition[i].end();
             ++midiEvent) {

            rosegardenEvent = 0;

            // [cc] -- avoid floating-point where possible

            timeT rawTime = (*midiEvent)->getTime();
            timeT rawDuration = (*midiEvent)->getDuration();

            if (m_timingFormat == MIDI_TIMING_PPQ_TIMEBASE) {

                if (rawTime < maxRawTime) {
                    rosegardenTime = origin +
                        timeT((rawTime * multiplier) / divisor);
                } else {
                    rosegardenTime = origin +
                        timeT((double(rawTime) * multiplier) / double(divisor) + 0.01);
                }

                rosegardenDuration =
                    timeT((rawDuration * multiplier) / divisor);

            } else {

                // SMPTE timestamps are a count of the number of
                // subframes, where the number of subframes per frame
                // and frames per second have been defined in the file
                // header (stored as m_subframes, m_fps).  We need to
                // go through a realtime -> musical time conversion
                // for these, having added our tempo changes earlier
                
                rosegardenTime = composition.getElapsedTimeForRealTime
                    (RealTime::frame2RealTime(rawTime,
                                              m_fps * m_subframes));

                rosegardenDuration = composition.getElapsedTimeForRealTime
                    (RealTime::frame2RealTime(rawTime + rawDuration,
                                              m_fps * m_subframes))
                    - rosegardenTime;
            }

#ifdef MIDI_DEBUG
            std::cerr << "MIDI file import: origin " << origin
                      << ", event time " << rosegardenTime
                      << ", duration " << rosegardenDuration
                      << ", event type " << (int)(*midiEvent)->getMessageType()
                      << ", previous max time " << maxTime
                      << ", potential max time " << (rosegardenTime + rosegardenDuration)
                      << ", ev raw time " << (*midiEvent)->getTime()
                      << ", ev raw duration " << (*midiEvent)->getDuration()
                      << ", crotchet " << crotchetTime
                      << ", multiplier " << multiplier
                      << ", divisor " << divisor
                      << ", sfps " << m_fps * m_subframes
                      << std::endl;
#endif

            if (rosegardenTime + rosegardenDuration > maxTime) {
                maxTime = rosegardenTime + rosegardenDuration;
            }

            if (rosegardenSegment->empty()) {
                endOfLastNote = composition.getBarStartForTime(rosegardenTime);
            }

            if ((*midiEvent)->isMeta()) {

                switch ((*midiEvent)->getMetaEventCode()) {

                case MIDI_TEXT_EVENT: {
                        std::string text = (*midiEvent)->getMetaMessage();
                        rosegardenEvent =
                            Text(text).getAsEvent(rosegardenTime);
                    }
                    break;

                case MIDI_LYRIC: {
                        std::string text = (*midiEvent)->getMetaMessage();
                        rosegardenEvent =
                            Text(text, Text::Lyric).
                            getAsEvent(rosegardenTime);
                    }
                    break;

                case MIDI_TEXT_MARKER: {
                        std::string text = (*midiEvent)->getMetaMessage();
                        composition.addMarker(new Marker
                                              (rosegardenTime, text, ""));
                    }
                    break;

                case MIDI_COPYRIGHT_NOTICE:
                    if (type == CONVERT_REPLACE) {
                        composition.setCopyrightNote((*midiEvent)->
                                                     getMetaMessage());
                    }
                    break;

                case MIDI_TRACK_NAME:
                    track->setLabel((*midiEvent)->getMetaMessage());
                    break;

                case MIDI_INSTRUMENT_NAME:
                    rosegardenSegment->setLabel((*midiEvent)->getMetaMessage());
                    break;

                case MIDI_END_OF_TRACK: {
                    timeT trackEndTime = rosegardenTime;
                    if (trackEndTime <= 0) {
                        trackEndTime = crotchetTime * 4 * numerator / denominator;
                    }
                    if (endOfLastNote < trackEndTime) {
                        // If there's nothing in the segment yet, then we
                        // shouldn't fill with rests because we don't want
                        // to cause the otherwise empty segment to be created
                        if (rosegardenSegment->size() > 0) {
                            rosegardenSegment->fillWithRests(trackEndTime);
                        }
                    }
                }
                    break;

                case MIDI_SET_TEMPO:
                    if (m_timingFormat == MIDI_TIMING_PPQ_TIMEBASE) {
                        // (if we have smpte, we have already done this)

                        MidiByte m0 = (*midiEvent)->getMetaMessage()[0];
                        MidiByte m1 = (*midiEvent)->getMetaMessage()[1];
                        MidiByte m2 = (*midiEvent)->getMetaMessage()[2];

                        long tempo = (((m0 << 8) + m1) << 8) + m2;

                        if (tempo != 0) {
                            double qpm = 60000000.0 / double(tempo);
                            tempoT rgt(Composition::getTempoForQpm(qpm));
//                            std::cout << "MidiFile: converted MIDI tempo " << tempo << " to Rosegarden tempo " << rgt << std::endl;
                            composition.addTempoAtTime(rosegardenTime, rgt);
                        }
                    }
                    break;

                case MIDI_TIME_SIGNATURE:
                    numerator = (int) (*midiEvent)->getMetaMessage()[0];
                    denominator = 1 << ((int)(*midiEvent)->getMetaMessage()[1]);

                    // NB. a MIDI time signature also has
                    // metamessage[2] and [3], containing some timing data

                    if (numerator == 0) numerator = 4;
                    if (denominator == 0) denominator = 4;

                    composition.addTimeSignature
                        (rosegardenTime,
                         TimeSignature(numerator, denominator));
                    //haveTimeSignatures = true;
                    break;

                case MIDI_KEY_SIGNATURE:
                    // get the details
                    accidentals = (int) (*midiEvent)->getMetaMessage()[0];
                    isMinor = (int) (*midiEvent)->getMetaMessage()[1];
                    isSharp = accidentals < 0 ? false : true;
                    accidentals = accidentals < 0 ? -accidentals : accidentals;
                    // create the key event
                    //
                    try {
                        rosegardenEvent = Rosegarden::Key
                                          (accidentals, isSharp, isMinor).
                                          getAsEvent(rosegardenTime);
                    }
                    catch (...) {
#ifdef MIDI_DEBUG
                        std::cerr << "MidiFile::convertToRosegarden - "
                        << " badly formed key signature"
                        << std::endl;
#endif
                        break;
                    }
                    break;

                case MIDI_SEQUENCE_NUMBER:
                case MIDI_CHANNEL_PREFIX_OR_PORT:
                case MIDI_CUE_POINT:
                case MIDI_CHANNEL_PREFIX:
                case MIDI_SEQUENCER_SPECIFIC:
                case MIDI_SMPTE_OFFSET:
                default:
#ifdef MIDI_DEBUG
                    std::cerr << "MidiFile::convertToRosegarden - "
                              << "unsupported META event code "
                              << (int)((*midiEvent)->getMetaEventCode()) << endl;
#endif
                    break;
                }

            } else
                switch ((*midiEvent)->getMessageType()) {
                case MIDI_NOTE_ON:

                    // A zero velocity here is a virtual "NOTE OFF"
                    // so we ignore this event
                    //
                    if ((*midiEvent)->getVelocity() == 0) break;

                    endOfLastNote = rosegardenTime + rosegardenDuration;

#ifdef MIDI_DEBUG
                    std::cerr << "MidiFile::convertToRosegarden: note at " << rosegardenTime << ", duration " << rosegardenDuration << ", midi time " << (*midiEvent)->getTime() << " and duration " << (*midiEvent)->getDuration() << std::endl;
#endif

                    // create and populate event
                    rosegardenEvent = new Event(Note::EventType,
                                                rosegardenTime,
                                                rosegardenDuration);
                    rosegardenEvent->set<Int>(BaseProperties::PITCH,
                                              (*midiEvent)->getPitch());
                    rosegardenEvent->set<Int>(BaseProperties::VELOCITY,
                                              (*midiEvent)->getVelocity());
                    break;

                    // We ignore any NOTE OFFs here as we've already
                    // converted NOTE ONs to have duration
                    //
                case MIDI_NOTE_OFF:
                    continue;
                    break;

                case MIDI_PROG_CHANGE:
                    // Attempt to turn the prog change we've found into an
                    // Instrument.  Send the program number and whether or
                    // not we're on the percussion channel.
                    //
                    // Note that we make no attempt to do the right
                    // thing with program changes during a track -- we
                    // just save them as events.  Only the first is
                    // used to select the instrument.  If it's at time
                    // zero, it's not saved as an event.
                    //
//                    std::cerr << "Program change found" << std::endl;

                    if (!instrument) {

                        bool percussion = ((*midiEvent)->getChannelNumber() ==
                                           MIDI_PERCUSSION_CHANNEL);
                        int program = (*midiEvent)->getData1();

                        if (type == CONVERT_REPLACE) {

                            // ??? Setting up the instrument just because we
                            //     received a Program Change seems rather
                            //     arbitrary.  See Bug #1439.  What really
                            //     should be done is a two-pass approach where
                            //     we import everything as-is, then do a second
                            //     pass checking to see if there are PC's,
                            //     BS's, and CC's at time 0 that can be
                            //     removed and converted to Instrument
                            //     settings.
                            instrument = m_studio->getInstrumentById(compInstrument);
                            if (instrument) {
                                instrument->setPercussion(percussion);
                                instrument->setSendProgramChange(true);
                                instrument->setProgramChange(program);
                                instrument->setSendBankSelect(msb >= 0 || lsb >= 0);
                                if (instrument->sendsBankSelect()) {
                                    instrument->setMSB(msb >= 0 ? msb : 0);
                                    instrument->setLSB(lsb >= 0 ? lsb : 0);
                                }
                            }
                        } else { // not CONVERT_REPLACE
                            // ??? type is always CONVERT_REPLACE.  This code
                            //     is never executed.
                            instrument =
                                m_studio->assignMidiProgramToInstrument
                                (program, msb, lsb, percussion);
                        }
                    }

                    // assign it here
                    if (instrument) {
                        track->setInstrument(instrument->getId());
                        // We used to set the segment name from the instrument
                        // here, but now we do them all at the end only if the
                        // segment has no other name set (e.g. from instrument
                        // meta event)
                        if ((*midiEvent)->getTime() == 0) break; // no insert
                    }

                    // did we have a bank select? if so, insert that too

                    if (msb >= 0) {
                        rosegardenSegment->insert
                        (Controller(MIDI_CONTROLLER_BANK_MSB, msb).
                         getAsEvent(rosegardenTime));
                    }
                    if (lsb >= 0) {
                        rosegardenSegment->insert
                        (Controller(MIDI_CONTROLLER_BANK_LSB, lsb).
                         getAsEvent(rosegardenTime));
                    }

                    rosegardenEvent =
                        ProgramChange((*midiEvent)->getData1()).
                        getAsEvent(rosegardenTime);
                    break;

                case MIDI_CTRL_CHANGE:
                    // If it's a bank select, interpret it (or remember
                    // for later insertion) instead of just inserting it
                    // as a Rosegarden event

                    if ((*midiEvent)->getData1() == MIDI_CONTROLLER_BANK_MSB) {
                        msb = (*midiEvent)->getData2();
                        break;
                    }

                    if ((*midiEvent)->getData1() == MIDI_CONTROLLER_BANK_LSB) {
                        lsb = (*midiEvent)->getData2();
                        break;
                    }

                    // If it's something we can use as an instrument
                    // parameter, and it's at time zero, and we already
                    // have an instrument, then apply it to the instrument
                    // instead of inserting

                    if (instrument && (*midiEvent)->getTime() == 0) {
                        if ((*midiEvent)->getData1() == MIDI_CONTROLLER_VOLUME) {
                            instrument->setControllerValue(MIDI_CONTROLLER_VOLUME, (*midiEvent)->getData2());
                            break;
                        }
                        if ((*midiEvent)->getData1() == MIDI_CONTROLLER_PAN) {
                            instrument->setControllerValue(MIDI_CONTROLLER_PAN, (*midiEvent)->getData2());
                            break;
                        }
                        if ((*midiEvent)->getData1() == MIDI_CONTROLLER_ATTACK) {
                            instrument->setControllerValue(MIDI_CONTROLLER_ATTACK, (*midiEvent)->getData2());
                            break;
                        }
                        if ((*midiEvent)->getData1() == MIDI_CONTROLLER_RELEASE) {
                            instrument->setControllerValue(MIDI_CONTROLLER_RELEASE, (*midiEvent)->getData2());
                            break;
                        }
                        if ((*midiEvent)->getData1() == MIDI_CONTROLLER_FILTER) {
                            instrument->setControllerValue(MIDI_CONTROLLER_FILTER, (*midiEvent)->getData2());
                            break;
                        }
                        if ((*midiEvent)->getData1() == MIDI_CONTROLLER_RESONANCE) {
                            instrument->setControllerValue(MIDI_CONTROLLER_RESONANCE, (*midiEvent)->getData2());
                            break;
                        }
                        if ((*midiEvent)->getData1() == MIDI_CONTROLLER_CHORUS) {
                            instrument->setControllerValue(MIDI_CONTROLLER_CHORUS, (*midiEvent)->getData2());
                            break;
                        }
                        if ((*midiEvent)->getData1() == MIDI_CONTROLLER_REVERB) {
                            instrument->setControllerValue(MIDI_CONTROLLER_REVERB, (*midiEvent)->getData2());
                            break;
                        }
                    }

                    rosegardenEvent =
                        Controller((*midiEvent)->getData1(),
                                   (*midiEvent)->getData2()).
                        getAsEvent(rosegardenTime);
                    break;

                case MIDI_PITCH_BEND:
                    rosegardenEvent =
                        PitchBend((*midiEvent)->getData2(),
                                  (*midiEvent)->getData1()).
                        getAsEvent(rosegardenTime);
                    break;

                case MIDI_SYSTEM_EXCLUSIVE:
                    rosegardenEvent =
                        SystemExclusive((*midiEvent)->getMetaMessage()).
                        getAsEvent(rosegardenTime);
                    break;

                case MIDI_POLY_AFTERTOUCH:
                    rosegardenEvent =
                        KeyPressure((*midiEvent)->getData1(),
                                    (*midiEvent)->getData2()).
                        getAsEvent(rosegardenTime);
                    break;

                case MIDI_CHNL_AFTERTOUCH:
                    rosegardenEvent =
                        ChannelPressure((*midiEvent)->getData1()).
                        getAsEvent(rosegardenTime);
                    break;

                default:

#ifdef MIDI_DEBUG
                    std::cerr << "MidiFile::convertToRosegarden - "
                    << "Unsupported event code = "
                    << (int)(*midiEvent)->getMessageType() << std::endl;
#endif

                    break;
                }

            if (rosegardenEvent) {
                //if (fillFromTime < rosegardenTime) {
                //   rosegardenSegment->fillWithRests(fillFromTime, rosegardenTime);
                //}
                if (endOfLastNote < rosegardenTime) {
                    rosegardenSegment->fillWithRests(endOfLastNote, rosegardenTime);
                }
                rosegardenSegment->insert(rosegardenEvent);
            }
        }

        if (rosegardenSegment->size() > 0) {

            // if all we have is key signatures and rests, take this
            // to be a conductor segment and don't insert it
            //
            bool keySigsOnly = true;
            bool haveKeySig = false;
            for (Segment::iterator i = rosegardenSegment->begin();
                    i != rosegardenSegment->end(); ++i) {
                if (!(*i)->isa(Rosegarden::Key::EventType) &&
                        !(*i)->isa(Note::EventRestType)) {
                    keySigsOnly = false;
                    break;
                } else if ((*i)->isa(Rosegarden::Key::EventType)) {
                    haveKeySig = true;
                }
            }

            if (keySigsOnly) {
                conductorSegment = rosegardenSegment;
                continue;
            } else if (!haveKeySig && conductorSegment) {
                // copy across any key sigs from the conductor segment

                timeT segmentStartTime = rosegardenSegment->getStartTime();
                timeT earliestEventEndTime = segmentStartTime;

                for (Segment::iterator i = conductorSegment->begin();
                        i != conductorSegment->end(); ++i) {
                    if ((*i)->getAbsoluteTime() + (*i)->getDuration() <
                            earliestEventEndTime) {
                        earliestEventEndTime =
                            (*i)->getAbsoluteTime() + (*i)->getDuration();
                    }
                    rosegardenSegment->insert(new Event(**i));
                }

                if (earliestEventEndTime < segmentStartTime) {
                    rosegardenSegment->fillWithRests(earliestEventEndTime,
                                                     segmentStartTime);
                }
            }

#ifdef MIDI_DEBUG
            std::cerr << "MIDI import: adding segment with start time " << rosegardenSegment->getStartTime() << " and end time " << rosegardenSegment->getEndTime() << std::endl;
            if (rosegardenSegment->getEndTime() == 2880) {
                std::cerr << "events:" << std::endl;
                for (Segment::iterator i = rosegardenSegment->begin();
                     i != rosegardenSegment->end(); ++i) {
                    std::cerr << "type = " << (*i)->getType() << std::endl;
                    std::cerr << "time = " << (*i)->getAbsoluteTime() << std::endl;
                    std::cerr << "duration = " << (*i)->getDuration() << std::endl;
                }
            }
#endif

            // add the Segment to the Composition and increment the
            // Rosegarden segment number
            //
            composition.addTrack(track);

            std::vector<TrackId> trackIds;
            trackIds.push_back(track->getId());
            composition.notifyTracksAdded(trackIds);

            composition.addSegment(rosegardenSegment);
            addedSegments.push_back(rosegardenSegment);
            compTrack++;

        } else {
            delete rosegardenSegment;
            rosegardenSegment = 0;
            delete track;
            track = 0;
        }
    }

    if (type == CONVERT_REPLACE || maxTime > composition.getEndMarker()) {
        composition.setEndMarker(composition.getBarEndForTime(maxTime));
    }

    for (std::vector<Segment *>::iterator i = addedSegments.begin();
         i != addedSegments.end(); ++i) {
        Segment *s = *i;
        if (s) {
            timeT duration = s->getEndMarkerTime() - s->getStartTime();
/*
            std::cerr << "duration = " << duration << " (start "
                      << s->getStartTime() << ", end " << s->getEndTime()
                      << ", marker " << s->getEndMarkerTime() << ")" << std::endl;
*/
            if (duration == 0) {
                s->setEndMarkerTime(s->getStartTime() +
                                    Note(Note::Crotchet).getDuration());
            }
            Instrument *instr = m_studio->getInstrumentFor(s);
            if (instr) {
                if (s->getLabel() == "") {
                    s->setLabel(m_studio->getSegmentName(instr->getId()));
                }
            }
        }
    }

    return true;
}

// Takes a Composition and turns it into internal MIDI representation
// that can then be written out to file.
//
// For the moment we should watch to make sure that multiple Segment
// (parts) don't equate to multiple segments in the MIDI Composition.
//
// This is a two pass operation - firstly convert the RG Composition
// into MIDI events and insert anything extra we need (i.e. NOTE OFFs)
// with absolute times before then processing all timings into delta
// times.
//
//
void
MidiFile::convertToMidi(Composition &comp)
{

    MappedBufMetaIterator * metaiterator =
        RosegardenMainWindow::self()->
        getSequenceManager()->
        makeTempMetaiterator();

    RealTime start = comp.getElapsedRealTime(comp.getStartMarker());
    RealTime end   = comp.getElapsedRealTime(comp.getEndMarker());
    // For ramping, we need to get MappedEvents in order, but
    // fetchEvents's order is only approximately
    // right, so we sort events first.
    SortingInserter sorter;
    metaiterator->jumpToTime(start);
    // Give the end a little margin to make it insert noteoffs at the
    // end.  If they tied with the end they'd get lost.
    metaiterator->
        fetchEvents(sorter, start, end + RealTime(0,1000));
    delete metaiterator;
    MidiInserter inserter(comp, 480, end);
    sorter.insertSorted(inserter);
    inserter.assignToMidiFile(*this);
}



// Convert an integer into a two byte representation and
// write out to the MidiFile.
//
void
MidiFile::intToMidiBytes(std::ofstream* midiFile, int number)
{
    MidiByte upper;
    MidiByte lower;

    upper = (number & 0xFF00) >> 8;
    lower = (number & 0x00FF);

    *midiFile << (MidiByte) upper;
    *midiFile << (MidiByte) lower;

}

void
MidiFile::longToMidiBytes(std::ofstream* midiFile, unsigned long number)
{
    MidiByte upper1;
    MidiByte lower1;
    MidiByte upper2;
    MidiByte lower2;

    upper1 = (number & 0xff000000) >> 24;
    lower1 = (number & 0x00ff0000) >> 16;
    upper2 = (number & 0x0000ff00) >> 8;
    lower2 = (number & 0x000000ff);

    *midiFile << (MidiByte) upper1;
    *midiFile << (MidiByte) lower1;
    *midiFile << (MidiByte) upper2;
    *midiFile << (MidiByte) lower2;

}

// Turn a delta time into a MIDI time - overlapping into
// a maximum of four bytes using the MSB as the carry on
// flag.
//
std::string
MidiFile::longToVarBuffer(unsigned long number)
{
    std::string rS;

    long inNumber = number;
    long outNumber;

    // get the lowest 7 bits of the number
    outNumber = number & 0x7f;

    // Shift and test and move the numbers
    // on if we need them - setting the MSB
    // as we go.
    //
    while ((inNumber >>= 7 ) > 0) {
        outNumber <<= 8;
        outNumber |= 0x80;
        outNumber += (inNumber & 0x7f);
    }

    // Now move the converted number out onto the buffer
    //
    while (true) {
        rS += (MidiByte)(outNumber & 0xff);
        if (outNumber & 0x80)
            outNumber >>= 8;
        else
            break;
    }

    return rS;
}



// Write out the MIDI file header
//
bool
MidiFile::writeHeader(std::ofstream* midiFile)
{
    // Our identifying Header string
    //
    *midiFile << MIDI_FILE_HEADER.c_str();

    // Write number of Bytes to follow
    //
    *midiFile << (MidiByte) 0x00;
    *midiFile << (MidiByte) 0x00;
    *midiFile << (MidiByte) 0x00;
    *midiFile << (MidiByte) 0x06;

    // Write File Format
    //
    *midiFile << (MidiByte) 0x00;
    *midiFile << (MidiByte) m_format;

    // Number of Tracks we're writing out
    //
    intToMidiBytes(midiFile, m_numberOfTracks);

    // Timing Division
    //
    intToMidiBytes(midiFile, m_timingDivision);

    return (true);
}

// Write a MIDI track to file
//
bool
MidiFile::writeTrack(std::ofstream* midiFile, TrackId trackNumber)
{
    bool retOK = true;
    MidiByte eventCode = 0;
    MidiTrack::iterator midiEvent;

    // First we write into the trackBuffer, then write it out to the
    // file with it's accompanying length.
    //
    string trackBuffer;

    long progressTotal = (long)m_midiComposition[trackNumber].size();
    long progressCount = 0;

    for (midiEvent = m_midiComposition[trackNumber].begin();
            midiEvent != m_midiComposition[trackNumber].end();
            ++midiEvent) {

        // HACK for #1404.  I gave up trying to find where the events
        // were originating, and decided to try just stripping them.  If
        // you can't do it right, do it badly, and somebody will
        // eventually freak out, then fix it the right way.
        if ((*midiEvent)->getData1() == 121) {
            std::cerr << "MidiFile::writeTrack(): Found controller 121.  Skipping.  This is a HACK to address BUG #1404." << std::endl;
            continue;
        }

        // Write the time to the buffer in MIDI format
        //
        //
        trackBuffer += longToVarBuffer((*midiEvent)->getTime());

#ifdef MIDI_DEBUG
        std::cerr << "MIDI event for channel "
                  << (int)(*midiEvent)->getChannelNumber() << " (track "
                  << (int)trackNumber << ") "
                  << " time" << (*midiEvent)->getTime()
                  << std::endl;
        (*midiEvent)->print();
#endif

        if ((*midiEvent)->isMeta()) {
            trackBuffer += MIDI_FILE_META_EVENT;
            trackBuffer += (*midiEvent)->getMetaEventCode();

            // Variable length number field
            trackBuffer += longToVarBuffer((*midiEvent)->
                                           getMetaMessage().length());

            trackBuffer += (*midiEvent)->getMetaMessage();
            eventCode = 0;
        } else {
            // Send the normal event code (with encoded channel information)
            //
            // Fix for 674731 by Pedro Lopez-Cabanillas (20030531)
            if (((*midiEvent)->getEventCode() != eventCode) ||
                    ((*midiEvent)->getEventCode() == MIDI_SYSTEM_EXCLUSIVE)) {
                trackBuffer += (*midiEvent)->getEventCode();
                eventCode = (*midiEvent)->getEventCode();
            }

            // Send the relevant data
            //
            switch ((*midiEvent)->getMessageType()) {
            case MIDI_NOTE_ON:
            case MIDI_NOTE_OFF:
            case MIDI_POLY_AFTERTOUCH:
                trackBuffer += (*midiEvent)->getData1();
                trackBuffer += (*midiEvent)->getData2();
                break;

            case MIDI_CTRL_CHANGE:
                trackBuffer += (*midiEvent)->getData1();
                trackBuffer += (*midiEvent)->getData2();
                break;

            case MIDI_PROG_CHANGE:
                trackBuffer += (*midiEvent)->getData1();
                break;

            case MIDI_CHNL_AFTERTOUCH:
                trackBuffer += (*midiEvent)->getData1();
                break;

            case MIDI_PITCH_BEND:
                trackBuffer += (*midiEvent)->getData1();
                trackBuffer += (*midiEvent)->getData2();
                break;

            case MIDI_SYSTEM_EXCLUSIVE:

                // write out message length
                trackBuffer +=
                    longToVarBuffer((*midiEvent)->getMetaMessage().length());

                // now the message
                trackBuffer += (*midiEvent)->getMetaMessage();

                break;

            default:
#ifdef MIDI_DEBUG

                std::cerr << "MidiFile::writeTrack()"
                << " - cannot write unsupported MIDI event"
                << endl;
#endif

                break;
            }
        }

        // For the moment just keep the app updating until we work
        // out a good way of accounting for this write.
        //
        ++progressCount;

        if (progressCount % 500 == 0) {
            emit progress(progressCount * 100 / progressTotal);
            //qApp->processEvents(500);
            qApp->processEvents(QEventLoop::AllEvents);
        }
    }

    // Now we write the track - First the standard header..
    //
    *midiFile << MIDI_TRACK_HEADER.c_str();

    // ..now the length of the buffer..
    //
    longToMidiBytes(midiFile, (long)trackBuffer.length());

    // ..then the buffer itself..
    //
    *midiFile << trackBuffer;

    return (retOK);
}

// Writes out a MIDI file from the internal Midi representation
//
bool
MidiFile::write()
{
    bool retOK = true;

    std::ofstream *midiFile =
        new std::ofstream(m_fileName.toLocal8Bit(), ios::out | ios::binary);


    if (!(*midiFile)) {
#ifdef MIDI_DEBUG
        std::cerr << "MidiFile::write() - can't write file" << endl;
#endif

        m_format = MIDI_FILE_NOT_LOADED;
        return false;
    }

    // Write out the Header
    //
    writeHeader(midiFile);

    // And now the tracks
    //
    for (TrackId i = 0; i < m_numberOfTracks; i++ )
        if (!writeTrack(midiFile, i))
            retOK = false;

    midiFile->close();

    if (!retOK)
        m_format = MIDI_FILE_NOT_LOADED;

    return (retOK);
}

// Delete dead NOTE OFF and NOTE ON/Zero Velocty Events after
// reading them and modifying their relevant NOTE ONs
//
bool
MidiFile::consolidateNoteOffEvents(TrackId track)
{
    MidiTrack::iterator nOE, mE = m_midiComposition[track].begin();
    bool notesOnTrack = false;
    bool noteOffFound;

    for (;mE != m_midiComposition[track].end(); ++mE) {
        if ((*mE)->getMessageType() == MIDI_NOTE_ON && (*mE)->getVelocity() > 0) {
            // We've found a note - flag it
            //
            if (!notesOnTrack)
                notesOnTrack = true;

            noteOffFound = false;

            for (nOE = mE; nOE != m_midiComposition[track].end(); ++nOE) {
                if (((*nOE)->getChannelNumber() == (*mE)->getChannelNumber()) &&
                        ((*nOE)->getPitch() == (*mE)->getPitch()) &&
                        ((*nOE)->getMessageType() == MIDI_NOTE_OFF ||
                         ((*nOE)->getMessageType() == MIDI_NOTE_ON &&
                          (*nOE)->getVelocity() == 0x00))) {
                    timeT noteDuration = ((*nOE)->getTime() - (*mE)->getTime());

                    // Some MIDI files floating around in the real world
                    // apparently have NOTE ON followed immediately by NOTE OFF
                    // on percussion tracks.  Instead of setting the duration to
                    // 0 in this case, which has no meaning, set it to 1.
                    if (noteDuration == 0) {
                        std::cerr << "MidiFile::consolidateNoteOffEvents() - detected MIDI note duration of 0.  Using duration of 1.  Touch wood."
                                  << std::endl;
                        noteDuration = 1;
                    }
                    (*mE)->setDuration(noteDuration);

                    delete *nOE;
                    m_midiComposition[track].erase(nOE);

                    noteOffFound = true;
                    break;
                }
            }

            // If no matching NOTE OFF has been found then set
            // Event duration to length of Segment
            //
            if (noteOffFound == false) {
                --nOE; // avoid crash due to nOE == track.end()
                (*mE)->setDuration((*nOE)->getTime() - (*mE)->getTime());

            }
        }
    }

    return notesOnTrack;
}

// Clear down the MidiFile Composition
//
void
MidiFile::clearMidiComposition()
{
    for (MidiComposition::iterator ci = m_midiComposition.begin();
            ci != m_midiComposition.end(); ++ci) {

        //std::cerr << "MidiFile::clearMidiComposition: track " << ci->first << std::endl;

        for (MidiTrack::iterator ti = ci->second.begin();
                ti != ci->second.end(); ++ti) {
            delete *ti;
        }

        ci->second.clear();
    }

    m_midiComposition.clear();
    m_trackChannelMap.clear();
}


}

#include "MidiFile.moc"
