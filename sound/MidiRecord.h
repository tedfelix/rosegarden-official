
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

#ifndef _ROSEGARDEN_MIDI_RECORD_H_
#define _ROSEGARDEN_MIDI_RECORD_H_

#include "MidiArts.h"
#include <arts/artsmidi.h>

// Extends the Arts MIDI skeleton for Rosegarden
//
//

namespace Rosegarden
{

  class RosegardenMidiRecord_impl :
         virtual public RosegardenMidiRecord_skel
  {
  public:
    RosegardenMidiRecord_impl();
    ~RosegardenMidiRecord_impl();

    void processCommand(const Arts::MidiCommand &midiCommand);
    void processEvent(const Arts::MidiEvent &midiEvent);

    bool record() { return _record; }
    void record(bool recordFlag) { _record = recordFlag; }
    void setMidiThru(Arts::MidiPort port) { _midiThru = port; }
  
    Arts::TimeStamp time() { return _midiThru.time(); }

    vector<Arts::MidiEvent> *getQueue();

  private:
    bool _record;
    Arts::MidiPort _midiThru;
    vector<Arts::MidiEvent> *_midiEventQueue;

    void addToList(const Arts::MidiEvent &mE);
  };

}

#endif // _ROSEGARDEN_MIDI_RECORD_H_
