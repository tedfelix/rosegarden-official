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
#include "Midi.h"
#include "MidiFile.h"
#include "Track.h"
#include "NotationTypes.h"
#include "TrackNotationHelper.h" //cc
#include "TrackPerformanceHelper.h"


namespace Rosegarden
{

using std::string;
using std::ifstream;
using std::cerr;
using std::cout;
using std::endl;
using std::ios;

MidiFile::MidiFile():_filename("unnamed.mid"),
                     _timingDivision(0),
                     _format(MIDI_FILE_NOT_LOADED),
                     _numberOfTracks(0),
                     _trackByteCount(0),
                     _decrementCount(false)
{
}

MidiFile::MidiFile(const char *fn):_filename(fn),
                                   _timingDivision(0),
                                   _format(MIDI_FILE_NOT_LOADED),
                                   _numberOfTracks(0),
                                   _trackByteCount(0),
                                   _decrementCount(false)
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
// can read only a specified number of bytes held in _trackByteCount.
//
//
const string
MidiFile::getMidiBytes(ifstream* midiFile, const unsigned int &numberOfBytes)
{
  string stringRet;
  char fileMidiByte;

  if (_decrementCount && (numberOfBytes > (unsigned int)_trackByteCount))
  {
    cerr << "Attempt to get more bytes than allowed on Track - ( " <<
          numberOfBytes << " > " << _trackByteCount << " )" << endl;
    exit(1);
  }

  if (midiFile->eof())
  {
    cerr << "MIDI file EOF - got " << stringRet.length() << " bytes out of "
         << numberOfBytes << endl;
    exit(1);
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
    exit(1);
  }

  // decrement the byte count
  if (_decrementCount)
    _trackByteCount -= stringRet.length();

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
// of bytes to be read in the counter _trackByteCount.
//
bool
MidiFile::skipToNextTrack(ifstream *midiFile)
{
  string buffer, buffer2;
  _trackByteCount = 0;
  _decrementCount = false;

  while(!midiFile->eof() && (_decrementCount == false ))
  {
    buffer = getMidiBytes(midiFile, 4); 

#if (__GNUC__ < 3)
    if (buffer.compare(Rosegarden::MIDI_TRACK_HEADER, 0, 4) == 0)
#else
    if (buffer.compare(0, 4, Rosegarden::MIDI_TRACK_HEADER) == 0)
#endif

    {
      _trackByteCount = midiBytesToLong(getMidiBytes(midiFile, 4));
      _decrementCount = true;
    }

  }

  if ( _trackByteCount == 0 ) // we haven't found a track
    return(false);
  else
    return(true);
}


// Read in a MIDI file.
// 
//
bool
MidiFile::open()
{

  // Open the file
  ifstream *midiFile = new ifstream(_filename.c_str(), ios::in | ios::binary);

  if (*midiFile)
  {
    // Parse the MIDI header first.  The first 14 bytes of the file.
    if (!parseHeader(getMidiBytes(midiFile, 14)))
    {
      _format = MIDI_FILE_NOT_LOADED;
      return(false);
    }

    for ( unsigned int i = 0; i < _numberOfTracks; i++ )
    {

#ifdef MIDI_DEBUG
      std::cout << "Parsing Track " << i << endl;
#endif

      if(!skipToNextTrack(midiFile))
      {
#ifdef MIDI_DEBUG
        cerr << "Couldn't find Track " << i << endl;
#endif
        _format = MIDI_FILE_NOT_LOADED;
        return(false);
      }

      // Run through the events taking them into our internal
      // representation.
      if (!parseTrack(midiFile, i))
      {
#ifdef MIDI_DEBUG
        std::cerr << "Track " << i << " parsing failed" << endl;
#endif
        _format = MIDI_FILE_NOT_LOADED;
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
    _format = MIDI_FILE_NOT_LOADED;
    return(false);
  }

  // Close the file now
  midiFile->close();

  return(true);
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

  _format = (MIDIFileFormatType) midiBytesToInt(midiHeader.substr(8,2));
  _numberOfTracks = midiBytesToInt(midiHeader.substr(10,2));
  _timingDivision = midiBytesToInt(midiHeader.substr(12,2));

  if ( _format == MIDI_SEQUENTIAL_TRACK_FILE )
  {

#ifdef MIDI_DEBUG
    std::cerr << "MidiFile::parseHeader - can't load sequential track file"
              << endl;
#endif

    return(false);
  }
  

  if ( _timingDivision < 0 )
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

  while (!midiFile->eof() && ( _trackByteCount > 0 ) )
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
      _trackByteCount++;
    }
    else
      eventCode = midiByte;

    if (eventCode == MIDI_FILE_META_EVENT)
    { 
      eventCode = getMidiBytes(midiFile,1)[0];
      messageLength = getNumberFromMidiBytes(midiFile);
      metaMessage = getMidiBytes(midiFile, messageLength);

      MidiEvent e(deltaTime, MIDI_FILE_META_EVENT, eventCode, metaMessage);

      _midiComposition[trackNum].push_back(e);
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
          _midiComposition[trackNum].push_back(*midiEvent);
          delete(midiEvent);

          break;

        case MIDI_PROG_CHANGE:
        case MIDI_CHNL_AFTERTOUCH:
          data1 = (MidiByte) getMidiBytes(midiFile, 1)[0];

          // create and store our event
          midiEvent = new MidiEvent(deltaTime, eventCode, data1);
          _midiComposition[trackNum].push_back(*midiEvent);
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


// If we wanted to abstract the MidiFile class to make it
// more useful to others we'd make this method and the return
// one pure virtual.
//
Rosegarden::Composition*
MidiFile::convertToRosegarden()
{
  MidiTrackIterator midiEvent, noteOffSearch;
  Rosegarden::Track *rosegardenTrack;
  Rosegarden::Event *rosegardenEvent;
  unsigned long trackTime;
  unsigned int compositionTrack = 0;
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

  // keys
  Rosegarden::Key *key;
  int accidentals;
  bool isMinor;
  bool isSharp;

  Rosegarden::Composition *composition = new Composition;

  // preset tempo to zero
  composition->setTempo(0);

  // precalculate the timing factor
  //
  float timingFactor = 0.0;

  if ( _timingDivision )
    timingFactor = (float) Note(Note::Crotchet).getDuration() /
                   (float) _timingDivision;

  for ( unsigned int i = 0; i < _numberOfTracks; i++ )
  {
    trackTime = 0;
    notesOnTrack = false;

    // Convert the deltaTime to an absolute time since
    // the start of the track.  The addTime method 
    // returns the sum of the current Midi Event delta
    // time plus the argument.
    //
    for ( midiEvent = (_midiComposition[i].begin());
          midiEvent != (_midiComposition[i].end());
          ++midiEvent )
    {
      trackTime = midiEvent->addTime(trackTime);
    }

    // Consolidate NOTE ON and NOTE OFF events into a
    // NOTE ON with a duration and delete the NOTE OFFs.
    //
    for ( midiEvent = (_midiComposition[i].begin());
          midiEvent != (_midiComposition[i].end());
          ++midiEvent )
    {
      if (midiEvent->messageType() == MIDI_NOTE_ON)
      {
        // flag that we've found notes on this track
        if (!notesOnTrack) notesOnTrack = true;

        noteOffFound = false;

        for ( noteOffSearch = midiEvent; 
              noteOffSearch != (_midiComposition[i].end());
              noteOffSearch++ )
        {
          if ( ( midiEvent->channelNumber() == noteOffSearch->channelNumber() )
                && ( ( noteOffSearch->messageType() == MIDI_NOTE_OFF ) ||
                   ( ( noteOffSearch->messageType() == MIDI_NOTE_ON ) &&
                     ( noteOffSearch->velocity() == 0x00 ) ) ) )
          {
            midiEvent->duration(noteOffSearch->time() - midiEvent->time());
            _midiComposition[i].erase(noteOffSearch);
            noteOffFound = true;
            break;
          }

          // set duration to the length of the track if not found
          if (noteOffFound == false)
          {
            midiEvent->duration(noteOffSearch->time() - midiEvent->time());
          }
        }
      }
    }

    if (notesOnTrack)
    {
      // Create Track on Composition object
      rosegardenTrack = new Track;
      rosegardenTrack->setInstrument(compositionTrack);
      rosegardenTrack->setStartIndex(0);
      TrackNotationHelper notationTrack(*rosegardenTrack); //cc

      // rest creation token needs to be reset here
      //
      endOfLastNote = 0;

      // add the Track to the Composition and increment the
      // Rosegarden track number
      //
      composition->addTrack(rosegardenTrack);
      compositionTrack++;

      for ( midiEvent = (_midiComposition[i].begin());
            midiEvent != (_midiComposition[i].end());
            ++midiEvent )
      {

        if (_timingDivision)
        {
          rosegardenTime = (timeT) ( midiEvent->time() * timingFactor ) ;
          rosegardenDuration = (timeT) ( midiEvent->duration() *  timingFactor );
        }


        if ( midiEvent->messageType() == MIDI_NOTE_ON ||
             midiEvent->messageType() == MIDI_NOTE_OFF )
        {
          // insert rests if we need them
          //
          if (endOfLastNote < rosegardenTime )
	      rosegardenTrack->fillWithRests(rosegardenTime);

          endOfLastNote = rosegardenTime + rosegardenDuration;
        }
        

        if (midiEvent->isMeta())
        {
          switch(midiEvent->metaMessageType())
          {
            case MIDI_SEQUENCE_NUMBER:
              //cout << "SEQ" << endl;
              break;

            case MIDI_TEXT_EVENT:
              //cout << "TEXT" << endl;
              break;

            case MIDI_COPYRIGHT_NOTICE:
              //cout << "COPYRIGHT" << endl;
              break;

            case MIDI_TRACK_NAME:
              //cout << "TRACK" << endl;
              break;

            case MIDI_INSTRUMENT_NAME:
              //cout << "INSTRUMENT" << endl;
              break;

            case MIDI_LYRIC:
              //cout << "LYRIC" << endl;
              break;

            case MIDI_TEXT_MARKER:
              //cout << "TEXT" << endl;
              break;

            case MIDI_CUE_POINT:
              //cout << "CUE" << endl;
              break;

            case MIDI_CHANNEL_PREFIX:
              //cout << "CNAHH" << endl;
              break;

            case MIDI_CHANNEL_PREFIX_OR_PORT:
              //cout << "CHANN" << endl;
              break;

            case MIDI_END_OF_TRACK:
              //cout << "EOT" << endl;
              break;

            case MIDI_SET_TEMPO:
              //cout << "TEMPO" << endl;
              break;

            case MIDI_SMPTE_OFFSET:
              //cout << "SMPTE" << endl;
              break;

            case MIDI_TIME_SIGNATURE:
              rosegardenEvent = new Event(Rosegarden::TimeSignature::EventType);
              numerator = (int) midiEvent->metaMessage()[0];
              denominator = 1 << ((int) midiEvent->metaMessage()[1]);

              if (numerator == 0 ) numerator = 4;
              if (denominator == 0 ) denominator = 4;

              rosegardenEvent = Rosegarden::TimeSignature
                  (numerator, denominator).getAsEvent(rosegardenTime);
              rosegardenTrack->insert(rosegardenEvent);

              break;

            case MIDI_KEY_SIGNATURE:
              // get the details
              accidentals = (int) midiEvent->metaMessage()[0];
              isMinor     = (int) midiEvent->metaMessage()[1];
              isSharp     = accidentals < 0 ?        false  :  true;
              accidentals = accidentals < 0 ?  -accidentals :  accidentals;

              // create and insert the key event
              //
              key = new Rosegarden::Key(accidentals, isSharp, isMinor);
              rosegardenEvent = key->getAsEvent(rosegardenTime);
              rosegardenTrack->insert(rosegardenEvent);
              delete key;

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
            // create and populate event
            rosegardenEvent = new Event(Rosegarden::Note::EventType);
            rosegardenEvent->setAbsoluteTime(rosegardenTime);
            rosegardenEvent->setType(Note::EventType);
            rosegardenEvent->set<Int>("pitch", midiEvent->note());
            rosegardenEvent->setDuration(rosegardenDuration);

            {
              // insert into Track
              Track::iterator loc = rosegardenTrack->insert(rosegardenEvent);

              // cc -- a bit of an experiment
              if (!notationTrack.isViable(rosegardenEvent)) {
                   notationTrack.makeNoteViable(loc);
              }
            }

            break;

          case MIDI_NOTE_OFF:
            cout << "MIDI OFF SHOULD NOT EXIST" << endl;

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

      // cc

      rosegardenTrack->insert
          (notationTrack.guessClef
           (rosegardenTrack->begin(), rosegardenTrack->end()).getAsEvent(0));

      rosegardenTrack->calculateBarPositions();

      notationTrack.autoBeam
          (rosegardenTrack->begin(), rosegardenTrack->end(),
           "beamed"); // probably shouldn't be hardcoded!
    }
  }

  // set a tempo based on _timingDivision or default
  //
  if (composition->getTempo() == 0)
  {
    if (_timingDivision)
      composition->setTempo(((timeT)(timingFactor * 120)));
    else
      composition->setTempo(120);
  }

  return composition;
}

// Takes a Composition and turns it into internal MIDI representation
// that can then be written out to file.
//
// For the moment we should watch to make sure that multiple
// Track (parts) don't equate to multiple tracks in the MIDI
// Composition.
//
//
void
MidiFile::convertToMidi(const Rosegarden::Composition &comp)
{
  MidiEvent *midiEvent;
  int trackNumber = 0;

  int trackStartTime;
  int lastEventDeltaTime = 0;
  int midiEventDeltaTime;
  int midiChannel = 0;

  //int midiInstrument;

  _timingDivision = (int)((float) Note(Note::Crotchet).getDuration() * 120.0 /
                          (float) comp.getTempo());

  _format = MIDI_SIMULTANEOUS_TRACK_FILE;

  // Clear out anything we have stored in this object already.
  //
  _midiComposition.clear();

  // Insert the Rosegarden Signature Track here and any relevant
  // file META information
  //

  // Our Composition to MIDI timing factor
  //
  float timingFactor = (float) _timingDivision/
                       (float) Note(Note::Crotchet).getDuration();

  for (Rosegarden::Composition::const_iterator trk = comp.begin();
                                               trk != comp.end(); ++trk)
  {
    // We use this later to get NOTE durations
    //
    TrackPerformanceHelper helper(**trk);

    trackStartTime = (int)(timingFactor * (float)(*trk)->getStartIndex());

    for (Rosegarden::Track::iterator el = (*trk)->begin();
                                     el != (*trk)->end(); ++el)
    {

      if ((*el)->isa(Note::EventType))
      {
        // Set delta time temporarily to absolute time for this event.
        //
        midiEventDeltaTime = (int)(timingFactor * (float)
                                                  ((*el)->getAbsoluteTime()));
                              
        // Convert to proper delta time and store the marker
        //
        midiEventDeltaTime -= lastEventDeltaTime;
        lastEventDeltaTime = midiEventDeltaTime;

        // insert the NOTE_ON at the appropriate channel
        //
        midiEvent = new MidiEvent(midiEventDeltaTime,
                                  MIDI_NOTE_ON & midiChannel, 
                                  (*el)->get<Int>("pitch"),     // pitch
                                  127);                         // velocity

        _midiComposition[trackNumber].push_back(*midiEvent);


        // Get the sounding time for the matching NOTE_OFF.
        // We use TrackPerformanceHelper::getSoundingDuration()
        // to work out the tied duration of the NOTE.
        //
        midiEventDeltaTime += (int)(timingFactor * (float)
                                            (helper.getSoundingDuration(el)));

        // insert the matching NOTE OFF
        //
        midiEvent = new MidiEvent(midiEventDeltaTime,
                                  MIDI_NOTE_OFF & midiChannel,
                                  (*el)->get<Int>("pitch"),
                                  127);

        _midiComposition[trackNumber].push_back(*midiEvent);

      }
      else if ((*el)->isa(Note::EventRestType))
      {
        // skip
      }
      else
      {  
        // skip
      }

      // rotate around to new MIDI channel
      midiChannel++;
      midiChannel %= 16;

    }

    if (trackNumber == 99)
      break;

    // increment track number
    trackNumber++;
  }

  // Setup number of tracks
  //
  _numberOfTracks = trackNumber;

  return;
}

// Convert an integer into a two byte representation and
// write out to the MidiFile.
//
void
MidiFile::intToHexMidiBytes(std::ofstream* midiFile, int number)
{
  MidiByte upper;
  MidiByte lower;

  upper = ( (number & 0xF000) << 24 ) + ( (number & 0x0F00) << 16 );
  lower = ( (number & 0xF0) << 8 ) + (number & 0x0F);

  *midiFile << (MidiByte) upper;
  *midiFile << (MidiByte) lower;

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
  *midiFile << (MidiByte) _format;

  // Number of Tracks we're writing and add one for
  // a first Data track.
  //
  intToHexMidiBytes(midiFile, _numberOfTracks + 1);

  // Timing Division
  //
  intToHexMidiBytes(midiFile, _timingDivision);

  return(true);
}

// Write out a MIDI file track
//
bool
MidiFile::writeTrack(std::ofstream* midiFile, const unsigned int &trackNumber)
{
  *midiFile << MIDI_TRACK_HEADER.c_str();

  // get the length of the track and write it out

  // parse all the elements out

  std::cout << trackNumber  << endl;

  return(true);
}

// Writes out a MIDI file from the internal Midi representation
//
bool
MidiFile::write()
{
  std::ofstream *midiFile =
           new std::ofstream(_filename.c_str(), ios::out | ios::binary);


  if (!(*midiFile))
  {
    std::cerr << "MidiFile::write() - can't write file" << endl;
    return false;
  }

  // Write out the Header
  writeHeader(midiFile);

  // And now the Tracks
  //
  for(unsigned int i = 0; i < _numberOfTracks; i++ )
  {
    writeTrack(midiFile, i);
  }

  midiFile->close();

  return (true);
}

}

