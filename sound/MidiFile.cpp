// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-

/*
  Rosegarden-4
  A sequencer and musical notation editor.

  This program is Copyright 2000-2003
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
#include <fstream>
#include <string>
#include <cstdio>

#include "Midi.h"
#include "MidiFile.h"
#include "Segment.h"
#include "NotationTypes.h"
#include "BaseProperties.h"
#include "SegmentNotationHelper.h"
#include "SegmentPerformanceHelper.h"
#include "CompositionTimeSliceAdapter.h"
#include "AnalysisTypes.h"
#include "Track.h"
#include "Instrument.h"
#include "Quantizer.h"
#include "Studio.h"
#include "MidiTypes.h"
#include "Profiler.h"

//#define MIDI_DEBUG 1

#if (__GNUC__ < 3)
#include <strstream>
#define stringstream strstream
#else
#include <sstream>
#endif

#include <kapp.h>

namespace Rosegarden
{

using std::string;
using std::ifstream;
using std::stringstream;
using std::cerr;
using std::cerr;
using std::endl;
using std::ends;
using std::ios;

MidiFile::MidiFile(Studio *studio):
                     SoundFile(std::string("unnamed.mid")),
                     m_timingDivision(0),
                     m_format(MIDI_FILE_NOT_LOADED),
                     m_numberOfTracks(0),
		     m_containsTimeChanges(false),
                     m_trackByteCount(0),
                     m_decrementCount(false),
                     m_studio(studio)
{
}

MidiFile::MidiFile(const std::string &fn,
                   Studio *studio):
                     SoundFile(fn),
                     m_timingDivision(0),
                     m_format(MIDI_FILE_NOT_LOADED),
                     m_numberOfTracks(0),
		     m_containsTimeChanges(false),
                     m_trackByteCount(0),
                     m_decrementCount(false),
                     m_studio(studio)
{
}

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
	throw (Exception("Wrong length for long data in MIDI stream"));
    }

    long longRet = ((long)(((MidiByte)bytes[0]) << 24)) |
                   ((long)(((MidiByte)bytes[1]) << 16)) |
                   ((long)(((MidiByte)bytes[2]) << 8)) |
                   ((long)((MidiByte)(bytes[3])));

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
	throw (Exception("Wrong length for int data in MIDI stream"));
    }

    int intRet = ((int)(((MidiByte)bytes[0]) << 8)) |
                 ((int)(((MidiByte)bytes[1])));
    return(intRet);
}



// Our MIDI file accessor function and best regarded as single point of entry
// despite the ifstream pointer flying around the place.  Gets a specified
// number of bytes from the MIDI byte stream.  For each track section we
// can read only a specified number of bytes held in m_trackByteCount.
//
//
string
MidiFile::getMidiBytes(ifstream* midiFile, unsigned long numberOfBytes)
{
    string stringRet;
    char fileMidiByte;
    static int bytesGot = 0; // purely for progress reporting purposes

    if (m_decrementCount && (numberOfBytes > (unsigned long)m_trackByteCount))
    {
#ifdef MIDI_DEBUG
        std::cerr << "Attempt to get more bytes than allowed on Track - ( "
             << numberOfBytes
             << " > "
             << m_trackByteCount
             << " )" << endl;
#endif

	//!!! Investigate -- I'm seeing this on new-notation-quantization
	// branch: load glazunov.rg, run Interpret on first segment, export
	// and attempt to import again
	
        throw(Exception("Attempt to get more bytes than allowed on Track"));
    }

    if (midiFile->eof())
    {
#ifdef MIDI_DEBUG
        std::cerr << "MIDI file EOF - got "
             << stringRet.length() << " bytes out of "
             << numberOfBytes << endl;
#endif

        throw(Exception("MIDI EOF encountered while reading"));

    }

    while(stringRet.length() < numberOfBytes &&
           midiFile->read(&fileMidiByte, 1))
    {
        stringRet += fileMidiByte;
    }

    // if we've reached the end of file without fulfilling the
    // quota then panic as our parsing has performed incorrectly
    //
    if (stringRet.length() < numberOfBytes)
    {
        stringRet = "";
#ifdef MIDI_DEBUG
        cerr << "Attempt to read past file end - got "
             << stringRet.length() << " bytes out of "
             << numberOfBytes << endl;
#endif

        throw(Exception("Attempt to read past MIDI file end"));

    }

    // decrement the byte count
    if (m_decrementCount)
        m_trackByteCount -= stringRet.length();

    // update a progress dialog if we have one
    //
    bytesGot += numberOfBytes;
    if (bytesGot > 1000) {

        emit setProgress((int)(double(midiFile->tellg())/
			       double(m_fileSize) * 20.0));
        kapp->processEvents(50);

        bytesGot = 0;
    }

    return stringRet;
}


// Get a long number of variable length from the MIDI byte stream.
//
//
long
MidiFile::getNumberFromMidiBytes(ifstream* midiFile)
{
    long longRet = 0;
    MidiByte midiByte;

    if(!midiFile->eof())
    {
        midiByte = getMidiBytes(midiFile, 1)[0];

        longRet = midiByte;
        if(midiByte & 0x80 )
        {
            longRet &= 0x7F;
            do
            {
                midiByte = getMidiBytes(midiFile, 1)[0];

                longRet = (longRet << 7) + (midiByte & 0x7F);
            }
            while (!midiFile->eof() && (midiByte & 0x80));
        }
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
    m_trackByteCount = 0;
    m_decrementCount = false;

    while(!midiFile->eof() && (m_decrementCount == false ))
    {
        buffer = getMidiBytes(midiFile, 4); 

#if (__GNUC__ < 3)
        if (buffer.compare(MIDI_TRACK_HEADER, 0, 4) == 0)
#else
            if (buffer.compare(0, 4, MIDI_TRACK_HEADER) == 0)
#endif

            {
                m_trackByteCount = midiBytesToLong(getMidiBytes(midiFile, 4));
                m_decrementCount = true;
            }

    }

    if ( m_trackByteCount == 0 ) // we haven't found a track
        return(false);
    else
        return(true);
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
    std::cerr << "MidiFile::open() : fileName = " << m_fileName.c_str() << endl;
#endif

    // Open the file
    ifstream *midiFile = new ifstream(m_fileName.c_str(), ios::in | ios::binary);

    try
    {
        if (*midiFile)
        {

            // Set file size so we can count it off
            //
            midiFile->seekg(0, std::ios::end);
            m_fileSize = midiFile->tellg();
            midiFile->seekg(0, std::ios::beg);

            // Parse the MIDI header first.  The first 14 bytes of the file.
            if (!parseHeader(getMidiBytes(midiFile, 14)))
            {
                m_format = MIDI_FILE_NOT_LOADED;
		m_error = "Not a MIDI file.";
                return(false);
            }

	    m_containsTimeChanges = false;

	    TrackId i = 0;

            for (unsigned int j = 0; j < m_numberOfTracks; ++j)
            {

#ifdef MIDI_DEBUG
                std::std::cerr << "Parsing Track " << j << endl;
#endif

                if(!skipToNextTrack(midiFile))
                {
#ifdef MIDI_DEBUG
                    cerr << "Couldn't find Track " << j << endl;
#endif
		    m_error = "File corrupted or in non-standard format?";
                    m_format = MIDI_FILE_NOT_LOADED;
                    return(false);
                }

#ifdef MIDI_DEBUG
		std::std::cerr << "Track has " << m_trackByteCount << " bytes" << std::endl;
#endif

                // Run through the events taking them into our internal
                // representation.
                if (!parseTrack(midiFile, i))
                {
#ifdef MIDI_DEBUG
                    std::cerr << "Track " << j << " parsing failed" << endl;
#endif
		    m_error = "File corrupted or in non-standard format?";
                    m_format = MIDI_FILE_NOT_LOADED;
                    return(false);
                }

		++i; // j is the source track number, i the destination
            }

	    m_numberOfTracks = i;
        }
        else
        {
	    m_error = "File not found or not readable.";
            m_format = MIDI_FILE_NOT_LOADED;
            return(false);
        }

        // Close the file now
        midiFile->close();
    }
    catch(Exception e)
    {
#ifdef MIDI_DEBUG
        std::cerr << "MidiFile::open() - caught exception - "
	     << e.getMessage() << endl;
#endif
	m_error = e.getMessage();
        retOK = false;
    }

    return(retOK);
}

// Parse and ensure the MIDI Header is legitimate
// 
//
bool
MidiFile::parseHeader(const string &midiHeader)
{
    if (midiHeader.size() < 14)
    {
#ifdef MIDI_DEBUG
        std::cerr << "MidiFile::parseHeader() - file header undersized" << endl;
#endif
        return(false);
    }

#if (__GNUC__ < 3)
    if (midiHeader.compare(MIDI_FILE_HEADER, 0, 4) != 0)
#else
        if (midiHeader.compare(0, 4, MIDI_FILE_HEADER) != 0)
#endif

        {
#ifdef MIDI_DEBUG
            std::cerr << "MidiFile::parseHeader()"
                      << "- file header not found or malformed"
                      << endl;
#endif
            return(false);
        }

    if (midiBytesToLong(midiHeader.substr(4,4)) != 6L)
    {
#ifdef MIDI_DEBUG
        std::cerr << "MidiFile::parseHeader()"
                  << " - header length incorrect"
                  << endl;
#endif
        return(false);
    }

    m_format = (MIDIFileFormatType) midiBytesToInt(midiHeader.substr(8,2));
    m_numberOfTracks = midiBytesToInt(midiHeader.substr(10,2));
    m_timingDivision = midiBytesToInt(midiHeader.substr(12,2));

    if ( m_format == MIDI_SEQUENTIAL_TRACK_FILE )
    {
#ifdef MIDI_DEBUG
        std::cerr << "MidiFile::parseHeader()"
                  << "- can't load sequential track file"
                  << endl;
#endif
        return(false);
    }
  

#ifdef MIDI_DEBUG
    if ( m_timingDivision < 0 )
    {
        std::cerr << "MidiFile::parseHeader()"
                  << " - file uses SMPTE timing"
                  << endl;
    }
#endif

    return(true); 
}



// Extract the contents from a MIDI file track and places it into
// our local map of MIDI events.
//
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

    bool firstTrack = true;

    while (!midiFile->eof() && ( m_trackByteCount > 0 ) )
    {
	if (eventCode < 0x80) {
#ifdef MIDI_DEBUG
	    cerr << "WARNING: Invalid event code " << eventCode
		 << " in MIDI file" << endl;
#endif
	    throw (Exception("Invalid event code found"));
	}

        deltaTime = getNumberFromMidiBytes(midiFile);

        // Get a single byte
        midiByte = (MidiByte) getMidiBytes(midiFile, 1)[0];

        if (!(midiByte & MIDI_STATUS_BYTE_MASK))
        {
            midiFile->seekg(-1, ios::cur);
            m_trackByteCount++;
        }
        else
            eventCode = midiByte;

        if (eventCode == MIDI_FILE_META_EVENT) // meta events
        { 
            metaEventCode = getMidiBytes(midiFile,1)[0];
            messageLength = getNumberFromMidiBytes(midiFile);
            metaMessage = getMidiBytes(midiFile, messageLength);

	    if (metaEventCode == MIDI_TIME_SIGNATURE ||
		metaEventCode == MIDI_SET_TEMPO) {
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
        }
        else // the rest
        {
            MidiEvent *midiEvent;

	    int channel = (eventCode & MIDI_CHANNEL_NUM_MASK);
	    if (channelTrackMap[channel] == -1) {
		if (!firstTrack) ++lastTrackNum;
		else firstTrack = false;
		channelTrackMap[channel] = lastTrackNum;
	    }

	    TrackId trackNum = channelTrackMap[channel];
	    
	    // accumulatedTime is abs time of last event on any track;
	    // trackTimeMap[trackNum] is that of last event on this track
	    
	    long gap = accumulatedTime - trackTimeMap[trackNum];
	    accumulatedTime += deltaTime;
	    deltaTime += gap;
	    trackTimeMap[trackNum] = accumulatedTime;

            switch (eventCode & MIDI_MESSAGE_TYPE_MASK)
            {
            case MIDI_NOTE_ON:
            case MIDI_NOTE_OFF:
            case MIDI_POLY_AFTERTOUCH:
            case MIDI_CTRL_CHANGE:

                data1 = (MidiByte) getMidiBytes(midiFile, 1)[0];
                data2 = (MidiByte) getMidiBytes(midiFile, 1)[0];

                // create and store our event
                midiEvent = new MidiEvent(deltaTime, eventCode, data1, data2);

                /*
		std::std::cerr << "MIDI event for channel " << channel << " (track "
			  << trackNum << ")" << std::endl;
		midiEvent->print();
                          */


                m_midiComposition[trackNum].push_back(midiEvent);
                break;

            case MIDI_PITCH_BEND:

                data1 = (MidiByte) getMidiBytes(midiFile, 1)[0];
                data2 = (MidiByte) getMidiBytes(midiFile, 1)[0];

                // create and store our event
                midiEvent = new MidiEvent(deltaTime, eventCode, data1, data2);
                m_midiComposition[trackNum].push_back(midiEvent);
                break;

            case MIDI_PROG_CHANGE:
            case MIDI_CHNL_AFTERTOUCH:
                data1 = (MidiByte) getMidiBytes(midiFile, 1)[0];

                // create and store our event
                midiEvent = new MidiEvent(deltaTime, eventCode, data1);
                m_midiComposition[trackNum].push_back(midiEvent);
                break;

            case MIDI_SYSTEM_EXCLUSIVE:
                messageLength = getNumberFromMidiBytes(midiFile);
                metaMessage= getMidiBytes(midiFile, messageLength);

                if (MidiByte(metaMessage[metaMessage.length() - 1]) !=
                        MIDI_END_OF_EXCLUSIVE)
                {
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
                metaMessage = metaMessage.substr(0, metaMessage.length()-1);

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

    return(true);
}


// If we wanted to abstract the MidiFile class to make it more useful to
// other applications (and formats) we'd make this method and its twin
// pure virtual.
//
bool
MidiFile::convertToRosegarden(Composition &composition, ConversionType type)
{
    Rosegarden::Profiler profiler("MidiFile::convertToRosegarden");

    MidiTrackIterator midiEvent;
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
    int numerator;
    int denominator;
    timeT segmentTime;

    // keys
    int accidentals;
    bool isMinor;
    bool isSharp;

    if (type == CONVERT_REPLACE) composition.clear();

    timeT origin = 0;
    if (type == CONVERT_APPEND && composition.getDuration() > 0) {
	origin = composition.getBarEndForTime(composition.getDuration());
    }

    TrackId compTrack = 0;
    for (Composition::iterator ci = composition.begin();
	 ci != composition.end(); ++ci) {
	if ((*ci)->getTrack() >= compTrack) compTrack = (*ci)->getTrack() + 1;
    }

    Track *track = 0;

    // precalculate the timing factor
    //
    // [cc] -- attempt to avoid floating-point rounding errors
    timeT crotchetTime = Note(Note::Crotchet).getDuration();
    int divisor = m_timingDivision ? m_timingDivision : 96;
    bool haveTimeSignatures = false;

    InstrumentId compInstrument = MidiInstrumentBase;

    // Clear down the assigned Instruments we already have
    //
    m_studio->unassignAllInstruments();

#ifdef MIDI_DEBUG
    std::cerr << "NUMBER OF TRACKS = " << m_numberOfTracks << endl;
    std::cerr << "MIDI COMP SIZE = " << m_midiComposition.size() << endl;
#endif

    for (TrackId i = 0; i < m_numberOfTracks; i++ )
    {
        segmentTime = 0;
        trackName = string("Imported MIDI");

        // progress - 20% total in file import itself and then 80%
        // split over these tracks
        emit setProgress(20 +
                         (int)((80.0 * double(i)/double(m_numberOfTracks))));
        kapp->processEvents(50);

        // Convert the deltaTime to an absolute time since
        // the start of the segment.  The addTime method 
        // returns the sum of the current Midi Event delta
        // time plus the argument.
        //
        for (midiEvent = m_midiComposition[i].begin();
             midiEvent != m_midiComposition[i].end();
             ++midiEvent)
        {
            segmentTime = (*midiEvent)->addTime(segmentTime);
        }

        // Consolidate NOTE ON and NOTE OFF events into a NOTE ON with
        // a duration. 
        //
        if (consolidateNoteOffEvents(i)) // returns true if some notes exist
        {
            // If we're true then rotate the instrument number
            compInstrument = MidiInstrumentBase + (compTrack % 16);
        }
        else
            compInstrument = MidiInstrumentBase;

        rosegardenSegment = new Segment;
        rosegardenSegment->setTrack(compTrack);
        rosegardenSegment->setStartTime(0);

	track = new Track(compTrack,        // id
			  compInstrument,   // instrument
			  compTrack,        // position
			  trackName,        // name
			  false);           // muted

        // rest creation token needs to be reset here
        //
        endOfLastNote = 0;

	int msb = -1, lsb = -1; // for bank selects
	Instrument *instrument = 0;

        for (midiEvent = m_midiComposition[i].begin();
             midiEvent != m_midiComposition[i].end();
             midiEvent++)
        {
            // [cc] -- avoid floating-point
            rosegardenTime = origin +
                timeT(((*midiEvent)->getTime() * crotchetTime) / divisor);
            rosegardenDuration =
                timeT(((*midiEvent)->getDuration() * crotchetTime) / divisor);
	    if (rosegardenTime + rosegardenDuration > maxTime) {
		maxTime = rosegardenTime + rosegardenDuration;
	    }

            if ((*midiEvent)->isMeta())
            {
                switch((*midiEvent)->getMetaEventCode())
                {
                case MIDI_TEXT_EVENT:
                    {
                        std::string text = (*midiEvent)->getMetaMessage();
                        rosegardenEvent =
                            Text(text).getAsEvent(rosegardenTime);
			rosegardenSegment->insert(rosegardenEvent);
                    }
                    break;

                case MIDI_LYRIC:
                    {
                        std::string text = (*midiEvent)->getMetaMessage();
                        rosegardenEvent =
                            Text(text, Text::Lyric).
			    getAsEvent(rosegardenTime);
			rosegardenSegment->insert(rosegardenEvent);
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

                case MIDI_END_OF_TRACK:
		    if (endOfLastNote < rosegardenTime) {

			// If there's nothing in the segment yet, then we
			// shouldn't fill with rests because we don't want
			// to cause the otherwise empty segment to be created
			if (rosegardenSegment->size() > 0) {
			    rosegardenSegment->fillWithRests(rosegardenTime);
			}
                    }
                    break;

                case MIDI_SET_TEMPO:
                    {
                        MidiByte m0 = (*midiEvent)->getMetaMessage()[0];
                        MidiByte m1 = (*midiEvent)->getMetaMessage()[1];
                        MidiByte m2 = (*midiEvent)->getMetaMessage()[2];

                        long tempo = (((m0 << 8) + m1) << 8) + m2;

                        if (tempo != 0)
                        {
                            tempo = 60000000 / tempo;
                            composition.addTempo(rosegardenTime, tempo);
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
                    haveTimeSignatures = true;
                    break;

                case MIDI_KEY_SIGNATURE:
                    // get the details
                    accidentals = (int) (*midiEvent)->getMetaMessage()[0];
                    isMinor     = (int) (*midiEvent)->getMetaMessage()[1];
                    isSharp     = accidentals < 0 ?        false  :  true;
                    accidentals = accidentals < 0 ?  -accidentals :  accidentals;
                    // create and insert the key event
                    //
                    try
                    {
                        rosegardenEvent = Rosegarden::Key
                            (accidentals, isSharp, isMinor).
                                getAsEvent(rosegardenTime);
                    }

                    catch(...)
                    {
#ifdef MIDI_DEBUG
                        std::cerr << "MidiFile::convertToRosegarden - "
                                  << " badly formed key signature"
                                  << std::endl;
#endif
                        break;
                    }
                    rosegardenSegment->insert(rosegardenEvent);
                    break;

                case MIDI_SEQUENCE_NUMBER:
                case MIDI_CHANNEL_PREFIX_OR_PORT:
                case MIDI_INSTRUMENT_NAME:
                case MIDI_TEXT_MARKER:
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

            }
            else switch((*midiEvent)->getMessageType())
            {
            case MIDI_NOTE_ON:

                // A zero velocity here is a virtual "NOTE OFF"
                // so we ignore this event
                //
                if ((*midiEvent)->getVelocity() == 0)
                    break;

                endOfLastNote = rosegardenTime + rosegardenDuration;

		//std::cerr << "MidiFile::convertToRosegarden: note at " << rosegardenTime << ", midi time " << (*midiEvent)->getTime() << std::endl;

                // create and populate event
                rosegardenEvent = new Event(Note::EventType,
                                            rosegardenTime,
                                            rosegardenDuration);
                rosegardenEvent->set<Int>(BaseProperties::PITCH,
                                          (*midiEvent)->getPitch());
                rosegardenEvent->set<Int>(BaseProperties::VELOCITY,
                                          (*midiEvent)->getVelocity());
                rosegardenSegment->insert(rosegardenEvent);
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
		if (!instrument) {
		    
		    if (msb >= 0 || lsb >= 0) {
			instrument = m_studio->assignMidiProgramToInstrument
			    ((*midiEvent)->getData1(),
			     (msb >= 0 ? msb : 0),
			     (lsb >= 0 ? lsb : 0),
			     (*midiEvent)->getChannelNumber() == 
			     MIDI_PERCUSSION_CHANNEL);
			msb = -1;
			lsb = -1;
		    } else {
                        instrument = m_studio->assignMidiProgramToInstrument
			    ((*midiEvent)->getData1(),
			     (*midiEvent)->getChannelNumber() == 
			     MIDI_PERCUSSION_CHANNEL);
		    }			
		    
                    // assign it here
                    if (instrument) {
                        
			track->setInstrument(instrument->getId());

                        // give the Segment a name based on the the Instrument
                        //
			rosegardenSegment->setLabel
			    (m_studio->getSegmentName(instrument->getId()));

			if ((*midiEvent)->getTime() == 0) break; // no insert
                    }
                }

		// did we have a bank select? if so, insert that too

		if (msb >= 0) {
		    rosegardenSegment->insert
			(Controller(MIDI_CONTROLLER_BANK_MSB, msb).
			 getAsEvent(rosegardenTime));
		}
		if (lsb >= 0) {
		    rosegardenSegment->insert
			(Controller(MIDI_CONTROLLER_BANK_LSB, msb).
			 getAsEvent(rosegardenTime));
		}

		rosegardenEvent =
		    ProgramChange((*midiEvent)->getData1()).
		    getAsEvent(rosegardenTime);
		rosegardenSegment->insert(rosegardenEvent);
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
			instrument->setVolume((*midiEvent)->getData2());
			break;
		    }
		    if ((*midiEvent)->getData1() == MIDI_CONTROLLER_PAN) {
			instrument->setPan((*midiEvent)->getData2());
			break;
		    }
		    if ((*midiEvent)->getData1() == MIDI_CONTROLLER_ATTACK) {
			instrument->setAttack((*midiEvent)->getData2());
			break;
		    }
		    if ((*midiEvent)->getData1() == MIDI_CONTROLLER_RELEASE) {
			instrument->setRelease((*midiEvent)->getData2());
			break;
		    }
		    if ((*midiEvent)->getData1() == MIDI_CONTROLLER_FILTER) {
			instrument->setFilter((*midiEvent)->getData2());
			break;
		    }
		    if ((*midiEvent)->getData1() == MIDI_CONTROLLER_RESONANCE) {
			instrument->setResonance((*midiEvent)->getData2());
			break;
		    }
		    if ((*midiEvent)->getData1() == MIDI_CONTROLLER_CHORUS) {
			instrument->setChorus((*midiEvent)->getData2());
			break;
		    }
		    if ((*midiEvent)->getData1() == MIDI_CONTROLLER_REVERB) {
			instrument->setReverb((*midiEvent)->getData2());
			break;
		    }
		}

		rosegardenEvent =
		    Controller((*midiEvent)->getData1(),
			       (*midiEvent)->getData2()).
		    getAsEvent(rosegardenTime);
                rosegardenSegment->insert(rosegardenEvent);
                break;

            case MIDI_PITCH_BEND:
                rosegardenEvent =
		    PitchBend((*midiEvent)->getData2(),
			      (*midiEvent)->getData1()).
		    getAsEvent(rosegardenTime);
                rosegardenSegment->insert(rosegardenEvent);
                break;

            case MIDI_SYSTEM_EXCLUSIVE:
                rosegardenEvent = 
		    SystemExclusive((*midiEvent)->getMetaMessage()).
		    getAsEvent(rosegardenTime);
                rosegardenSegment->insert(rosegardenEvent);
                break;

            case MIDI_POLY_AFTERTOUCH:
                rosegardenEvent =
		    KeyPressure((*midiEvent)->getData1(),
				(*midiEvent)->getData2()).
		    getAsEvent(rosegardenTime);
                rosegardenSegment->insert(rosegardenEvent);
                break;

            case MIDI_CHNL_AFTERTOUCH:
                rosegardenEvent =
		    ChannelPressure((*midiEvent)->getData1()).
		    getAsEvent(rosegardenTime);
                rosegardenSegment->insert(rosegardenEvent);
                break;

            default:
#ifdef MIDI_DEBUG
                std::cerr << "MidiFile::convertToRosegarden - "
                          << "Unsupported event code = " 
                          << (int)(*midiEvent)->getMessageType() << std::endl;
#endif
                break;
            }
        }

	if (rosegardenSegment->size() > 0) {

	    // if all we have is key signatures, take this to be a
	    // conductor segment and don't insert it
	    //
	    bool keySigsOnly = true;
	    bool haveKeySig = false;
	    for (Segment::iterator i = rosegardenSegment->begin();
		 i != rosegardenSegment->end(); ++i) {
		if (!(*i)->isa(Rosegarden::Key::EventType)) {
		    keySigsOnly = false;
		    break;
		} else {
		    haveKeySig = true;
		}
	    }

	    if (keySigsOnly) {
		conductorSegment = rosegardenSegment;
		continue;
	    } else if (!haveKeySig && conductorSegment) {
		// copy across any key sigs from the conductor segment
		for (Segment::iterator i = conductorSegment->begin();
		     i != conductorSegment->end(); ++i) {
		    rosegardenSegment->insert(new Event(**i));
		}
		    
	    }

	    // add the Segment to the Composition and increment the
	    // Rosegarden segment number
	    //
	    composition.addTrack(track);
	    composition.addSegment(rosegardenSegment);
	    compTrack++;

	} else {
	    delete rosegardenSegment;
	    rosegardenSegment = 0;
	    delete track;
	    track = 0;
	}
    }

    if (maxTime > composition.getEndMarker()) {
	composition.setEndMarker(composition.getBarEndForTime(maxTime));
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
    MidiEvent *midiEvent;
    int trackNumber = 0;

    timeT midiEventAbsoluteTime;
    MidiByte midiVelocity;
    MidiByte midiChannel = 0;

    // [cc] int rather than floating point
    //
    m_timingDivision = 480; //!!! make this configurable
    timeT crotchetDuration = Note(Note::Crotchet).getDuration();

    // Export as this format only
    //
    m_format = MIDI_SIMULTANEOUS_TRACK_FILE;

    // Clear out the MidiComposition internal store
    //
    clearMidiComposition();

    // Insert the Rosegarden Signature Track here and any relevant
    // file META information - this will get written out just like
    // any other MIDI track.
    //
    midiEvent = new MidiEvent(0, MIDI_FILE_META_EVENT, MIDI_TEXT_MARKER,
                              "Created by Rosegarden");

    m_midiComposition[trackNumber].push_back(midiEvent);

    midiEvent = new MidiEvent(0, MIDI_FILE_META_EVENT, MIDI_TEXT_MARKER,
                              "http://www.all-day-breakfast.com/rosegarden");

    m_midiComposition[trackNumber].push_back(midiEvent);

    // Insert tempo events
    //
    for (int i = 0; i < comp.getTempoChangeCount(); i++)
    {
        std::pair<timeT, long> tempo = comp.getRawTempoChange(i);

        midiEventAbsoluteTime = tempo.first * m_timingDivision
                                 / crotchetDuration;

        long tempoValue = (long) ( tempo.second / 60.0 );
        tempoValue = 60000000 / tempoValue;

        string tempoString;
        tempoString += (MidiByte) ( tempoValue >> 16 & 0xFF );
        tempoString += (MidiByte) ( tempoValue >> 8  & 0xFF );
        tempoString += (MidiByte) ( tempoValue & 0xFF );

        midiEvent = new MidiEvent(midiEventAbsoluteTime,
                                  MIDI_FILE_META_EVENT,
                                  MIDI_SET_TEMPO,
                                  tempoString);

        m_midiComposition[trackNumber].push_back(midiEvent);
    }

    // Insert time signatures (don't worry that the times might be out
    // of order with those of the tempo events -- we sort the track later)
    //
    for (int i = 0; i < comp.getTimeSignatureCount(); i++)
    {
        std::pair<timeT, TimeSignature> timeSig =
                comp.getTimeSignatureChange(i);
        
        midiEventAbsoluteTime = timeSig.first * m_timingDivision
                                 / crotchetDuration;

        string timeSigString;
        timeSigString += (MidiByte) (timeSig.second.getNumerator());
        int denominator = timeSig.second.getDenominator();
        int denPowerOf2 = 0;

        // Work out how many powers of two are in the denominator
        //
        while (denominator >>= 1)
            denPowerOf2++;

        timeSigString += (MidiByte) denPowerOf2;

	// The third byte is the number of MIDI clocks per beat.
	// There are 24 clocks per quarter-note (the MIDI clock
	// is tempo-independent and is not related to the timebase).
	//
	int cpb = 24 * timeSig.second.getBeatDuration() / crotchetDuration;
	timeSigString += (MidiByte) cpb;

	// And the fourth byte is always 8, for us (it expresses
	// the number of notated 32nd-notes in a MIDI quarter-note,
	// for applications that may want to notate and perform
	// in different units)
	//
	timeSigString += (MidiByte) 8;
	
        midiEvent = new MidiEvent(midiEventAbsoluteTime,
                                  MIDI_FILE_META_EVENT,
                                  MIDI_TIME_SIGNATURE,
                                  timeSigString);

        m_midiComposition[trackNumber].push_back(midiEvent);
    }

    // first track proper
    //
    trackNumber++;

    // In pass one just insert all events including new NOTE OFFs at the right
    // absolute times.
    //
    for (Composition::const_iterator segment = comp.begin();
         segment != comp.end(); ++segment)
    {
        // We use this later to get NOTE durations
        //
        SegmentPerformanceHelper helper(**segment);

        Track *track =
            comp.getTrackById((*segment)->getTrack());

	if (track->isMuted()) continue;

        {
            stringstream trackName;
            // insert a track name

#if (__GNUC__ < 3)
            trackName << "Track " << trackNumber << ends;
#else
            trackName << "Track " << trackNumber;
#endif

            midiEvent = new MidiEvent(0,
                                      MIDI_FILE_META_EVENT,
                                      MIDI_TRACK_NAME,
				      track->getLabel());

            m_midiComposition[trackNumber].push_back(midiEvent);
        }

        // Get the Instrument
        //
        Instrument *instr =
            m_studio->getInstrumentById(track->getInstrument());

        MidiByte program = 0;
        midiChannel = 0;

	bool useBank = false;
	MidiByte lsb = 0;
	MidiByte msb = 0;

        if (instr) {
            midiChannel = instr->getMidiChannel();
            program = instr->getProgramChange();
	    if (instr->sendsBankSelect()) {
		lsb = instr->getLSB();
		msb = instr->getMSB();
		useBank = true;
	    }
        }

	if (useBank) {
	    // insert a bank select

	    if (msb != 0) {
		midiEvent = new MidiEvent(0,
					  MIDI_CTRL_CHANGE | midiChannel,
					  MIDI_CONTROLLER_BANK_MSB,
					  msb);
		m_midiComposition[trackNumber].push_back(midiEvent);
	    }

	    if (lsb != 0) {
		midiEvent = new MidiEvent(0,
					  MIDI_CTRL_CHANGE | midiChannel,
					  MIDI_CONTROLLER_BANK_LSB,
					  lsb);
		m_midiComposition[trackNumber].push_back(midiEvent);
	    }
	}
 
        // insert a program change
        midiEvent = new MidiEvent(0, // time
                                 MIDI_PROG_CHANGE | midiChannel,
                                 program);
        m_midiComposition[trackNumber].push_back(midiEvent);

	if (instr) {
	    // MidiInstrument parameters: volume, pan, attack,
	    // release, filter, resonance, chorus, reverb.  Always
	    // write these: the Instrument has an additional parameter
	    // to record whether they should be sent, but it isn't
	    // actually set anywhere so we have to ignore it.

	    m_midiComposition[trackNumber].push_back
		(new MidiEvent(0, MIDI_CTRL_CHANGE | midiChannel,
			       MIDI_CONTROLLER_VOLUME, instr->getVolume()));

	    m_midiComposition[trackNumber].push_back
		(new MidiEvent(0, MIDI_CTRL_CHANGE | midiChannel,
			       MIDI_CONTROLLER_PAN, instr->getPan()));

	    m_midiComposition[trackNumber].push_back
		(new MidiEvent(0, MIDI_CTRL_CHANGE | midiChannel,
			       MIDI_CONTROLLER_ATTACK, instr->getAttack()));

	    m_midiComposition[trackNumber].push_back
		(new MidiEvent(0, MIDI_CTRL_CHANGE | midiChannel,
			       MIDI_CONTROLLER_RELEASE, instr->getRelease()));

	    m_midiComposition[trackNumber].push_back
		(new MidiEvent(0, MIDI_CTRL_CHANGE | midiChannel,
			       MIDI_CONTROLLER_FILTER, instr->getFilter()));

	    m_midiComposition[trackNumber].push_back
		(new MidiEvent(0, MIDI_CTRL_CHANGE | midiChannel,
			       MIDI_CONTROLLER_RESONANCE, instr->getResonance()));

	    m_midiComposition[trackNumber].push_back
		(new MidiEvent(0, MIDI_CTRL_CHANGE | midiChannel,
			       MIDI_CONTROLLER_CHORUS, instr->getChorus()));

	    m_midiComposition[trackNumber].push_back
		(new MidiEvent(0, MIDI_CTRL_CHANGE | midiChannel,
			       MIDI_CONTROLLER_REVERB, instr->getReverb()));
	}

	timeT segmentMidiDuration =
	    ((*segment)->getEndMarkerTime() -
	     (*segment)->getStartTime()) * m_timingDivision /
	    crotchetDuration;

        for (Segment::iterator el = (*segment)->begin();
             (*segment)->isBeforeEndMarker(el); ++el)
        {
            midiEventAbsoluteTime =
		(*el)->getAbsoluteTime() + (*segment)->getDelay();

	    timeT absoluteTimeLimit = midiEventAbsoluteTime;
	    if ((*segment)->isRepeating()) {
		absoluteTimeLimit = ((*segment)->getRepeatEndTime() - 1) +
		    (*segment)->getDelay();
	    }

	    if ((*segment)->getRealTimeDelay() != RealTime::zeroTime) {
		RealTime evRT = comp.getElapsedRealTime(midiEventAbsoluteTime);
		timeT timeBeforeDelay = midiEventAbsoluteTime;
		midiEventAbsoluteTime = comp.getElapsedTimeForRealTime
		    (evRT + (*segment)->getRealTimeDelay());
		absoluteTimeLimit += (midiEventAbsoluteTime - timeBeforeDelay);
	    }

	    midiEventAbsoluteTime =
		midiEventAbsoluteTime * m_timingDivision / crotchetDuration;
	    absoluteTimeLimit =
		absoluteTimeLimit * m_timingDivision / crotchetDuration;

	    while (midiEventAbsoluteTime <= absoluteTimeLimit) {

		try {
		    
		    if ((*el)->isa(Note::EventType))
		    {
			if ((*el)->has(BaseProperties::VELOCITY))
			    midiVelocity = (*el)->get<Int>(BaseProperties::VELOCITY);
			else
			    midiVelocity = 127;

			int pitch = (*el)->get<Int>(BaseProperties::PITCH);
			pitch += (*segment)->getTranspose();
                              
			// insert the NOTE_ON at the appropriate channel
			//
			midiEvent = 
			    new MidiEvent(midiEventAbsoluteTime,
					  MIDI_NOTE_ON | midiChannel,
					  pitch,
					  midiVelocity);

			m_midiComposition[trackNumber].push_back(midiEvent);

			// Get the sounding time for the matching NOTE_OFF.
			// We use SegmentPerformanceHelper::getSoundingDuration()
			// to work out the tied duration of the NOTE.
			//
			// [cc] avoiding floating-point
			timeT midiEventEndTime = midiEventAbsoluteTime +
			    helper.getSoundingDuration(el) * m_timingDivision /
			    crotchetDuration;

			// insert the matching NOTE OFF
			//
			midiEvent =
			    new MidiEvent(midiEventEndTime,
					  MIDI_NOTE_OFF | midiChannel,
					  pitch,
					  127); // full volume silence

			m_midiComposition[trackNumber].push_back(midiEvent);

		    }
		    else if ((*el)->isa(PitchBend::EventType))
		    {
			PitchBend pb(**el);
			midiEvent =
			    new MidiEvent(midiEventAbsoluteTime,
					  MIDI_PITCH_BEND | midiChannel,
					  pb.getLSB(), pb.getMSB());

			m_midiComposition[trackNumber].push_back(midiEvent);
		    }
		    else if ((*el)->isa(Rosegarden::Key::EventType))
		    {
			Rosegarden::Key key(**el);

			int accidentals = key.getAccidentalCount();
			if (!key.isSharp())
			    accidentals = -accidentals;

			// stack out onto the meta string
			//
			std::string metaMessage;
			metaMessage += MidiByte(accidentals);
			metaMessage += MidiByte(key.isMinor());

			midiEvent =
			    new MidiEvent(midiEventAbsoluteTime,
					  MIDI_FILE_META_EVENT,
					  MIDI_KEY_SIGNATURE,
					  metaMessage);

			//m_midiComposition[trackNumber].push_back(midiEvent);

		    }
		    else if ((*el)->isa(Controller::EventType))
		    {
			Controller c(**el);
			midiEvent =
			    new MidiEvent(midiEventAbsoluteTime,
					  MIDI_CTRL_CHANGE | midiChannel,
					  c.getNumber(), c.getValue());

			m_midiComposition[trackNumber].push_back(midiEvent);
		    }
		    else if ((*el)->isa(ProgramChange::EventType))
		    {
			ProgramChange pc(**el);
			midiEvent =
			    new MidiEvent(midiEventAbsoluteTime,
					  MIDI_PROG_CHANGE | midiChannel,
					  pc.getProgram());

			m_midiComposition[trackNumber].push_back(midiEvent);
		    }
		    else if ((*el)->isa(SystemExclusive::EventType))
		    {
			SystemExclusive s(**el);
			std::string data = s.getRawData();

			// check for closing EOX and add one if none found
			//
			if (MidiByte(data[data.length() - 1]) != MIDI_END_OF_EXCLUSIVE)
			{
			    data += MIDI_END_OF_EXCLUSIVE;
			}

			// construct plain SYSEX event
			//
			midiEvent = new MidiEvent(midiEventAbsoluteTime,
						  MIDI_SYSTEM_EXCLUSIVE,
						  data);

			m_midiComposition[trackNumber].push_back(midiEvent);

		    }
		    else if ((*el)->isa(ChannelPressure::EventType))
		    {
			ChannelPressure cp(**el);
			midiEvent =
			    new MidiEvent(midiEventAbsoluteTime,
					  MIDI_CHNL_AFTERTOUCH | midiChannel,
					  cp.getPressure());

			m_midiComposition[trackNumber].push_back(midiEvent);
		    }
		    else if ((*el)->isa(KeyPressure::EventType))
		    {
			KeyPressure kp(**el);
			midiEvent =
			    new MidiEvent(midiEventAbsoluteTime,
					  MIDI_POLY_AFTERTOUCH | midiChannel,
					  kp.getPitch(), kp.getPressure());

			m_midiComposition[trackNumber].push_back(midiEvent);
		    }
		    else if ((*el)->isa(Text::EventType))
		    {
			Text text(**el);
			std::string metaMessage = text.getText();

			MidiByte midiTextType = MIDI_TEXT_EVENT;

			if (text.getTextType() == Text::Lyric) {
			    midiTextType = MIDI_LYRIC;
			}

			if (text.getTextType() != Text::Annotation) {
			    // (we don't write annotations)

			    midiEvent =
				new MidiEvent(midiEventAbsoluteTime,
					      MIDI_FILE_META_EVENT,
					      midiTextType,
					      metaMessage);
			
			    m_midiComposition[trackNumber].push_back(midiEvent);
			}
		    }
		    else if ((*el)->isa(Note::EventRestType))
		    {
			// skip legitimately
		    }
		    else
		    {  
/*
  cerr << "MidiFile::convertToMidi - "
  << "unsupported MidiType \""
  << (*el)->getType()
  << "\" at export"
  << std::endl;
*/
		    }

		} catch (MIDIValueOutOfRange r) {
#ifdef MIDI_DEBUG
		    std::cerr << "MIDI value out of range at "
			      << (*el)->getAbsoluteTime() << std::endl;
#endif
		} catch (Event::NoData d) {
#ifdef MIDI_DEBUG
		    std::cerr << "Caught Event::NoData at "
			      << (*el)->getAbsoluteTime() << ", message is:"
			      << std::endl << d.getMessage() << std::endl;
#endif
		} catch (Event::BadType b) {
#ifdef MIDI_DEBUG
		    std::cerr << "Caught Event::BadType at "
			      << (*el)->getAbsoluteTime() << ", message is:"
			      << std::endl << b.getMessage() << std::endl;
#endif
		} catch (SystemExclusive::BadEncoding e) {
#ifdef MIDI_DEBUG
		    std::cerr << "Caught bad SysEx encoding at "
			      << (*el)->getAbsoluteTime() << std::endl;
#endif
		}

		if (segmentMidiDuration > 0) {
		    midiEventAbsoluteTime += segmentMidiDuration;
		} else break;
	    }
        }

        trackNumber++;
    }

    // Setup number of tracks in the daddy object
    //
    m_numberOfTracks = trackNumber;


    // Now gnash through the MIDI events and turn the absolute times
    // into delta times.
    //
    //
    MidiTrackIterator it;
    timeT deltaTime, lastMidiTime;

    for (TrackId i = 0; i < m_numberOfTracks; i++)
    {
        lastMidiTime = 0;

        // First sort the track with the MidiEvent comparator.  Use
        // stable_sort so that events with equal times are maintained
        // in their current order (important for e.g. bank-program
        // pairs, or the controllers at the start of the track which
        // should follow the program so we can treat them correctly
        // when re-reading).
        //
        std::stable_sort(m_midiComposition[i].begin(),
			 m_midiComposition[i].end(),
			 MidiEventCmp());

        for (it = m_midiComposition[i].begin();
             it != m_midiComposition[i].end();
             it++)
        {
            deltaTime = (*it)->getTime() - lastMidiTime;
            lastMidiTime = (*it)->getTime();
            (*it)->setTime(deltaTime);
        }

        // Insert end of track event (delta time = 0)
        //
        midiEvent = new MidiEvent(0, MIDI_FILE_META_EVENT,
                                  MIDI_END_OF_TRACK, "");

        m_midiComposition[i].push_back(midiEvent);

    }

    return;
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
    while  ((inNumber >>= 7 ) > 0)
    {
        outNumber <<= 8;
        outNumber |= 0x80;
        outNumber += (inNumber & 0x7f);
    }

    // Now move the converted number out onto the buffer
    //
    while(true)
    {
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

    return(true);
}

// Write a MIDI track to file
//
bool
MidiFile::writeTrack(std::ofstream* midiFile, TrackId trackNumber)
{
    bool retOK = true;
    MidiByte eventCode = 0;
    MidiTrackIterator midiEvent;

    // First we write into the trackBuffer, then write it out to the
    // file with it's accompanying length.
    //
    string trackBuffer;

    long progressTotal = m_midiComposition[trackNumber].size();
    long progressCount = 0;

    for (midiEvent = m_midiComposition[trackNumber].begin();
         midiEvent != m_midiComposition[trackNumber].end();
         midiEvent++)
    {
        // Write the time to the buffer in MIDI format
        //
        //
        trackBuffer += longToVarBuffer((*midiEvent)->getTime());

        if ((*midiEvent)->isMeta())
        {
            trackBuffer += MIDI_FILE_META_EVENT;
            trackBuffer += (*midiEvent)->getMetaEventCode();

            // Variable length number field
            trackBuffer += longToVarBuffer((*midiEvent)->
                                               getMetaMessage().length());

            trackBuffer += (*midiEvent)->getMetaMessage();
        }
        else
        {
            // Send the normal event code (with encoded channel information)
            //
            // Fix for 674731 by Pedro Lopez-Cabanillas (20030531)
            if (((*midiEvent)->getEventCode() != eventCode) ||
                ((*midiEvent)->getEventCode() == MIDI_SYSTEM_EXCLUSIVE))
            {
                trackBuffer += (*midiEvent)->getEventCode();
                eventCode = (*midiEvent)->getEventCode();
            }

            // Send the relevant data
            //
            switch((*midiEvent)->getMessageType())
            {
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
            emit setProgress(progressCount * 100 / progressTotal);
            kapp->processEvents(500);
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

    return(retOK);
}

// Writes out a MIDI file from the internal Midi representation
//
bool
MidiFile::write()
{
    bool retOK = true;

    std::ofstream *midiFile =
        new std::ofstream(m_fileName.c_str(), ios::out | ios::binary);


    if (!(*midiFile))
    {
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

    if(!retOK)
        m_format = MIDI_FILE_NOT_LOADED;
   
    return (retOK);
}

// Delete dead NOTE OFF and NOTE ON/Zero Velocty Events after
// reading them and modifying their relevant NOTE ONs
//
bool
MidiFile::consolidateNoteOffEvents(TrackId track)
{
    MidiTrackIterator nOE, mE = m_midiComposition[track].begin();
    bool notesOnTrack = false;
    bool noteOffFound;

    for(;mE != m_midiComposition[track].end(); mE++)
    {
        if ((*mE)->getMessageType() == MIDI_NOTE_ON && (*mE)->getVelocity() > 0)
        {
            // We've found a note - flag it
            //
            if (!notesOnTrack) notesOnTrack = true;

            noteOffFound = false;

            for (nOE = mE; nOE != m_midiComposition[track].end(); nOE++)
            {
                if (((*nOE)->getChannelNumber() == (*mE)->getChannelNumber()) &&
		    ((*nOE)->getPitch() == (*mE)->getPitch()) &&
                    ((*nOE)->getMessageType() == MIDI_NOTE_OFF ||
                    ((*nOE)->getMessageType() == MIDI_NOTE_ON &&
                     (*nOE)->getVelocity() == 0x00)))
                {
                    (*mE)->setDuration((*nOE)->getTime() - (*mE)->getTime());

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

	for (MidiTrackIterator ti = ci->second.begin();
	     ti != ci->second.end(); ++ti) {
	    delete *ti;
	}

	ci->second.clear();
    }

    m_midiComposition.clear();
}

// Doesn't do anything yet - doesn't need to.  We need to satisfy
// the pure virtual function in the base class.
//
void
MidiFile::close()
{
}



}

