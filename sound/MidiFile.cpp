// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-

/*
  Rosegarden-4 v0.1
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


#include <iostream>
#include <fstream>
#include <string>
#include "Midi.h"
#include "MidiFile.h"
#include "Segment.h"
#include "NotationTypes.h"
#include "BaseProperties.h"
#include "SegmentNotationHelper.h"
#include "SegmentPerformanceHelper.h"
#include "Track.h"
#include "Instrument.h"

#if (__GNUC__ < 3)
#include <strstream>
#define stringstream strstream
#else
#include <sstream>
#endif


namespace Rosegarden
{

using std::string;
using std::ifstream;
using std::stringstream;
using std::cerr;
using std::cout;
using std::endl;
using std::ends;
using std::ios;

MidiFile::MidiFile():SoundFile(std::string("unnamed.mid")),
                     m_timingDivision(0),
                     m_format(MIDI_FILE_NOT_LOADED),
                     m_numberOfTracks(0),
                     m_trackByteCount(0),
                     m_decrementCount(false)
{
}

MidiFile::MidiFile(const std::string &fn):SoundFile(fn),
                                   m_timingDivision(0),
                                   m_format(MIDI_FILE_NOT_LOADED),
                                   m_numberOfTracks(0),
                                   m_trackByteCount(0),
                                   m_decrementCount(false)
{
}

MidiFile::~MidiFile()
{
}


// A couple of convenience functions.   Watch the byte conversions out
// of the STL strings.
//
//
const unsigned long
MidiFile::midiBytesToLong(const string& bytes)
{
    assert(bytes.length() == 4);
    long longRet = ((long)(((MidiByte)bytes[0]) << 24)) |
        ((long)(((MidiByte)bytes[1]) << 16)) |
        ((short)(((MidiByte)bytes[2]) << 8)) |
        ((short)((MidiByte)(bytes[3])));

    return(longRet);
}

const unsigned int
MidiFile::midiBytesToInt(const string& bytes)
{
    assert(bytes.length() == 2);
    int intRet = ((short)(((MidiByte)bytes[0]) << 8)) |
        ((short)(((MidiByte)bytes[1])));
    return(intRet);
}



// Our MIDI file accessor function and best regarded as single point of entry
// despite the ifstream pointer flying around the place.  Gets a specified
// number of bytes from the MIDI byte stream.  For each track section we
// can read only a specified number of bytes held in m_trackByteCount.
//
//
const string
MidiFile::getMidiBytes(ifstream* midiFile, const unsigned int &numberOfBytes)
{
    string stringRet;
    char fileMidiByte;

    if (m_decrementCount && (numberOfBytes > (unsigned int)m_trackByteCount))
    {
        cerr << "Attempt to get more bytes than allowed on Track - ( " <<
            numberOfBytes << " > " << m_trackByteCount << " )" << endl;
        throw(std::string("Attempt to get more bytes than allowed on Track"));
    }

    if (midiFile->eof())
    {
        cerr << "MIDI file EOF - got " << stringRet.length() << " bytes out of "
             << numberOfBytes << endl;
        throw(std::string("MIDI EOF encountered while reading"));
    }

    while((stringRet.length() < numberOfBytes) && midiFile->read(&fileMidiByte, 1))
    {
        stringRet += fileMidiByte;
#ifdef MIDI_DEBUG
        cout << " STR = " << (int)stringRet[stringRet.length()-1] <<
            " - " << (int) fileMidiByte << endl;
#endif
    }

    // if we've reached the end of file without fulfilling the
    // quota then panic as our parsing has performed incorrectly
    //
    if (stringRet.length() < numberOfBytes)
    {
        stringRet = "";
        cerr << "Attempt to read past file end - got " << stringRet.length() <<
            " bytes out of " << numberOfBytes << endl;
        throw(std::string("Attempt to read part MIDI file end"));
    }

    // decrement the byte count
    if (m_decrementCount)
        m_trackByteCount -= stringRet.length();

    return stringRet;
}


// Get a long number of variable length from the MIDI byte stream.
//
//
const unsigned int
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
        if (buffer.compare(Rosegarden::MIDI_TRACK_HEADER, 0, 4) == 0)
#else
            if (buffer.compare(0, 4, Rosegarden::MIDI_TRACK_HEADER) == 0)
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

    // Open the file
    ifstream *midiFile = new ifstream(m_fileName.c_str(), ios::in | ios::binary);

    try
    {
        if (*midiFile)
        {
            // Parse the MIDI header first.  The first 14 bytes of the file.
            if (!parseHeader(getMidiBytes(midiFile, 14)))
            {
                m_format = MIDI_FILE_NOT_LOADED;
                return(false);
            }

            for ( unsigned int i = 0; i < m_numberOfTracks; i++ )
            {

#ifdef MIDI_DEBUG
                std::cout << "Parsing Track " << i << endl;
#endif

                if(!skipToNextTrack(midiFile))
                {
#ifdef MIDI_DEBUG
                    cerr << "Couldn't find Track " << i << endl;
#endif
                    m_format = MIDI_FILE_NOT_LOADED;
                    return(false);
                }

                // Run through the events taking them into our internal
                // representation.
                if (!parseTrack(midiFile, i))
                {
#ifdef MIDI_DEBUG
                    std::cerr << "Track " << i << " parsing failed" << endl;
#endif
                    m_format = MIDI_FILE_NOT_LOADED;
                    return(false);
                }
            }
        }
        else
        {
            m_format = MIDI_FILE_NOT_LOADED;
            return(false);
        }

        // Close the file now
        midiFile->close();
    }
    catch(std::string e)
    {
        cout << "MidiFile::open() - caught exception - " << e << endl;
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
        std::cerr << "MidiFile::parseHeader - header undersized" << endl;
#endif

        return(false);
    }

#if (__GNUC__ < 3)
    if (midiHeader.compare(Rosegarden::MIDI_FILE_HEADER, 0, 4) != 0)
#else
        if (midiHeader.compare(0, 4, Rosegarden::MIDI_FILE_HEADER) != 0)
#endif

        {

#ifdef MIDI_DEBUG
            std::cerr << "MidiFile::parseHeader - header not found or malformed"
                      << endl;
#endif

            return(false);
        }

    if(midiBytesToLong(midiHeader.substr(4,4)) != 6L)
    {

#ifdef MIDI_DEBUG
        std::cerr << "MidiFile::parseHeader - header length incorrect" << endl;
#endif

        return(false);
    }

    m_format = (MIDIFileFormatType) midiBytesToInt(midiHeader.substr(8,2));
    m_numberOfTracks = midiBytesToInt(midiHeader.substr(10,2));
    m_timingDivision = midiBytesToInt(midiHeader.substr(12,2));

    if ( m_format == MIDI_SEQUENTIAL_TRACK_FILE )
    {

#ifdef MIDI_DEBUG
        std::cerr << "MidiFile::parseHeader - can't load sequential track file"
                  << endl;
#endif

        return(false);
    }
  

    if ( m_timingDivision < 0 )
    {

#ifdef MIDI_DEBUG
        std::cerr << "MidiFile::parseHeader - Uses SMPTE timing" << endl;
#endif

    }

    return(true); 
}



// Extract the contents from a MIDI file track and places it into
// our local map of MIDI events.
//
//
bool
MidiFile::parseTrack(ifstream* midiFile, const unsigned int &trackNum)
{
    MidiByte midiByte, eventCode, data1, data2;
    unsigned int messageLength;
    unsigned long deltaTime;
    string metaMessage;

    while (!midiFile->eof() && ( m_trackByteCount > 0 ) )
    {
        deltaTime = getNumberFromMidiBytes(midiFile);

#ifdef MIDI_DEBUG
        std::cout << "deltaTime = " << deltaTime << endl;
#endif

        // Get a single byte
        midiByte = (MidiByte) getMidiBytes(midiFile, 1)[0];

        if (!(midiByte & MIDI_STATUS_BYTE_MASK))
        {
            midiFile->seekg(-1, ios::cur);
            m_trackByteCount++;
        }
        else
            eventCode = midiByte;

        if (eventCode == MIDI_FILE_META_EVENT)
        { 
            eventCode = getMidiBytes(midiFile,1)[0];
            messageLength = getNumberFromMidiBytes(midiFile);
            metaMessage = getMidiBytes(midiFile, messageLength);

            MidiEvent e(deltaTime, MIDI_FILE_META_EVENT, eventCode, metaMessage);

            m_midiComposition[trackNum].push_back(e);
        }
        else
        {

            // We must declare the MidiEvent here as a pointer and then new it
            // inside the switch to keep the compiler happy.
            //
            MidiEvent *midiEvent;

            switch (eventCode & MIDI_MESSAGE_TYPE_MASK)
            {
            case MIDI_NOTE_ON:
            case MIDI_NOTE_OFF:
            case MIDI_POLY_AFTERTOUCH:
            case MIDI_CTRL_CHANGE:
            case MIDI_PITCH_BEND:

                data1 = (MidiByte) getMidiBytes(midiFile, 1)[0];
                data2 = (MidiByte) getMidiBytes(midiFile, 1)[0];

                // create and store our event
                midiEvent = new MidiEvent(deltaTime, eventCode, data1, data2);
                m_midiComposition[trackNum].push_back(*midiEvent);
                delete(midiEvent);

                break;

            case MIDI_PROG_CHANGE:
            case MIDI_CHNL_AFTERTOUCH:
                data1 = (MidiByte) getMidiBytes(midiFile, 1)[0];

                // create and store our event
                midiEvent = new MidiEvent(deltaTime, eventCode, data1);
                m_midiComposition[trackNum].push_back(*midiEvent);
                delete(midiEvent);

                break;

            default:
                std::cerr << "MidiFile::parseTrack - Unsupported MIDI Event" << endl;
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
Rosegarden::Composition*
MidiFile::convertToRosegarden()
{
    MidiTrackIterator midiEvent, noteOffSearch;
    Rosegarden::Segment *rosegardenSegment;
    Rosegarden::Event *rosegardenEvent;
    Rosegarden::TrackId compositionTrack = 0;
    string trackName;
    bool noteOffFound;
    bool notesOnTrack;

    // Time conversions
    //
    timeT rosegardenTime = 0;
    timeT rosegardenDuration = 0;

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

    Rosegarden::Composition *composition = new Composition;
    Rosegarden::Track *track;

    // preset default tempo
    composition->setDefaultTempo(120.0);

    // precalculate the timing factor
    //

    // [cc] -- attempt to avoid floating-point rounding errors
    timeT crotchetTime = Note(Note::Crotchet).getDuration();
    int divisor = m_timingDivision ? m_timingDivision : crotchetTime;

    for ( unsigned int i = 0; i < m_numberOfTracks; i++ )
    {
        segmentTime = 0;
        notesOnTrack = false;
        trackName = string("Imported MIDI");

        // Convert the deltaTime to an absolute time since
        // the start of the segment.  The addTime method 
        // returns the sum of the current Midi Event delta
        // time plus the argument.
        //
        for ( midiEvent = (m_midiComposition[i].begin());
              midiEvent != (m_midiComposition[i].end());
              ++midiEvent )
        {
            segmentTime = midiEvent->addTime(segmentTime);
        }

        // Consolidate NOTE ON and NOTE OFF events into a
        // NOTE ON with a duration and delete the NOTE OFFs.
        //
        for ( midiEvent = (m_midiComposition[i].begin());
              midiEvent != (m_midiComposition[i].end());
              ++midiEvent )
        {
            if (midiEvent->messageType() == MIDI_NOTE_ON)
            {
                // Flag that we've found notes on this track
                //
                if (!notesOnTrack) notesOnTrack = true;

                noteOffFound = false;

                for ( noteOffSearch = midiEvent; 
                      noteOffSearch != (m_midiComposition[i].end());
                      noteOffSearch++ )
                {
                    if ( ( midiEvent->channelNumber() == noteOffSearch->channelNumber() )
                         && ( ( noteOffSearch->messageType() == MIDI_NOTE_OFF ) ||
                              ( ( noteOffSearch->messageType() == MIDI_NOTE_ON ) &&
                                ( noteOffSearch->velocity() == 0x00 ) ) ) )
                    {
                        midiEvent->duration(noteOffSearch->time() - midiEvent->time());
                        m_midiComposition[i].erase(noteOffSearch);
                        noteOffFound = true;
                        break;
                    }

                    // set duration to the length of the segment if not found
                    if (noteOffFound == false)
                    {
                        midiEvent->duration(noteOffSearch->time() - midiEvent->time());
                    }
                }
            }
        }

        // We parse the track even if there are known to be no notes
        // on it, so as to handle tempo and time-signature events
        // correctly.  But we only create a segment to write it to if
        // there are some notes to write.
        
        if (notesOnTrack) {
            rosegardenSegment = new Segment;
            rosegardenSegment->setTrack(compositionTrack);
            rosegardenSegment->setStartIndex(0);

            track = new Rosegarden::Track(compositionTrack,
                                          false,
                                          Rosegarden::Track::Midi,
                                          trackName,
                                          compositionTrack,
                                          0);

            composition->addTrack(track);
            // add the Segment to the Composition and increment the
            // Rosegarden segment number
            //
            composition->addSegment(rosegardenSegment);
            compositionTrack++;

        } else {
            rosegardenSegment = 0;
        }

        // rest creation token needs to be reset here
        //
        endOfLastNote = 0;

        for ( midiEvent = (m_midiComposition[i].begin());
              midiEvent != (m_midiComposition[i].end());
              ++midiEvent )
        {
            // [cc] -- avoid floating-point
            rosegardenTime =
                ((timeT)midiEvent->time() * crotchetTime) / divisor;
            rosegardenDuration =
                ((timeT)midiEvent->duration() * crotchetTime) / divisor;

            if (midiEvent->isMeta())
            {
                switch(midiEvent->metaEventCode())
                {
                case MIDI_SEQUENCE_NUMBER:
                    break;

                case MIDI_TEXT_EVENT:
                    break;

                case MIDI_COPYRIGHT_NOTICE:
                    break;
                    
                case MIDI_TRACK_NAME:
                    if (rosegardenSegment)
                        track->setLabel(midiEvent->metaMessage());
                    break;

                case MIDI_INSTRUMENT_NAME:
                    break;

                case MIDI_LYRIC:
                    break;

                case MIDI_TEXT_MARKER:
                    break;

                case MIDI_CUE_POINT:
                    break;

                case MIDI_CHANNEL_PREFIX:
                    break;

                case MIDI_CHANNEL_PREFIX_OR_PORT:
                    break;

                case MIDI_END_OF_TRACK:
                    if (rosegardenSegment) {
                        if (endOfLastNote < rosegardenTime) {
                            rosegardenSegment->fillWithRests(rosegardenTime);
                        }
                    }

                    break;

                case MIDI_SET_TEMPO:
                    {
                        MidiByte m0 = midiEvent->metaMessage()[0];
                        MidiByte m1 = midiEvent->metaMessage()[1];
                        MidiByte m2 = midiEvent->metaMessage()[2];

                        long tempo = (((m0 << 8) + m1) << 8) + m2;
                        
                        if (tempo != 0)
                        {
                            tempo = 60000000 / tempo;
                            composition->addTempo(rosegardenTime, tempo);
                        }
                    }

                    break;

                case MIDI_SMPTE_OFFSET:
                    break;

                case MIDI_TIME_SIGNATURE:
                    numerator = (int) midiEvent->metaMessage()[0];
                    denominator = 1 << ((int) midiEvent->metaMessage()[1]);

                    if (numerator == 0 ) numerator = 4;
                    if (denominator == 0 ) denominator = 4;

                    composition->addTimeSignature
                        (rosegardenTime,
                         Rosegarden::TimeSignature(numerator, denominator));

                    break;

                case MIDI_KEY_SIGNATURE:

                    if (!rosegardenSegment) break;

                    // get the details
                    accidentals = (int) midiEvent->metaMessage()[0];
                    isMinor     = (int) midiEvent->metaMessage()[1];
                    isSharp     = accidentals < 0 ?        false  :  true;
                    accidentals = accidentals < 0 ?  -accidentals :  accidentals;

                    // create and insert the key event
                    //
                    rosegardenEvent = Rosegarden::Key
                        (accidentals, isSharp, isMinor).getAsEvent(rosegardenTime);
                    rosegardenSegment->insert(rosegardenEvent);

                    break;

                case MIDI_SEQUENCER_SPECIFIC:
                    break;

                default:
                    break;
                } 

            }


            switch(midiEvent->messageType())
            {
            case MIDI_NOTE_ON:

                if (!rosegardenSegment) break;

                // insert rests if we need them
                //
                if (endOfLastNote < rosegardenTime ) {
                    rosegardenSegment->fillWithRests(rosegardenTime, true);
                }

                endOfLastNote = rosegardenTime + rosegardenDuration;

                // create and populate event
                rosegardenEvent = new Event(Rosegarden::Note::EventType);
                rosegardenEvent->setAbsoluteTime(rosegardenTime);
                rosegardenEvent->setType(Note::EventType);
                rosegardenEvent->set<Int>(BaseProperties::PITCH, midiEvent->note());
                rosegardenEvent->set<Int>(BaseProperties::VELOCITY, midiEvent->velocity());
                rosegardenEvent->setDuration(rosegardenDuration);

                {
                    // insert into Segment
                    Segment::iterator loc =
                        rosegardenSegment->insert(rosegardenEvent);

                    // [cc] -- a bit of an experiment
                    SegmentNotationHelper helper(*rosegardenSegment);
                    if (!helper.isViable(rosegardenEvent)) {
                        helper.makeNoteViable(loc);
                    }
                }
                break;

            case MIDI_NOTE_OFF:
                std::cout << "MidiFile::convertToRosegarden - MIDI_OFF should not exist here!" << endl;
                break;

            case MIDI_POLY_AFTERTOUCH:
                break;

            case MIDI_CTRL_CHANGE:
                break;

            case MIDI_PROG_CHANGE:
                break;

            case MIDI_CHNL_AFTERTOUCH:
                break;

            case MIDI_PITCH_BEND:
                break;

            default:
                //cout << "Can't create Rosegarden event for unknown MIDI event"
                //<< endl;
                break;
            }
        }

        // [cc]

        if (rosegardenSegment) {

            SegmentNotationHelper helper(*rosegardenSegment);

            rosegardenSegment->insert
                (helper.guessClef(rosegardenSegment->begin(),
                                  rosegardenSegment->end()).getAsEvent(0));

            helper.autoBeam(rosegardenSegment->begin(),
                            rosegardenSegment->end(),
                            BaseProperties::GROUP_TYPE_BEAMED);
        }

    }

    Rosegarden::Instrument *instrument =
                new Rosegarden::Instrument(0,
                                           Rosegarden::Instrument::Midi,
                                           string("MIDI Instrument 1"));

    composition->addInstrument(instrument);


    // set a tempo based on timing division or default
    //
//!!! No, divisor has nothing to do with tempo I think
//    composition->setDefaultTempo((120 * crotchetTime) / divisor);

    return composition;


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
MidiFile::convertToMidi(Rosegarden::Composition &comp)
{
    MidiEvent *midiEvent;
    int trackNumber = 0;

    int midiEventAbsoluteTime;
    int midiVelocity;
    int midiChannel = 0;

    // [cc] int rather than floating point
    //
    m_timingDivision = Note(Note::Crotchet).getDuration();
    timeT crotchetDuration = Note(Note::Crotchet).getDuration();


    // Export as this format only
    //
    m_format = MIDI_SIMULTANEOUS_TRACK_FILE;

    // Clear out the store
    //
    m_midiComposition.clear();

    // Insert the Rosegarden Signature Track here and any relevant
    // file META information - this will get written out just like
    // any other MIDI track.
    //
    midiEvent = new MidiEvent(0, MIDI_FILE_META_EVENT, MIDI_TEXT_MARKER,
                              "Created by Rosegarden 4.0");

    m_midiComposition[trackNumber].push_back(*midiEvent);

    midiEvent = new MidiEvent(0, MIDI_FILE_META_EVENT, MIDI_TEXT_MARKER,
                              "http://rosegarden.sourceforge.net");

    m_midiComposition[trackNumber].push_back(*midiEvent);

    // Insert tempo events
    //
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
        m_midiComposition[trackNumber].push_back(*midiEvent);
    }

    // Insert time signatures
    //
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

        midiEvent = new MidiEvent(midiEventAbsoluteTime,
                                  MIDI_FILE_META_EVENT,
                                  MIDI_TIME_SIGNATURE,
                                  timeSigString);

        m_midiComposition[trackNumber].push_back(*midiEvent);
    }

    // first track proper
    //
    trackNumber++;

    // In pass one just insert all events including new NOTE OFFs at the right
    // absolute times.
    //
    for (Rosegarden::Composition::const_iterator trk = comp.begin();
         trk != comp.end(); ++trk)
    {
        // We use this later to get NOTE durations
        //
        SegmentPerformanceHelper helper(**trk);

        {
            stringstream trackName;
            // insert a track name
            trackName << "Track " << trackNumber << ends;

            midiEvent = new MidiEvent(0,
                                      MIDI_FILE_META_EVENT,
                                      MIDI_TRACK_NAME,
                                      //trackName.str());
                          comp.getTrackByIndex((*trk)->getTrack())->getLabel());
            m_midiComposition[trackNumber].push_front(*midiEvent);

            // insert a program change
            midiEvent = new MidiEvent(0, MIDI_PROG_CHANGE | midiChannel, 0);
            m_midiComposition[trackNumber].push_front(*midiEvent);
        }

 
        for (Rosegarden::Segment::iterator el = (*trk)->begin();
             el != (*trk)->end(); ++el)
        {

            if ((*el)->isa(Note::EventType))
            {
                // Set delta time temporarily to absolute time for this event.
                //
                // [cc] -- avoiding floating-point
                midiEventAbsoluteTime =
                    (*el)->getAbsoluteTime() * m_timingDivision / crotchetDuration;
                try
                {
                    midiVelocity = (*el)->get<Int>(BaseProperties::VELOCITY);
                }
                catch(...)
                {
                    std::cerr << "MidiFile::convertToMidi() - " <<
                                 "couldn't get velocity - " << 
                                 "using default (127)" << std::endl;
                    midiVelocity = 127;
                }
                              
                // insert the NOTE_ON at the appropriate channel
                //
                midiEvent = new MidiEvent(midiEventAbsoluteTime,
                                        MIDI_NOTE_ON + midiChannel,
                                        (*el)->get<Int>(BaseProperties::PITCH),
                                        midiVelocity);

                m_midiComposition[trackNumber].push_back(*midiEvent);

                // Get the sounding time for the matching NOTE_OFF.
                // We use SegmentPerformanceHelper::getSoundingDuration()
                // to work out the tied duration of the NOTE.
                //
                // [cc] avoiding floating-point
                midiEventAbsoluteTime +=
                    helper.getSoundingDuration(el) * m_timingDivision / crotchetDuration;

                // insert the matching NOTE OFF
                //
                midiEvent = new MidiEvent(midiEventAbsoluteTime,
                                          MIDI_NOTE_OFF + midiChannel,
                                          (*el)->get<Int>(BaseProperties::PITCH),
                                          127);

                m_midiComposition[trackNumber].push_back(*midiEvent);

            }
            else if ((*el)->isa(Note::EventRestType))
            {
                // skip
            }
            else
            {  
                // skip
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
    MidiTrackIterator midiEventIt;
    int lastMidiTime;
    int endOfSegmentTime;

    for (unsigned int i = 0; i < m_numberOfTracks; i++)
    {
        lastMidiTime = 0;

        // First sort the list
        //
        m_midiComposition[i].sort();

        // insert end of track event
        MidiTrackIterator lastEvent = (m_midiComposition[i].end());
        endOfSegmentTime  = (--lastEvent)->time();

        midiEvent = new MidiEvent(endOfSegmentTime, MIDI_FILE_META_EVENT,
                                  MIDI_END_OF_TRACK, "");

        m_midiComposition[i].push_back(*midiEvent);

        for ( midiEventIt = (m_midiComposition[i].begin());
              midiEventIt != (m_midiComposition[i].end()); midiEventIt++ )
        {
            midiEventAbsoluteTime = midiEventIt->time() - lastMidiTime;
            lastMidiTime = midiEventIt->time();
            midiEventIt->setTime(midiEventAbsoluteTime);
        }
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
MidiFile::longToMidiBytes(std::ofstream* midiFile, const unsigned long &number)
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
void
MidiFile::longToVarBuffer(std::string &buffer, const unsigned long &number)
{
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
        buffer += (MidiByte) ( outNumber & 0xff );
        if ( outNumber & 0x80 )
            outNumber >>= 8;
        else
            break;
    }

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
MidiFile::writeTrack(std::ofstream* midiFile, const unsigned int &trackNumber)
{
    bool retOK = true;
    MidiByte eventCode = 0;
    MidiTrackIterator midiEvent;

    // First we write into the trackBuffer, then write it out to the
    // file with it's accompanying length.
    //
    string trackBuffer;

    for ( midiEvent = (m_midiComposition[trackNumber].begin());
          midiEvent != (m_midiComposition[trackNumber].end()); ++midiEvent )
    {
        // Write the time to the buffer in MIDI format
        //
        //
        longToVarBuffer(trackBuffer, midiEvent->time());

        if (midiEvent->isMeta())
        {
            trackBuffer += (MidiByte)MIDI_FILE_META_EVENT;
            trackBuffer += (MidiByte)midiEvent->metaEventCode();

            // Variable length number field
            longToVarBuffer(trackBuffer, midiEvent->metaMessage().length());

            trackBuffer += midiEvent->metaMessage();
        }
        else
        {
            // Send the normal event code (with encoded channel information)
            //
            if ((MidiByte)midiEvent->eventCode() != eventCode)
            {
                trackBuffer += (MidiByte)midiEvent->eventCode();
                eventCode = (MidiByte)midiEvent->eventCode();
            }

            // Send the relevant data
            //
            switch(midiEvent->messageType())
            {
            case MIDI_NOTE_ON:
            case MIDI_NOTE_OFF:
            case MIDI_POLY_AFTERTOUCH:
                trackBuffer += (MidiByte)midiEvent->note();
                trackBuffer += (MidiByte)midiEvent->velocity();
                break;

            case MIDI_CTRL_CHANGE:
                trackBuffer += (MidiByte)midiEvent->data1();
                trackBuffer += (MidiByte)midiEvent->data2();
                break;

            case MIDI_PROG_CHANGE:
                trackBuffer += (MidiByte)midiEvent->data1();
                break;

            case MIDI_CHNL_AFTERTOUCH:
                trackBuffer += (MidiByte)midiEvent->data1();
                break;

            case MIDI_PITCH_BEND:
                trackBuffer += (MidiByte)midiEvent->data1();
                trackBuffer += (MidiByte)midiEvent->data2();
                break;

            default:
                std::cerr << "MidiFile::writeTrack - cannot write unsupported MIDI event"
                          << endl;
                break;
            }
        }
    }

    // Now we write the track - First the standard header..
    //
    *midiFile << MIDI_TRACK_HEADER.c_str();

    // ..now the length of the buffer..
    //
    longToMidiBytes(midiFile, (long)trackBuffer.length());

#ifdef MIDI_DEBUG
    cout << "LENGTH of BUFFER = " << trackBuffer.length() << endl;
#endif

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
        std::cerr << "MidiFile::write() - can't write file" << endl;
        m_format = MIDI_FILE_NOT_LOADED;
        return false;
    }

    // Write out the Header
    //
    writeHeader(midiFile);

    // And now the tracks
    //
    for(unsigned int i = 0; i < m_numberOfTracks; i++ )
    {
        if (!writeTrack(midiFile, i))
        {
            retOK = false;
        }
    }

    midiFile->close();

    if(!retOK)
        m_format = MIDI_FILE_NOT_LOADED;
   
    return (retOK);
}

}

