// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-

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


#include <iostream>
#include <fstream>
#include <string>
#include <strstream>
#include "Midi.h"
#include "MidiFile.h"
#include "Segment.h"
#include "NotationTypes.h"
#include "BaseProperties.h"
#include "SegmentNotationHelper.h"
#include "SegmentPerformanceHelper.h"


namespace Rosegarden
{

using std::string;
using std::ifstream;
using std::strstream;
using std::cerr;
using std::cout;
using std::endl;
using std::ends;
using std::ios;

MidiFile::MidiFile():m_filename("unnamed.mid"),
		     m_timingDivision(0),
		     m_format(MIDI_FILE_NOT_LOADED),
		     m_numberOfSegments(0),
		     m_segmentByteCount(0),
		     m_decrementCount(false)
{
}

MidiFile::MidiFile(const char *fn):m_filename(fn),
				   m_timingDivision(0),
				   m_format(MIDI_FILE_NOT_LOADED),
				   m_numberOfSegments(0),
				   m_segmentByteCount(0),
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
// number of bytes from the MIDI byte stream.  For each segment section we
// can read only a specified number of bytes held in m_segmentByteCount.
//
//
const string
MidiFile::getMidiBytes(ifstream* midiFile, const unsigned int &numberOfBytes)
{
    string stringRet;
    char fileMidiByte;

    if (m_decrementCount && (numberOfBytes > (unsigned int)m_segmentByteCount))
    {
	cerr << "Attempt to get more bytes than allowed on Segment - ( " <<
	    numberOfBytes << " > " << m_segmentByteCount << " )" << endl;
	throw(std::string("Attempt to get more bytes than allowed on Segment"));
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
	m_segmentByteCount -= stringRet.length();

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



// Seeks to the next segment in the midi file and sets the number
// of bytes to be read in the counter m_segmentByteCount.
//
bool
MidiFile::skipToNextSegment(ifstream *midiFile)
{
    string buffer, buffer2;
    m_segmentByteCount = 0;
    m_decrementCount = false;

    while(!midiFile->eof() && (m_decrementCount == false ))
    {
	buffer = getMidiBytes(midiFile, 4); 

#if (__GNUC__ < 3)
	if (buffer.compare(Rosegarden::MIDI_SEGMENT_HEADER, 0, 4) == 0)
#else
	    if (buffer.compare(0, 4, Rosegarden::MIDI_SEGMENT_HEADER) == 0)
#endif

	    {
		m_segmentByteCount = midiBytesToLong(getMidiBytes(midiFile, 4));
		m_decrementCount = true;
	    }

    }

    if ( m_segmentByteCount == 0 ) // we haven't found a segment
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
    ifstream *midiFile = new ifstream(m_filename.c_str(), ios::in | ios::binary);

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

	    for ( unsigned int i = 0; i < m_numberOfSegments; i++ )
	    {

#ifdef MIDI_DEBUG
		std::cout << "Parsing Segment " << i << endl;
#endif

		if(!skipToNextSegment(midiFile))
		{
#ifdef MIDI_DEBUG
		    cerr << "Couldn't find Segment " << i << endl;
#endif
		    m_format = MIDI_FILE_NOT_LOADED;
		    return(false);
		}

		// Run through the events taking them into our internal
		// representation.
		if (!parseSegment(midiFile, i))
		{
#ifdef MIDI_DEBUG
		    std::cerr << "Segment " << i << " parsing failed" << endl;
#endif
		    m_format = MIDI_FILE_NOT_LOADED;
		    return(false);
		}
	    }
	}
	else
	{
#ifdef MIDI_DEBUG
	    std::cerr << "MidiFile::open - \"" << _filename <<
		"\" not recognised as a MIDI file" << endl;
#endif
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
    m_numberOfSegments = midiBytesToInt(midiHeader.substr(10,2));
    m_timingDivision = midiBytesToInt(midiHeader.substr(12,2));

    if ( m_format == MIDI_SEQUENTIAL_SEGMENT_FILE )
    {

#ifdef MIDI_DEBUG
	std::cerr << "MidiFile::parseHeader - can't load sequential segment file"
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



// Extract the contents from a MIDI file segment and places it into
// our local map of MIDI events.
//
//
bool
MidiFile::parseSegment(ifstream* midiFile, const unsigned int &segmentNum)
{
    MidiByte midiByte, eventCode, data1, data2;
    unsigned int messageLength;
    unsigned long deltaTime;
    string metaMessage;

    while (!midiFile->eof() && ( m_segmentByteCount > 0 ) )
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
	    m_segmentByteCount++;
	}
	else
	    eventCode = midiByte;

	if (eventCode == MIDI_FILE_META_EVENT)
	{ 
	    eventCode = getMidiBytes(midiFile,1)[0];
	    messageLength = getNumberFromMidiBytes(midiFile);
	    metaMessage = getMidiBytes(midiFile, messageLength);

	    MidiEvent e(deltaTime, MIDI_FILE_META_EVENT, eventCode, metaMessage);

	    m_midiComposition[segmentNum].push_back(e);
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
		m_midiComposition[segmentNum].push_back(*midiEvent);
		delete(midiEvent);

		break;

	    case MIDI_PROG_CHANGE:
	    case MIDI_CHNL_AFTERTOUCH:
		data1 = (MidiByte) getMidiBytes(midiFile, 1)[0];

		// create and store our event
		midiEvent = new MidiEvent(deltaTime, eventCode, data1);
		m_midiComposition[segmentNum].push_back(*midiEvent);
		delete(midiEvent);

		break;

	    default:
		std::cerr << "MidiFile::parseSegment - Unsupported MIDI Event" << endl;
		break;
	    } 
	}
    }

    return(true);
}


// If we wanted to abstract the MidiFile class to make it
// more useful to others we'd make this method and the return
// one pure virtual.
//
Rosegarden::Composition*
MidiFile::convertToRosegarden()
{
    MidiSegmentIterator midiEvent, noteOffSearch;
    Rosegarden::Segment *rosegardenSegment;
    Rosegarden::Event *rosegardenEvent;
    unsigned long segmentTime;
    unsigned int compositionTrack = 0;
    bool noteOffFound;
    bool notesOnSegment;

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

    // keys
    Rosegarden::Key *key;
    int accidentals;
    bool isMinor;
    bool isSharp;

    Rosegarden::Composition *composition = new Composition;

    // preset default tempo
    composition->setDefaultTempo(120.0);

    // precalculate the timing factor
    //

    // [cc] -- attempt to avoid floating-point rounding errors
    timeT crotchetTime = Note(Note::Crotchet).getDuration();
    int divisor = m_timingDivision ? m_timingDivision : crotchetTime;

    for ( unsigned int i = 0; i < m_numberOfSegments; i++ )
    {
	segmentTime = 0;
	notesOnSegment = false;

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
		// flag that we've found notes on this segment
		if (!notesOnSegment) notesOnSegment = true;

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
	
	if (notesOnSegment) {
	    rosegardenSegment = new Segment;
	    rosegardenSegment->setTrack(compositionTrack);
	    rosegardenSegment->setStartIndex(0);

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

		case MIDI_SEGMENT_NAME:
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

		case MIDI_END_OF_SEGMENT:
		    break;

		case MIDI_SET_TEMPO:

		{
		    long tempo =
			(((long(midiEvent->metaMessage()[0])  << 8) +
			  (long(midiEvent->metaMessage()[1])) << 8) +
			 ((long)midiEvent->metaMessage()[2]));
		    
		    Event *tempoEvent = new Event(Composition::TempoEventType);
		    tempoEvent->setAbsoluteTime(rosegardenTime);
		    tempoEvent->set<Int>(Composition::TempoProperty, tempo*60);
		    composition->getReferenceSegment()->insert(tempoEvent);
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
		    rosegardenSegment->fillWithRests(rosegardenTime);
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

    // set a tempo based on timing division or default
    //
    composition->setDefaultTempo((120 * crotchetTime) / divisor);

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
MidiFile::convertToMidi(const Rosegarden::Composition &comp)
{
    MidiEvent *midiEvent;
    int segmentNumber = 0;

    //int segmentStartTime;
    int midiEventAbsoluteTime;
    int midiChannel = 0;

    //int midiInstrument;

    // [cc] int rather than floating point
    //
    m_timingDivision = Note(Note::Crotchet).getDuration() * 120 / 
	(int)comp.getTempo();

    m_format = MIDI_SIMULTANEOUS_SEGMENT_FILE;

    // Clear out anything we have stored in this object already.
    //
    m_midiComposition.clear();

    // Insert the Rosegarden Signature Segment here and any relevant
    // file META information - this will get written out just like
    // any other MIDI segment.
    //
    //
    midiEvent = new MidiEvent(0, MIDI_FILE_META_EVENT, MIDI_TEXT_MARKER,
			      "Created by Rosegarden 4.0");

    m_midiComposition[segmentNumber].push_back(*midiEvent);

    midiEvent = new MidiEvent(0, MIDI_FILE_META_EVENT, MIDI_TEXT_MARKER,
			      "http://rosegarden.sourceforge.net");

    m_midiComposition[segmentNumber].push_back(*midiEvent);

    midiEvent = new MidiEvent(0, MIDI_FILE_META_EVENT, MIDI_SET_TEMPO,
			      (int)comp.getTempo());

    m_midiComposition[segmentNumber].push_back(*midiEvent);

    segmentNumber = 1;

    // Our Composition to MIDI timing factor
    //

    // [cc] -- avoiding floating-point
    timeT crotchetDuration = Note(Note::Crotchet).getDuration();

#ifdef MIDI_DEBUG
    cout << "TIMING DIVISION = " << _timingDivision << endl;
    cout << "TIMING FACTOR   = " << timingFactor << endl;
#endif 

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
	    strstream segmentName;
	    // insert a segment name
	    segmentName << "Segment " << segmentNumber << ends;

	    midiEvent = new MidiEvent(0, MIDI_FILE_META_EVENT, MIDI_SEGMENT_NAME, segmentName.str());
	    m_midiComposition[segmentNumber].push_front(*midiEvent);

	    // insert a program change
	    midiEvent = new MidiEvent(0, MIDI_PROG_CHANGE | midiChannel, 0);
	    m_midiComposition[segmentNumber].push_front(*midiEvent);
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
                              
		// insert the NOTE_ON at the appropriate channel
		//
		midiEvent = new MidiEvent(midiEventAbsoluteTime,        // time
					  MIDI_NOTE_ON + midiChannel,   // eventcode
					  (*el)->get<Int>(BaseProperties::PITCH),     // pitch
					  (*el)->get<Int>(BaseProperties::VELOCITY));                         // velocity

		m_midiComposition[segmentNumber].push_back(*midiEvent);

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

		m_midiComposition[segmentNumber].push_back(*midiEvent);

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

	// rotate around to new MIDI channel
	//midiChannel++;
	//midiChannel %= 16;

	// increment segment number
	segmentNumber++;
    }

    // Setup number of segments in daddy object
    //
    m_numberOfSegments = segmentNumber;


    // Now gnash through the MIDI events and turn the absolute times
    // into delta times.
    //
    //
    MidiSegmentIterator midiEventIt;
    int lastMidiTime;
    int endOfSegmentTime;

    for (unsigned int i = 0; i < m_numberOfSegments; i++)
    {
	lastMidiTime = 0;

	// First sort the list
	//
	m_midiComposition[i].sort();

	// insert end of segment event
	endOfSegmentTime = m_midiComposition[i].end()->time();

	midiEvent = new MidiEvent(endOfSegmentTime, MIDI_FILE_META_EVENT,
				  MIDI_END_OF_SEGMENT, "");

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

#ifdef MIDI_DEBUG
    cout << "WRITING TIME = " << number << endl;
#endif

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

    // Number of Segments we're writing and add one for
    // a first Data segment.
    //
    intToMidiBytes(midiFile, m_numberOfSegments);

    // Timing Division
    //
    intToMidiBytes(midiFile, m_timingDivision);

    return(true);
}

// Write a MIDI segment to file
//
bool
MidiFile::writeSegment(std::ofstream* midiFile, const unsigned int &segmentNumber)
{
    bool retOK = true;
    MidiByte eventCode = 0;
    MidiSegmentIterator midiEvent;

    // First we write into the segmentBuffer, then write it out to the
    // file with it's accompanying length.
    //
    string segmentBuffer;

    for ( midiEvent = (m_midiComposition[segmentNumber].begin());
	  midiEvent != (m_midiComposition[segmentNumber].end()); ++midiEvent )
    {
	// Write the time to the buffer in MIDI format
	//
	//
	longToVarBuffer(segmentBuffer, midiEvent->time());

	if (midiEvent->isMeta())
	{
	    segmentBuffer += (MidiByte)MIDI_FILE_META_EVENT;
	    segmentBuffer += (MidiByte)midiEvent->metaEventCode();

	    // Variable length number field
	    longToVarBuffer(segmentBuffer, midiEvent->metaMessage().length());

	    segmentBuffer += midiEvent->metaMessage();
	}
	else
	{
	    // Send the normal event code (with encoded channel information)
	    //
	    if ((MidiByte)midiEvent->eventCode() != eventCode)
	    {
		segmentBuffer += (MidiByte)midiEvent->eventCode();
		eventCode = (MidiByte)midiEvent->eventCode();
	    }

	    // Send the relevant data
	    //
	    switch(midiEvent->messageType())
	    {
	    case MIDI_NOTE_ON:
	    case MIDI_NOTE_OFF:
	    case MIDI_POLY_AFTERTOUCH:
		segmentBuffer += (MidiByte)midiEvent->note();
		segmentBuffer += (MidiByte)midiEvent->velocity();
		break;

	    case MIDI_CTRL_CHANGE:
		segmentBuffer += (MidiByte)midiEvent->data1();
		segmentBuffer += (MidiByte)midiEvent->data2();
		break;

	    case MIDI_PROG_CHANGE:
		segmentBuffer += (MidiByte)midiEvent->data1();
		break;

	    case MIDI_CHNL_AFTERTOUCH:
		segmentBuffer += (MidiByte)midiEvent->data1();
		break;

	    case MIDI_PITCH_BEND:
		segmentBuffer += (MidiByte)midiEvent->data1();
		segmentBuffer += (MidiByte)midiEvent->data2();
		break;

	    default:
		std::cerr << "MidiFile::writeSegment - cannot write unsupported MIDI event"
			  << endl;
		break;
	    }
	}
    }

    // Now we write the segment - First thei standard header..
    //
    *midiFile << MIDI_SEGMENT_HEADER.c_str();

    // ..now the length of the buffer..
    //
    longToMidiBytes(midiFile, (long)segmentBuffer.length());

#ifdef MIDI_DEBUG
    cout << "LENGTH of BUFFER = " << segmentBuffer.length() << endl;
#endif

    // ..then the buffer itself..
    //
    *midiFile << segmentBuffer;

    return(retOK);
}

// Writes out a MIDI file from the internal Midi representation
//
bool
MidiFile::write()
{
    bool retOK = true;

    std::ofstream *midiFile =
	new std::ofstream(m_filename.c_str(), ios::out | ios::binary);


    if (!(*midiFile))
    {
	std::cerr << "MidiFile::write() - can't write file" << endl;
	m_format = MIDI_FILE_NOT_LOADED;
	return false;
    }

    // Write out the Header
    writeHeader(midiFile);

    // And now the Segments
    //
    for(unsigned int i = 0; i < m_numberOfSegments; i++ )
    {
	if (!writeSegment(midiFile, i))
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

