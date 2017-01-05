/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.
    See the AUTHORS file for more details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[MidiFile]"

#include "MidiFile.h"

#include "Midi.h"
#include "MidiEvent.h"
#include "base/Segment.h"
//#include "base/NotationTypes.h"
#include "base/BaseProperties.h"
#include "base/Track.h"
#include "base/Instrument.h"
#include "base/Studio.h"
#include "base/MidiTypes.h"
#include "base/Profiler.h"
#include "document/RosegardenDocument.h"
#include "gui/application/RosegardenMainWindow.h"
#include "gui/seqmanager/SequenceManager.h"
#include "misc/Debug.h"
#include "misc/Strings.h"
#include "sound/MappedBufMetaIterator.h"
#include "sound/MidiInserter.h"
#include "sound/SortingInserter.h"

#include <QProgressDialog>

#include <fstream>
#include <string>
#include <sstream>

static const char MIDI_FILE_HEADER[] = "MThd";
static const char MIDI_TRACK_HEADER[] = "MTrk";

namespace Rosegarden
{


MidiFile::MidiFile() :
    m_format(MIDI_FILE_NOT_LOADED),
    m_numberOfTracks(0),
    m_timingFormat(MIDI_TIMING_PPQ_TIMEBASE),
    m_timingDivision(0),
    m_fps(0),
    m_subframes(0),
    m_fileSize(0),
    m_trackByteCount(0),
    m_decrementCount(false),
    m_bytesRead(0)
{
}

MidiFile::~MidiFile()
{
    // Delete all the event objects.
    clearMidiComposition();
}

long
MidiFile::midiBytesToLong(const std::string &bytes)
{
    if (bytes.length() != 4) {
        RG_WARNING << "midiBytesToLong(): WARNING: Wrong length for long data (" << bytes.length() << ", should be 4)";

        // TRANSLATOR: "long" is a C++ data type
        throw Exception(qstrtostr(QObject::tr("Wrong length for long data in MIDI stream")));
    }

    long longRet = static_cast<long>(static_cast<MidiByte>(bytes[0])) << 24 |
                   static_cast<long>(static_cast<MidiByte>(bytes[1])) << 16 |
                   static_cast<long>(static_cast<MidiByte>(bytes[2])) << 8 |
                   static_cast<long>(static_cast<MidiByte>(bytes[3]));

    RG_DEBUG << "midiBytesToLong(" << static_cast<long>(static_cast<MidiByte>(bytes[0])) << "," << static_cast<long>(static_cast<MidiByte>(bytes[1])) << "," << static_cast<long>(static_cast<MidiByte>(bytes[2])) << "," << static_cast<long>(static_cast<MidiByte>(bytes[3])) << ") -> " << longRet;

    return longRet;
}

int
MidiFile::midiBytesToInt(const std::string &bytes)
{
    if (bytes.length() != 2) {
        RG_WARNING << "midiBytesToInt(): WARNING: Wrong length for int data (" << bytes.length() << ", should be 2)";

        // TRANSLATOR: "int" is a C++ data type
        throw Exception(qstrtostr(QObject::tr("Wrong length for int data in MIDI stream")));
    }

    return static_cast<int>(static_cast<MidiByte>(bytes[0])) << 8 |
           static_cast<int>(static_cast<MidiByte>(bytes[1]));
}

MidiByte
MidiFile::read(std::ifstream *midiFile)
{
    return static_cast<MidiByte>(read(midiFile, 1)[0]);
}

std::string
MidiFile::read(std::ifstream *midiFile, unsigned long numberOfBytes)
{
    if (midiFile->eof()) {
        RG_WARNING << "read(): MIDI file EOF - got 0 bytes out of " << numberOfBytes;

        throw Exception(qstrtostr(QObject::tr("End of MIDI file encountered while reading")));
    }

    // For each track section we can read only m_trackByteCount bytes.
    if (m_decrementCount  &&
        numberOfBytes > static_cast<unsigned long>(m_trackByteCount)) {

        RG_WARNING << "read(): Attempt to get more bytes than allowed on Track (" << numberOfBytes << " > " << m_trackByteCount << ")";

        throw Exception(qstrtostr(QObject::tr("Attempt to get more bytes than expected on Track")));
    }

    char fileMidiByte;
    std::string stringRet;

    // ??? Reading one byte at a time is terribly slow.  However, this
    //     routine is usually called with numberOfBytes == 1.
    while (stringRet.length() < numberOfBytes  &&
           midiFile->read(&fileMidiByte, 1)) {
        stringRet += fileMidiByte;

        // Kick the event loop to make sure the UI doesn't become
        // unresponsive during a long load.
        qApp->processEvents();
    }

    // Unexpected EOF
    if (stringRet.length() < numberOfBytes) {
        RG_WARNING << "read(): Attempt to read past file end - got " << stringRet.length() << " bytes out of " << numberOfBytes;

        throw Exception(qstrtostr(QObject::tr("Attempt to read past MIDI file end")));
    }

    if (m_decrementCount)
        m_trackByteCount -= numberOfBytes;

    m_bytesRead += numberOfBytes;

    // Every 2000 bytes...
    if (m_bytesRead >= 2000) {
        m_bytesRead = 0;

        // This is the first 20% of the "reading" process.
        int progressValue = static_cast<int>(
                static_cast<double>(midiFile->tellg()) /
                static_cast<double>(m_fileSize) * 20.0);
        // Update the progress dialog if one is connected.
        if (m_progressDialog) {
            if (m_progressDialog->wasCanceled())
                throw Exception(qstrtostr(QObject::tr("Cancelled by user.")));

            m_progressDialog->setValue(progressValue);
        }
        emit progress(progressValue);
    }

    return stringRet;
}

long
MidiFile::readNumber(std::ifstream *midiFile, int firstByte)
{
    if (midiFile->eof())
        return 0;

    MidiByte midiByte;

    // If we already have the first byte, use it
    if (firstByte >= 0) {
        midiByte = static_cast<MidiByte>(firstByte);
    } else {  // read it
        midiByte = read(midiFile);
    }

    long longRet = midiByte;

    // See MIDI spec section 4, pages 2 and 11.

    if (midiByte & 0x80) {
        longRet &= 0x7F;
        do {
            midiByte = read(midiFile);
            longRet = (longRet << 7) + (midiByte & 0x7F);
        } while (!midiFile->eof() && (midiByte & 0x80));
    }

    return longRet;
}

void
MidiFile::findNextTrack(std::ifstream *midiFile)
{
    // Conforms to recommendation in the MIDI spec, section 4, page 3:
    // "Your programs should /expect/ alien chunks and treat them as if
    // they weren't there."  (Emphasis theirs.)

    // Assume not found.
    m_decrementCount = false;
    m_trackByteCount = -1;

    // For each chunk
    while (!midiFile->eof()) {
        // Read the chunk type and size.
        std::string chunkType = read(midiFile, 4);
        long chunkSize = midiBytesToLong(read(midiFile, 4));

        // If we've found a track chunk
        if (chunkType.compare(0, 4, MIDI_TRACK_HEADER) == 0) {
            m_trackByteCount = chunkSize;
            m_decrementCount = true;
            return;
        }

        RG_DEBUG << "findNextTrack(): skipping alien chunk.  Type:" << chunkType;

        // Alien chunk encountered, initiate evasive maneuvers (skip it).
        midiFile->seekg(chunkSize, std::ios::cur);
    }

    // Track not found.
    RG_WARNING << "findNextTrack(): Couldn't find Track";
    throw Exception(qstrtostr(QObject::tr("File corrupted or in non-standard format")));
}

bool
MidiFile::read(const QString &filename)
{
    RG_DEBUG << "read(): filename = " << filename;

    clearMidiComposition();

    // Open the file
    std::ifstream *midiFile =
            new std::ifstream(filename.toLocal8Bit(),
                              std::ios::in | std::ios::binary);

    if (!(*midiFile)) {
        m_error = "File not found or not readable.";
        m_format = MIDI_FILE_NOT_LOADED;
        return false;
    }

    // Compute the file size so we can report progress.
    midiFile->seekg(0, std::ios::end);
    m_fileSize = midiFile->tellg();
    midiFile->seekg(0, std::ios::beg);

    // The parsing process throws string exceptions back up here if we
    // run into trouble which we can then pass back out to whomever
    // called us using m_error and a nice bool.
    try {
        // Parse the MIDI header first.
        parseHeader(midiFile);

        // For each track chunk in the MIDI file.
        for (unsigned track = 0; track < m_numberOfTracks; ++track) {

            RG_DEBUG << "read(): Parsing MIDI file track " << track;

            // Skip any alien chunks.
            findNextTrack(midiFile);

            RG_DEBUG << "read(): Track has " << m_trackByteCount << " bytes";

            // Read the track into m_midiComposition.
            parseTrack(midiFile);
        }

    } catch (const Exception &e) {
        RG_WARNING << "read() - caught exception - " << e.getMessage();

        m_error = e.getMessage();
        m_format = MIDI_FILE_NOT_LOADED;
        return false;
    }

    midiFile->close();

    return true;
}

void
MidiFile::parseHeader(std::ifstream *midiFile)
{
    // The basic MIDI header is 14 bytes.
    std::string midiHeader = read(midiFile, 14);

    if (midiHeader.size() < 14) {
        RG_WARNING << "parseHeader() - file header undersized";
        throw Exception(qstrtostr(QObject::tr("Not a MIDI file")));
    }

    if (midiHeader.compare(0, 4, MIDI_FILE_HEADER) != 0) {
        RG_WARNING << "parseHeader() - file header not found or malformed";
        throw Exception(qstrtostr(QObject::tr("Not a MIDI file")));
    }

    long chunkSize = midiBytesToLong(midiHeader.substr(4, 4));
    m_format = static_cast<FileFormatType>(
            midiBytesToInt(midiHeader.substr(8, 2)));
    m_numberOfTracks = midiBytesToInt(midiHeader.substr(10, 2));
    m_timingDivision = midiBytesToInt(midiHeader.substr(12, 2));
    m_timingFormat = MIDI_TIMING_PPQ_TIMEBASE;

    if (m_format == MIDI_SEQUENTIAL_TRACK_FILE) {
        RG_WARNING << "parseHeader() - can't load sequential track (Format 2) MIDI file";
        throw Exception(qstrtostr(QObject::tr("Unexpected MIDI file format")));
    }

    if (m_timingDivision > 32767) {
        RG_DEBUG << "parseHeader() - file uses SMPTE timing";

        m_timingFormat = MIDI_TIMING_SMPTE;
        m_fps = 256 - (m_timingDivision >> 8);
        m_subframes = (m_timingDivision & 0xff);
    }

    if (chunkSize > 6) {
        // Skip any remaining bytes in the header chunk.
        // MIDI spec section 4, page 5: "[...] more parameters may be
        // added to the MThd chunk in the future: it is important to
        // read and honor the length, even if it is longer than 6."
        midiFile->seekg(chunkSize - 6, std::ios::cur);
    }
}

static const std::string defaultTrackName = "Imported MIDI";

void
MidiFile::parseTrack(std::ifstream *midiFile)
{
    // The term "Track" is overloaded in this routine.  The first
    // meaning is a track in the MIDI file.  That is what this routine
    // processes.  A single track from a MIDI file.  The second meaning
    // is a track in m_midiComposition.  This is the most common usage.
    // To improve clarity, "MIDI file track" will be used to refer to
    // the first sense of the term.  Occasionally, "m_midiComposition
    // track" will be used to refer to the second sense.

    // Absolute time of the last event on any track.
    unsigned long eventTime = 0;

    // lastTrackNum is the track for all events provided they're
    // all on the same channel.  If we find events on more than one
    // channel, we increment lastTrackNum and record the mapping from
    // channel to trackNum in channelToTrack.
    TrackId lastTrackNum = m_midiComposition.size();

    // MIDI channel to m_midiComposition track.
    // Note: This would be a vector<TrackId> but TrackId is unsigned
    //       and we need -1 to indicate "not yet used"
    std::vector<int> channelToTrack(16, -1);

    // This is used to store the last absolute time found on each track,
    // allowing us to modify delta-times correctly when separating events
    // out from one to multiple tracks
    std::map<int /*track*/, unsigned long /*lastTime*/> lastEventTime;
    lastEventTime[lastTrackNum] = 0;

    // Meta-events don't have a channel, so we place them in a fixed
    // track number instead
    TrackId metaTrack = lastTrackNum;

    std::string trackName = defaultTrackName;
    std::string instrumentName;

    // Remember the last non-meta status byte (-1 if we haven't seen one)
    int runningStatus = -1;

    bool firstTrack = true;

    RG_DEBUG << "parseTrack(): last track number is " << lastTrackNum;

    // While there is still data to read in the MIDI file track.
    // Why "m_trackByteCount > 1" instead of "m_trackByteCount > 0"?  Since
    // no event and its associated delta time can fit in just one
    // byte, a single remaining byte in the MIDI file track has to be padding.
    // This is obscure and non-standard, but such files do exist; ordinarily
    // there should be no bytes in the MIDI file track after the last event.
    while (!midiFile->eof()  &&  m_trackByteCount > 1) {

        unsigned long deltaTime = readNumber(midiFile);

        RG_DEBUG << "parseTrack(): read delta time " << deltaTime;

        // Compute the absolute time for the event.
        eventTime += deltaTime;

        // Get a single byte
        MidiByte midiByte = read(midiFile);

        MidiByte statusByte = 0;
        MidiByte data1 = 0;

        // If this is a status byte, use it.
        if (midiByte & MIDI_STATUS_BYTE_MASK) {
            RG_DEBUG << "parseTrack(): have new status byte" << QString("0x%1").arg(midiByte, 0, 16);

            statusByte = midiByte;
            data1 = read(midiFile);
        } else {  // Use running status.
            // If we haven't seen a status byte yet, fail.
            if (runningStatus < 0)
                throw Exception(qstrtostr(QObject::tr("Running status used for first event in track")));

            statusByte = static_cast<MidiByte>(runningStatus);
            data1 = midiByte;

            RG_DEBUG << "parseTrack(): using running status (byte " << QString("0x%1").arg(midiByte, 0, 16) << " found)";
        }

        if (statusByte == MIDI_FILE_META_EVENT) {

            MidiByte metaEventCode = data1;
            unsigned messageLength = readNumber(midiFile);

            RG_DEBUG << "parseTrack(): Meta event of type " << QString("0x%1").arg(metaEventCode, 0, 16) << " and " << messageLength << " bytes found";

            std::string metaMessage = read(midiFile, messageLength);

            // Compute the difference between this event and the previous
            // event on this track.
            deltaTime = eventTime - lastEventTime[metaTrack];
            // Store the absolute time of the last event on this track.
            lastEventTime[metaTrack] = eventTime;

            // create and store our event
            MidiEvent *e = new MidiEvent(deltaTime,
                                         MIDI_FILE_META_EVENT,
                                         metaEventCode,
                                         metaMessage);
            m_midiComposition[metaTrack].push_back(e);

            if (metaEventCode == MIDI_TRACK_NAME)
                trackName = metaMessage;
            else if (metaEventCode == MIDI_INSTRUMENT_NAME)
                instrumentName = metaMessage;

            // Get the next event.
            continue;
        }

        runningStatus = statusByte;

        int channel = (statusByte & MIDI_CHANNEL_NUM_MASK);

        // If this channel hasn't been seen yet in this MIDI file track
        if (channelToTrack[channel] == -1) {
            // If this is the first m_midiComposition track we've
            // used
            if (firstTrack) {
                // We've already allocated an m_midiComposition track for
                // the first channel we encounter.  Use it.
                firstTrack = false;
            } else {  // We need a new track.
                // Allocate a new track for this channel.
                ++lastTrackNum;
                lastEventTime[lastTrackNum] = 0;
            }

            RG_DEBUG << "parseTrack(): new channel map entry: channel " << channel << " -> track " << lastTrackNum;

            channelToTrack[channel] = lastTrackNum;
            m_trackChannelMap[lastTrackNum] = channel;
        }

        TrackId trackNum = channelToTrack[channel];

#ifdef DEBUG
        {
            static int prevTrackNum = -1;
            static int prevChannel = -1;

            // If we're on a different track or channel
            if (prevTrackNum != static_cast<int>(trackNum)  ||
                prevChannel != channel) {

                // Report it for debugging.
                RG_DEBUG << "parseTrack(): track number for channel " << channel << " is " << trackNum;

                prevTrackNum = trackNum;
                prevChannel = channel;
            }
        }
#endif

        // Compute the difference between this event and the previous
        // event on this track.
        deltaTime = eventTime - lastEventTime[trackNum];
        // Store the absolute time of the last event on this track.
        lastEventTime[trackNum] = eventTime;

        switch (statusByte & MIDI_MESSAGE_TYPE_MASK) {
        case MIDI_NOTE_ON:        // These events have two data bytes.
        case MIDI_NOTE_OFF:
        case MIDI_POLY_AFTERTOUCH:
        case MIDI_CTRL_CHANGE:
        case MIDI_PITCH_BEND:
            {
                MidiByte data2 = read(midiFile);

                // create and store our event
                MidiEvent *midiEvent =
                        new MidiEvent(deltaTime, statusByte, data1, data2);
                m_midiComposition[trackNum].push_back(midiEvent);

                if (statusByte != MIDI_PITCH_BEND) {
                    RG_DEBUG << "parseTrack(): MIDI event for channel " << channel + 1 << " (track " << trackNum << ')';
                    RG_DEBUG << *midiEvent;
                }
            }
            break;

        case MIDI_PROG_CHANGE:    // These events have a single data byte.
        case MIDI_CHNL_AFTERTOUCH:
            {
                RG_DEBUG << "parseTrack(): Program change (Cn) or channel aftertouch (Dn): time " << deltaTime << ", code " << QString("0x%1").arg(statusByte, 0, 16) << ", data " << (int) data1  << " going to track " << trackNum;

                // create and store our event
                MidiEvent *midiEvent =
                        new MidiEvent(deltaTime, statusByte, data1);
                m_midiComposition[trackNum].push_back(midiEvent);
            }
            break;

        case MIDI_SYSTEM_EXCLUSIVE:
            {
                unsigned messageLength = readNumber(midiFile, data1);

                RG_DEBUG << "parseTrack(): SysEx of " << messageLength << " bytes found";

                std::string sysex = read(midiFile, messageLength);

                if (MidiByte(sysex[sysex.length() - 1]) !=
                        MIDI_END_OF_EXCLUSIVE) {
                    RG_WARNING << "parseTrack() - malformed or unsupported SysEx type";
                    continue;
                }

                // Chop off the EOX.
                sysex = sysex.substr(0, sysex.length() - 1);

                // create and store our event
                MidiEvent *midiEvent =
                        new MidiEvent(deltaTime,
                                      MIDI_SYSTEM_EXCLUSIVE,
                                      sysex);
                m_midiComposition[trackNum].push_back(midiEvent);
            }
            break;

        case MIDI_END_OF_EXCLUSIVE:
            RG_WARNING << "parseTrack() - Found a stray MIDI_END_OF_EXCLUSIVE";
            break;

        default:
            RG_WARNING << "parseTrack() - Unsupported MIDI Status Byte:  " << QString("0x%1").arg(statusByte, 0, 16);
            break;
        }
    }

    // If the MIDI file track has a padding byte, read and discard it
    // to make sure that the stream is positioned at the beginning of
    // the following MIDI file track (if there is one.)
    if (m_trackByteCount == 1)
        midiFile->ignore();

    if (instrumentName != "")
        trackName += " (" + instrumentName + ")";

    // Fill out the Track Names
    for (TrackId i = metaTrack; i <= lastTrackNum; ++i)
        m_trackNames.push_back(trackName);
}

bool
MidiFile::convertToRosegarden(const QString &filename, RosegardenDocument *doc)
{
    Profiler profiler("MidiFile::convertToRosegarden");

    // Read the MIDI file into m_midiComposition.
    if (!read(filename))
        return false;

    if (m_midiComposition.size() != m_trackNames.size()) {
        RG_WARNING << "convertToRosegarden(): m_midiComposition and m_trackNames size mismatch";
        return false;
    }

    Composition &composition = doc->getComposition();
    Studio &studio = doc->getStudio();

    composition.clear();

    // Clear down the assigned Instruments we already have
    studio.unassignAllInstruments();

    RG_DEBUG << "convertToRosegarden(): MIDI COMP SIZE = " << m_midiComposition.size();

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

        typedef std::map<int /*time*/, tempoT> TempoMap;
        TempoMap tempi;

        // For each track
        for (TrackId i = 0; i < m_midiComposition.size(); ++i) {
            // For each event
            for (MidiTrack::const_iterator midiEvent = m_midiComposition[i].begin();
                 midiEvent != m_midiComposition[i].end();
                 ++midiEvent) {        
                if ((*midiEvent)->isMeta()) {
                    // If we have a set tempo meta-event
                    if ((*midiEvent)->getMetaEventCode() == MIDI_SET_TEMPO) {
                        MidiByte m0 = (*midiEvent)->getMetaMessage()[0];
                        MidiByte m1 = (*midiEvent)->getMetaMessage()[1];
                        MidiByte m2 = (*midiEvent)->getMetaMessage()[2];
                        long tempo = (((m0 << 8) + m1) << 8) + m2;
                        if (tempo != 0) {
                            double qpm = 60000000.0 / double(tempo);
                            tempoT rgt(Composition::getTempoForQpm(qpm));
                            // Store the tempo in the tempo map.
                            // ??? Strange.  The event's time is a delta
                            //     time at this point.  Seems wrong.
                            tempi[(*midiEvent)->getTime()] = rgt;
                        }
                    }
                }
            }
        }

        // For each set tempo meta-event
        for (TempoMap::const_iterator i = tempi.begin();
             i != tempi.end();
             ++i) {
            timeT t = composition.getElapsedTimeForRealTime(
                    RealTime::frame2RealTime(i->first, m_fps * m_subframes));
            composition.addTempoAtTime(t, i->second);
        }
    }

    const int rosegardenPPQ = Note(Note::Crotchet).getDuration();
    const int midiFilePPQ = m_timingDivision ? m_timingDivision : 96;
    // Conversion factor.
    const double midiToRgTime =
            static_cast<double>(rosegardenPPQ) /
            static_cast<double>(midiFilePPQ);

    // Used to expand the composition if needed.
    timeT maxTime = 0;

    Segment *conductorSegment = 0;

    // Time Signature
    int numerator = 4;
    int denominator = 4;

    // Destination TrackId in the Composition.
    TrackId rosegardenTrackId = 0;

    // For each track
    // ??? BIG loop.
    for (TrackId trackId = 0;
         trackId < m_midiComposition.size();
         ++trackId) {

        // 20% total in file import itself (see read()) and then 80%
        // split over the tracks.
        int progressValue = 20 + static_cast<int>(
                80.0 * trackId / m_midiComposition.size());
        RG_DEBUG << "convertToRosegarden() progressValue: " << progressValue;
        if (m_progressDialog) {
            if (m_progressDialog->wasCanceled()) {
                m_error = qstrtostr(QObject::tr("Cancelled by user."));
                return false;
            }
            m_progressDialog->setValue(progressValue);
        }
        emit progress(progressValue);
        // Kick the event loop.
        qApp->processEvents();

        timeT absTime = 0;

        // Convert the event times from delta to absolute for
        // consolidateNoteEvents().
        for (MidiTrack::iterator eventIter = m_midiComposition[trackId].begin();
             eventIter != m_midiComposition[trackId].end();
             ++eventIter) {
            absTime += (*eventIter)->getTime();
            (*eventIter)->setTime(absTime);
        }

        // Consolidate NOTE ON and NOTE OFF events into NOTE ON events with
        // a duration.
        consolidateNoteEvents(trackId);

        InstrumentId instrumentId = MidiInstrumentBase;

        // If this track has a channel, use that channel's instrument.
        if (m_trackChannelMap.find(trackId) != m_trackChannelMap.end()) {
            instrumentId = MidiInstrumentBase + m_trackChannelMap[trackId];
        }

        Track *track = new Track(rosegardenTrackId,   // id
                                 instrumentId,        // instrument
                                 rosegardenTrackId,   // position
                                 m_trackNames[trackId],  // label
                                 false);              // muted

        Segment *segment = new Segment;
        segment->setLabel(m_trackNames[trackId]);
        segment->setTrack(rosegardenTrackId);
        segment->setStartTime(0);

        RG_DEBUG << "convertToRosegarden(): New Rosegarden track: id = " << rosegardenTrackId << ", instrument = " << instrumentId;

        // Used for filling the space between events with rests.  Also used
        // for padding the end of the track with rests.
        timeT endOfLastNote = 0;

        // Statistics.
        int noteCount = 0;
        int keySigCount = 0;

        // For each event on the current track
        // ??? BIG loop.
        for (MidiTrack::const_iterator midiEventIter =
                 m_midiComposition[trackId].begin();
             midiEventIter != m_midiComposition[trackId].end();
             ++midiEventIter) {
            const MidiEvent &midiEvent = **midiEventIter;

            // Kick the event loop.
            qApp->processEvents();

            const timeT midiAbsoluteTime = midiEvent.getTime();
            const timeT midiDuration = midiEvent.getDuration();
            timeT rosegardenTime = 0;
            timeT rosegardenDuration = 0;

            if (m_timingFormat == MIDI_TIMING_PPQ_TIMEBASE) {
                rosegardenTime =
                        static_cast<timeT>(midiAbsoluteTime * midiToRgTime);
                rosegardenDuration =
                        static_cast<timeT>(midiDuration * midiToRgTime);
            } else {

                // SMPTE timestamps are a count of the number of
                // subframes, where the number of subframes per frame
                // and frames per second have been defined in the file
                // header (stored as m_subframes, m_fps).  We need to
                // go through a realtime -> musical time conversion
                // for these, having added our tempo changes earlier
                
                rosegardenTime = composition.getElapsedTimeForRealTime(
                    RealTime::frame2RealTime(midiAbsoluteTime,
                                             m_fps * m_subframes));

                rosegardenDuration = composition.getElapsedTimeForRealTime(
                    RealTime::frame2RealTime(midiAbsoluteTime + midiDuration,
                                             m_fps * m_subframes))
                    - rosegardenTime;
            }

            RG_DEBUG << "convertToRosegarden(): MIDI file import: event time " << rosegardenTime
                     << ", duration " << rosegardenDuration
                     << ", event type " << QString("0x%1").arg(midiEvent.getMessageType(), 0, 16)
                     << ", previous max time " << maxTime
                     << ", potential max time " << (rosegardenTime + rosegardenDuration)
                     << ", ev raw time " << midiAbsoluteTime
                     << ", ev raw duration " << midiDuration
                     << ", crotchet " << Note(Note::Crotchet).getDuration()
                     << ", midiToRgTime " << midiToRgTime
                     << ", sfps " << m_fps * m_subframes;

            if (rosegardenTime + rosegardenDuration > maxTime)
                maxTime = rosegardenTime + rosegardenDuration;

            // If we don't have any events yet
            if (segment->empty()) {
                // Save the beginning of the bar so we can pad to the
                // left with rests.
                // ??? But if we have a loop with a precise start and end,
                //     this ruins the start.  This would preserve it:
                //       endOfLastNote = rosegardenTime;
                //     If the user wants the beginning on a bar, they can
                //     easily do that.  We shouldn't be modifying things.
                endOfLastNote = composition.getBarStartForTime(rosegardenTime);
            }

            // The incoming midiEvent is transformed into this rosegardenEvent.
            Event *rosegardenEvent = 0;

            if (midiEvent.isMeta()) {

                switch (midiEvent.getMetaEventCode()) {

                case MIDI_TEXT_EVENT: {
                    std::string text = midiEvent.getMetaMessage();
                    rosegardenEvent =
                            Text(text).getAsEvent(rosegardenTime);
                    break;
                }

                case MIDI_LYRIC: {
                    std::string text = midiEvent.getMetaMessage();
                    rosegardenEvent =
                            Text(text, Text::Lyric).getAsEvent(rosegardenTime);
                    break;
                }

                case MIDI_TEXT_MARKER: {
                    std::string text = midiEvent.getMetaMessage();
                    composition.addMarker(
                            new Marker(rosegardenTime, text, ""));
                    break;
                }

                case MIDI_COPYRIGHT_NOTICE:
                    composition.setCopyrightNote(midiEvent.getMetaMessage());
                    break;

                case MIDI_TRACK_NAME:
                    // We already handled this in parseTrack().
                    break;

                case MIDI_INSTRUMENT_NAME:
                    // We already handled this in parseTrack().
                    break;

                case MIDI_END_OF_TRACK: {
                    timeT trackEndTime = rosegardenTime;

                    // If the track's empty (or worse)
                    if (trackEndTime - segment->getStartTime() <= 0) {
                        RG_WARNING << "convertToRosegarden(): Zero-length track encountered";

                        // Make it a full bar.
                        trackEndTime = segment->getStartTime() +
                                Note(Note::Semibreve).getDuration() *
                                    numerator / denominator;
                    }

                    // If there's space between the last note and the track
                    // end, fill it out with rests.
                    if (endOfLastNote < trackEndTime) {
                        // If there's nothing in the segment yet, then we
                        // shouldn't fill with rests because we don't want
                        // to cause the otherwise empty segment to be created.
                        if (!segment->empty())
                            segment->fillWithRests(trackEndTime);
                    }

                    break;
                }

                case MIDI_SET_TEMPO:
                    // We've already handled the SMPTE case above.
                    if (m_timingFormat == MIDI_TIMING_PPQ_TIMEBASE) {
                        MidiByte m0 = midiEvent.getMetaMessage()[0];
                        MidiByte m1 = midiEvent.getMetaMessage()[1];
                        MidiByte m2 = midiEvent.getMetaMessage()[2];

                        // usecs per quarter-note
                        long midiTempo = (((m0 << 8) + m1) << 8) + m2;

                        if (midiTempo != 0) {
                            // Convert to quarter-notes per minute.
                            double qpm = 60000000.0 / midiTempo;
                            tempoT rosegardenTempo(
                                    Composition::getTempoForQpm(qpm));
                            //RG_DEBUG << "convertToRosegarden(): converted MIDI tempo " << midiTempo << " to Rosegarden tempo " << rosegardenTempo;
                            composition.addTempoAtTime(
                                    rosegardenTime, rosegardenTempo);
                        }
                    }
                    break;

                case MIDI_TIME_SIGNATURE: {
                    std::string metaMessage = midiEvent.getMetaMessage();

                    numerator = static_cast<int>(metaMessage[0]);
                    denominator = 1 << static_cast<int>(metaMessage[1]);

                    // A MIDI time signature has additional information that
                    // we ignore.  From the spec, section 4 page 10:
                    // metaMessage[2] "expresses the number of *MIDI clocks*
                    //   in a metronome tick."
                    // metaMessage[3] "expresses the number of notated
                    //   32nd-notes in what MIDI thinks of as a quarter-note
                    //   (24 MIDI clocks)."

                    // Fall back on 4/4
                    if (numerator == 0)
                        numerator = 4;
                    if (denominator == 0)
                        denominator = 4;

                    composition.addTimeSignature(
                            rosegardenTime,
                            TimeSignature(numerator, denominator));

                    break;
                }

                case MIDI_KEY_SIGNATURE: {
                    std::string metaMessage = midiEvent.getMetaMessage();

                    // Whether char is signed or unsigned is platform
                    // dependent.  Casting to signed char guarantees the
                    // correct results on all platforms.

                    int accidentals =
                            abs(static_cast<signed char>(metaMessage[0]));
                    bool isSharp =
                            (static_cast<signed char>(metaMessage[0]) >= 0);

                    bool isMinor = (metaMessage[1] != 0);

                    try {
                        rosegardenEvent =
                                Rosegarden::Key(accidentals, isSharp, isMinor).
                                        getAsEvent(rosegardenTime);
                    }
                    catch (...) {
                        RG_WARNING << "convertToRosegarden() - badly formed key signature";
                        break;
                    }

                    ++keySigCount;

                    break;
                }

                case MIDI_SEQUENCE_NUMBER:
                case MIDI_CHANNEL_PREFIX_OR_PORT:
                case MIDI_CUE_POINT:
                case MIDI_CHANNEL_PREFIX:
                case MIDI_SEQUENCER_SPECIFIC:
                case MIDI_SMPTE_OFFSET:
                default:
                    RG_WARNING << "convertToRosegarden() - unsupported META event code " << QString("0x%1").arg(midiEvent.getMetaEventCode(), 0, 16);
                    break;
                }

            } else {  // Not a meta-event.
                switch (midiEvent.getMessageType()) {
                case MIDI_NOTE_ON:

                    // Note-off?  Ignore.  Note-ons and note-offs have been
                    // consolidated.  Stray note-offs can be ignored.
                    if (midiEvent.getVelocity() == 0)
                        break;

                    endOfLastNote = rosegardenTime + rosegardenDuration;

                    RG_DEBUG << "convertToRosegarden(): note at " << rosegardenTime << ", duration " << rosegardenDuration << ", midi time " << midiAbsoluteTime << " and duration " << midiDuration;

                    rosegardenEvent = new Event(Note::EventType,
                                                rosegardenTime,
                                                rosegardenDuration);
                    rosegardenEvent->set<Int>(BaseProperties::PITCH,
                                              midiEvent.getPitch());
                    rosegardenEvent->set<Int>(BaseProperties::VELOCITY,
                                              midiEvent.getVelocity());

                    ++noteCount;

                    break;

                case MIDI_NOTE_OFF:
                    // Note-off?  Ignore.  Note-ons and note-offs have been
                    // consolidated.  Stray note-offs can be ignored.
                    break;

                case MIDI_PROG_CHANGE:
                    rosegardenEvent = ProgramChange(midiEvent.getData1()).
                                          getAsEvent(rosegardenTime);
                    break;

                case MIDI_CTRL_CHANGE:
                    rosegardenEvent =
                            Controller(midiEvent.getData1(),
                                       midiEvent.getData2()).
                                getAsEvent(rosegardenTime);
                    break;

                case MIDI_PITCH_BEND:
                    rosegardenEvent =
                            PitchBend(midiEvent.getData2(),
                                      midiEvent.getData1()).
                                getAsEvent(rosegardenTime);
                    break;

                case MIDI_SYSTEM_EXCLUSIVE:
                    rosegardenEvent =
                            SystemExclusive(midiEvent.getMetaMessage()).
                                getAsEvent(rosegardenTime);
                    break;

                case MIDI_POLY_AFTERTOUCH:
                    rosegardenEvent =
                            KeyPressure(midiEvent.getData1(),
                                        midiEvent.getData2()).
                                getAsEvent(rosegardenTime);
                    break;

                case MIDI_CHNL_AFTERTOUCH:
                    rosegardenEvent =
                            ChannelPressure(midiEvent.getData1()).
                                getAsEvent(rosegardenTime);
                    break;

                default:
                    RG_WARNING << "convertToRosegarden() - Unsupported event code = " << QString("0x%1").arg(midiEvent.getMessageType(), 0, 16);
                    break;
                }  // switch message type (non-meta-event)
            }  // if meta-event

            if (rosegardenEvent) {
                // If there's a gap between the last note and this event
                if (endOfLastNote < rosegardenTime) {
                    // Fill it with rests.
                    segment->fillWithRests(endOfLastNote, rosegardenTime);
                }
                segment->insert(rosegardenEvent);
            }
        }  // for each event

        // Empty segment?  Toss it.
        if (segment->empty()) {
            delete segment;
            segment = 0;
            delete track;
            track = 0;

            // Try the next track
            continue;
        }

        // Need to count rests separately.
        int restCount = 0;

        // For each event in the segment
        for (Segment::const_iterator i = segment->begin();
             i != segment->end();
             ++i) {
            const Event &event = **i;

            if (event.isa(Note::EventRestType))
                ++restCount;
        }

        int nonRestCount = segment->size() - restCount;

        RG_DEBUG << "convertToRosegarden(): Track analysis...";
        RG_DEBUG << "  Total events:" << segment->size();
        RG_DEBUG << "  Notes:" << noteCount;
        RG_DEBUG << "  Rests:" << restCount;
        RG_DEBUG << "  Non-rests:" << nonRestCount;
        RG_DEBUG << "  Key signatures:" << keySigCount;

        // Key sigs and no notes means a conductor segment.
        // Note: A conductor track also contains time signatures and
        //       tempo events, however we've already added those to the
        //       composition above.  They are not added to the segment.
        if (noteCount == 0  &&  keySigCount > 0) {
            conductorSegment = segment;

            // If this conductor segment has nothing but rests and key sigs,
            // there's no need to add it to the composition.
            // ??? This probably never happens.  Conductor tracks almost
            //     always have a few text events in them.
            if (nonRestCount == keySigCount) {
                // ??? Memory leak.  conductorSegment will not be deleted.
                continue;
            }
        }

        // If this track has no key sigs and we have a conductor segment
        if (keySigCount == 0  &&  conductorSegment) {
            // copy across any key sigs from the conductor segment

            timeT segmentStartTime = segment->getStartTime();
            timeT earliestEventEndTime = segmentStartTime;

            // For each event in the segment
            for (Segment::iterator i = conductorSegment->begin();
                 i != conductorSegment->end();
                 ++i) {
                const Event &event = **i;

                // If this isn't a key sig, try the next event.
                if (!event.isa(Rosegarden::Key::EventType))
                    continue;

                // ??? Should we really expand the segment?  Seems like
                //     it would make more sense to only add the key sigs
                //     that fall within the segment's time.  If there isn't
                //     one at the very start of the segment, then insert
                //     the previous.

                // Adjust the earliest event time if needed.
                if (event.getAbsoluteTime() + event.getDuration() <
                        earliestEventEndTime) {
                    earliestEventEndTime =
                            event.getAbsoluteTime() + event.getDuration();
                }

                segment->insert(new Event(event));
            }

            // Add rests to the beginning.
            // ??? No need.  It happens anyway.  Must be someone else doing
            //     this.
            if (earliestEventEndTime < segmentStartTime)
                segment->fillWithRests(earliestEventEndTime,
                                       segmentStartTime);
        }

        // Configure the Instrument based on events at time 0.
        configureInstrument(track, segment,
                            studio.getInstrumentById(instrumentId));

#ifdef DEBUG
        RG_DEBUG << "convertToRosegarden(): MIDI import: adding segment with start time " << segment->getStartTime() << " and end time " << segment->getEndTime();
        // For this secret event dump to be triggered, a track that ends
        // on beat 4 (2880 = 3 * 960) is required.  To generate such
        // a track in rg, set the time signature to 3/4 and reduce the
        // composition to one bar in length.  Make a single bar segment and
        // add events to it.  Export as MIDI.
        if (segment->getEndTime() == 2880) {
            RG_DEBUG << "  events:";
            for (Segment::iterator i = segment->begin();
                 i != segment->end(); ++i) {
                RG_DEBUG << **i;
            }
        }
#endif

        // This does not send a "track added" notification.
        composition.addTrack(track);

        // Notify Composition observers of the new track.
        std::vector<TrackId> trackIds;
        trackIds.push_back(track->getId());
        composition.notifyTracksAdded(trackIds);

        // This also sends a "segment added" notification.
        composition.addSegment(segment);

        // Next Track in the Composition.
        ++rosegardenTrackId;

    }  // for each track

    // Adjust the composition to hold the segments.
    composition.setEndMarker(composition.getBarEndForTime(maxTime));

    return true;
}

void MidiFile::configureInstrument(
        Track *track, Segment *segment, Instrument *instrument)
{
    if (!instrument)
        return;

    instrument->setPercussion(
            instrument->getNaturalChannel() ==
                    MIDI_PERCUSSION_CHANNEL);

    track->setInstrument(instrument->getId());

    Segment::iterator msbIter = segment->end();
    Segment::iterator lsbIter = segment->end();

    // For each event in the segment
    for (Segment::iterator i = segment->begin();
         i != segment->end();
         /* increment before use */) {
        // Increment before use.  This allows us to delete events
        // from the Segment.
        Segment::iterator j = i++;

        const Event &event = **j;

        // We only care about events at time 0.
        if (event.getAbsoluteTime() > 0)
            break;

        // If this is a Bank Select MSB, save it.
        if (event.isa(Controller::EventType)  &&
            event.get<Int>(Controller::NUMBER) ==
                    MIDI_CONTROLLER_BANK_MSB) {
            msbIter = j;
            continue;
        }

        // If this is a Bank Select LSB, save it.
        if (event.isa(Controller::EventType)  &&
            event.get<Int>(Controller::NUMBER) ==
                    MIDI_CONTROLLER_BANK_LSB) {
            lsbIter = j;
            continue;
        }

        // If this is a Program Change
        if (event.isa(ProgramChange::EventType)) {
            // Configure the instrument.
            int program = event.get<Int>(ProgramChange::PROGRAM);
            instrument->setProgramChange(program);
            instrument->setSendProgramChange(true);

            // Remove the program change from the Segment.
            segment->erase(j);

            continue;
        }

        // If this is a control change
        if (event.isa(Controller::EventType)) {
            int controller = event.get<Int>(Controller::NUMBER);

            // Only process the four that usually appear in the
            // Instrument Parameters box.
            if (controller == MIDI_CONTROLLER_VOLUME  ||
                controller == MIDI_CONTROLLER_PAN  ||
                controller == MIDI_CONTROLLER_REVERB  ||
                controller == MIDI_CONTROLLER_CHORUS) {

                // Configure the Instrument.
                instrument->setControllerValue(
                        controller,
                        event.get<Int>(Controller::VALUE));

                // Remove the event from the Segment.
                segment->erase(j);

                continue;
            }
        }
    }

    // If we have both msb and lsb for bank select
    if (msbIter != segment->end()  &&  lsbIter != segment->end()) {
        // Configure the Instrument
        instrument->setMSB((*msbIter)->get<Int>(Controller::VALUE));
        instrument->setLSB((*lsbIter)->get<Int>(Controller::VALUE));
        instrument->setSendBankSelect(true);

        // Remove the events.
        segment->erase(msbIter);
        segment->erase(lsbIter);
    }

    // Use the program name for a label if nothing else
    // was found.
    std::string programName = instrument->getProgramName();
    if (programName != "") {
        if (track->getLabel() == defaultTrackName)
            track->setLabel(instrument->getProgramName());
        if (segment->getLabel() == defaultTrackName)
            segment->setLabel(instrument->getProgramName());
    }
}

bool
MidiFile::convertToMidi(Composition &comp, const QString &filename)
{

    MappedBufMetaIterator *metaIterator =
        RosegardenMainWindow::self()->
        getSequenceManager()->
        makeTempMetaiterator();

    RealTime start = comp.getElapsedRealTime(comp.getStartMarker());
    RealTime end   = comp.getElapsedRealTime(comp.getEndMarker());

    // For ramping, we need to get MappedEvents in order, but
    // fetchEvents's order is only approximately
    // right, so we sort events first.
    SortingInserter sorter;

    // Fetch the channel setup for all MIDI tracks in Fixed channel mode.
    metaIterator->fetchFixedChannelSetup(sorter);

    metaIterator->jumpToTime(start);
    // Copy the events from metaIterator to sorter.
    // Give the end a little margin to make it insert noteoffs at the
    // end.  If they tied with the end they'd get lost.
    metaIterator->fetchEvents(sorter, start, end + RealTime(0,1000));

    delete metaIterator;

    MidiInserter inserter(comp, 480, end);
    // Copy the events from sorter to inserter.
    sorter.insertSorted(inserter);
    // Finally, copy the events from inserter to m_midiComposition.
    inserter.assignToMidiFile(*this);

    // Write m_midiComposition to the file.
    return write(filename);
}

void
MidiFile::writeInt(std::ofstream *midiFile, int number)
{
    *midiFile << static_cast<MidiByte>((number & 0xFF00) >> 8);
    *midiFile << static_cast<MidiByte>(number & 0x00FF);
}

void
MidiFile::writeLong(std::ofstream *midiFile, unsigned long number)
{
    *midiFile << static_cast<MidiByte>((number & 0xFF000000) >> 24);
    *midiFile << static_cast<MidiByte>((number & 0x00FF0000) >> 16);
    *midiFile << static_cast<MidiByte>((number & 0x0000FF00) >> 8);
    *midiFile << static_cast<MidiByte>(number & 0x000000FF);
}

std::string
MidiFile::longToVarBuffer(unsigned long value)
{
    // See WriteVarLen() in the MIDI Spec section 4, page 11.

    // Convert value into a "variable-length quantity" in buffer.

    // Start with the lowest 7 bits of the number
    long buffer = value & 0x7f;

    while ((value >>= 7 ) > 0) {
        buffer <<= 8;
        buffer |= 0x80;
        buffer += (value & 0x7f);
    }

    // Copy buffer to returnString and return it.

    std::string returnString;

    while (true) {
        returnString += (MidiByte)(buffer & 0xff);
        if (buffer & 0x80)
            buffer >>= 8;
        else
            break;
    }

    return returnString;
}

void
MidiFile::writeHeader(std::ofstream *midiFile)
{
    // Our identifying Header string
    *midiFile << MIDI_FILE_HEADER;

    // Write number of Bytes to follow
    *midiFile << static_cast<MidiByte>(0x00);
    *midiFile << static_cast<MidiByte>(0x00);
    *midiFile << static_cast<MidiByte>(0x00);
    *midiFile << static_cast<MidiByte>(0x06);

    writeInt(midiFile, static_cast<int>(m_format));
    writeInt(midiFile, m_numberOfTracks);
    writeInt(midiFile, m_timingDivision);
}

void
MidiFile::writeTrack(std::ofstream *midiFile, TrackId trackNumber)
{
    // For running status.
    MidiByte previousEventCode = 0;

    // First we write into trackBuffer, then we write trackBuffer
    // out to the file with its accompanying length.

    std::string trackBuffer;

    int progressTotal = static_cast<int>(m_midiComposition[trackNumber].size());
    int progressCount = 0;
    // Used to accumulate time deltas for skipped events.
    timeT skippedTime = 0;

    for (MidiTrack::iterator i = m_midiComposition[trackNumber].begin();
         i != m_midiComposition[trackNumber].end();
         ++i) {
        const MidiEvent &midiEvent = **i;

        // Do not write controller reset events to the buffer/file.
        // HACK for #1404.  I gave up trying to find where the events
        // were originating, and decided to try just stripping them.  If
        // you can't do it right, do it badly, and somebody will
        // eventually freak out, then fix it the right way.
        // ??? This is created in ChannelManager::insertControllers().
        // ??? Since we now have the "Allow Reset All Controllers" config
        //     option, we can probably get rid of this and tell people to
        //     use the config option.  If someone complains that 121's
        //     aren't appearing in MIDI files, that's probably the route
        //     we'll have to go.
        if (midiEvent.getData1() == MIDI_CONTROLLER_RESET) {
            RG_WARNING << "writeTrack(): Found controller 121.  Skipping.  This is a HACK to address BUG #1404.";

            // Keep track of the timestamps from skipped events so we can
            // add them to the next event that makes it through.
            skippedTime += midiEvent.getTime();

            continue;
        }

        // Add the time to the buffer in MIDI format
        trackBuffer += longToVarBuffer(midiEvent.getTime() + skippedTime);

        skippedTime = 0;

        RG_DEBUG << "MIDI event for channel "
                  << static_cast<int>(midiEvent.getChannelNumber())
                  << " (track " << trackNumber << ") "
                  << " time" << midiEvent.getTime();
        RG_DEBUG << midiEvent;

        if (midiEvent.isMeta()) {
            trackBuffer += MIDI_FILE_META_EVENT;
            trackBuffer += midiEvent.getMetaEventCode();

            trackBuffer += longToVarBuffer(midiEvent.
                                           getMetaMessage().length());
            trackBuffer += midiEvent.getMetaMessage();

            // Meta events cannot use running status.
            previousEventCode = 0;

        } else {  // non-meta event
            // If the event code has changed, or this is a SYSEX event, we
            // can't use running status.
            // Running status is "[f]or Voice and Mode messages only."
            // Sysex is a system message.  See the MIDI spec, Section 2,
            // page 5.
            if ((midiEvent.getEventCode() != previousEventCode) ||
                (midiEvent.getEventCode() == MIDI_SYSTEM_EXCLUSIVE)) {

                // Send the normal event code (with encoded channel information)
                trackBuffer += midiEvent.getEventCode();

                previousEventCode = midiEvent.getEventCode();
            }

            switch (midiEvent.getMessageType()) {
            case MIDI_NOTE_ON:  // These have two data bytes.
            case MIDI_NOTE_OFF:
            case MIDI_PITCH_BEND:
            case MIDI_CTRL_CHANGE:
            case MIDI_POLY_AFTERTOUCH:
                trackBuffer += midiEvent.getData1();
                trackBuffer += midiEvent.getData2();
                break;

            case MIDI_PROG_CHANGE:  // These have one data byte.
            case MIDI_CHNL_AFTERTOUCH:
                trackBuffer += midiEvent.getData1();
                break;

            case MIDI_SYSTEM_EXCLUSIVE:
                trackBuffer +=
                        longToVarBuffer(midiEvent.getMetaMessage().length());
                trackBuffer += midiEvent.getMetaMessage();
                break;

            default:
                RG_WARNING << "writeTrack() - cannot write unsupported MIDI event: " << QString("0x%1").arg(midiEvent.getMessageType(), 0, 16);
                break;
            }
        }

        ++progressCount;

        // Every 500 times through...
        if (progressCount % 500 == 0) {
            int progressValue = progressCount * 100 / progressTotal;
            if (m_progressDialog)
                m_progressDialog->setValue(progressValue);
            emit progress(progressValue);
            // Kick the event loop to keep the UI responsive.
            qApp->processEvents();
        }
    }

    // Now we write the track to the file.

    *midiFile << MIDI_TRACK_HEADER;
    writeLong(midiFile, trackBuffer.length());
    *midiFile << trackBuffer;
}

bool
MidiFile::write(const QString &filename)
{
    std::ofstream *midiFile =
        new std::ofstream(filename.toLocal8Bit(),
                          std::ios::out | std::ios::binary);

    if (!(*midiFile)) {
        RG_WARNING << "write() - can't write file";
        m_format = MIDI_FILE_NOT_LOADED;
        return false;
    }

    writeHeader(midiFile);

    // For each track, write it out.
    for (TrackId i = 0; i < m_numberOfTracks; i++ )
        writeTrack(midiFile, i);

    midiFile->close();

    return true;
}

void
MidiFile::consolidateNoteEvents(TrackId trackId)
{
    MidiTrack &track = m_midiComposition[trackId];

    // For each MIDI event on the track.
    for (MidiTrack::iterator firstEventIter = track.begin();
         firstEventIter != track.end();
         ++firstEventIter) {
        MidiEvent &firstEvent = **firstEventIter;

        // Not a note-on?  Try the next event.
        if (firstEvent.getMessageType() != MIDI_NOTE_ON)
            continue;

        // Note-on with velocity 0 is a note-off.  Try the next event.
        if (firstEvent.getVelocity() == 0)
            continue;

        // At this point, firstEvent is a note-on event.
        // Search for the matching note-off event.

        bool noteOffFound = false;

        MidiTrack::iterator secondEventIter;

        // For each following MIDI event
        for (secondEventIter = firstEventIter + 1;
             secondEventIter != track.end();
             ++secondEventIter) {
            const MidiEvent &secondEvent = **secondEventIter;

            bool noteOff = (secondEvent.getMessageType() == MIDI_NOTE_OFF  ||
                    (secondEvent.getMessageType() == MIDI_NOTE_ON  &&
                     secondEvent.getVelocity() == 0x00));

            // Not a note-off?  Try the next event.
            if (!noteOff)
                continue;

            // Wrong pitch?  Try the next event.
            if (secondEvent.getPitch() != firstEvent.getPitch())
                continue;

            // Wrong channel?  Try the next event.
            // Note: Since all the events in a track are for a single channel,
            //       this will never be true.
            if (secondEvent.getChannelNumber() != firstEvent.getChannelNumber())
                continue;

            timeT noteDuration = secondEvent.getTime() - firstEvent.getTime();

            // Some MIDI files floating around in the real world
            // apparently have note-on followed immediately by note-off
            // on percussion tracks.  Instead of setting the duration to
            // 0 in this case, which has no meaning, set it to 1.
            if (noteDuration == 0) {
                RG_WARNING << "consolidateNoteEvents() - detected MIDI note duration of 0.  Using duration of 1.  Touch wood.";
                noteDuration = 1;
            }

            firstEvent.setDuration(noteDuration);

            // Remove the note-off.
            delete *secondEventIter;
            track.erase(secondEventIter);

            noteOffFound = true;
            break;
        }

        if (!noteOffFound) {
            // avoid crash due to secondEventIter == track.end()
            --secondEventIter;
            // Set Event duration to length of Segment.
            firstEvent.setDuration(
                    (*secondEventIter)->getTime() - firstEvent.getTime());
        }
    }
}

void
MidiFile::clearMidiComposition()
{
    // For each track
    for (MidiComposition::iterator trackIter = m_midiComposition.begin();
         trackIter != m_midiComposition.end();
         ++trackIter) {

        MidiTrack &midiTrack = trackIter->second;

        // For each event on the track.
        for (MidiTrack::iterator eventIter = midiTrack.begin();
             eventIter != midiTrack.end();
             ++eventIter) {

            delete *eventIter;
        }

        midiTrack.clear();
    }

    m_midiComposition.clear();
    m_trackChannelMap.clear();
    m_trackNames.clear();
}


}

