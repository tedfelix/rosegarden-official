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


#ifndef _ROSEGARDEN_MIDI_EVENT_H_
#define _ROSEGARDEN_MIDI_EVENT_H_

#include "Midi.h"

// We use this class to hold our MIDI data before and after file access.
// This is our stepping stone between the Application and the MIDI file.  
//
//

namespace Rosegarden
{
  class MidiEvent
  {

  public:
    MidiEvent();

    // single data byte case
    //
    MidiEvent(const unsigned int &deltaTime,
              const MidiByte &eventCode,
              const MidiByte &data1);

    // double data byte
    //
    MidiEvent(const unsigned int &deltaTime,
              const MidiByte &eventCode,
              const MidiByte &data1,
              const MidiByte &data2);

    // meta event
    //
    MidiEvent(const unsigned int &deltaTime,
              const MidiByte &eventCode,
              const MidiByte &metaEventCode,
              const std::string &metaMessage);

    ~MidiEvent();

    void print();

    unsigned int time() const { return _deltaTime; }
    unsigned int addTime(const unsigned int &time);
    void setTime(const unsigned int &time) { _deltaTime = time; }

    inline const MidiByte messageType()
        { return ( _eventCode & MIDI_MESSAGE_TYPE_MASK ); }

    inline const MidiByte channelNumber()
        { return ( _eventCode & MIDI_CHANNEL_NUM_MASK ); }

    inline const MidiByte eventCode() { return _eventCode; }

    inline const MidiByte note() { return _data1; }
    inline const MidiByte velocity() { return _data2; }

    inline const bool isMeta()
      { return (_eventCode == MIDI_FILE_META_EVENT ? true : false ); }

    inline std::string metaMessage() const { return _metaMessage; }
    inline const MidiByte metaMessageType() { return _metaEventCode; }

    void duration(const unsigned int& duration)
        { _duration = duration; }

    const unsigned int& duration() { return _duration; }

    friend bool operator<(const MidiEvent &a, const MidiEvent &b);


  private:

    MidiEvent& operator=(const MidiEvent mE) {;}

    unsigned int _deltaTime;
    unsigned int _duration;
    MidiByte _eventCode;
    MidiByte _data1;
    MidiByte _data2;

    MidiByte _metaEventCode;
    std::string   _metaMessage;
    

  };
}

#endif // _ROSEGARDEN_MIDI_EVENT_H_
