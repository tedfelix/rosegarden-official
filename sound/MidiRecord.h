// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-

/*
  Rosegarden-4 v0.2
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

#ifndef _ROSEGARDEN_MIDI_RECORD_H_
#define _ROSEGARDEN_MIDI_RECORD_H_

#include "MidiArts.h"
#include <arts/artsmidi.h>
#include <arts/artsversion.h>

// Extends the Arts MIDI skeleton.  This provides a basic specialised
// aRts recording port which is loaded dynamically when the Sequencer
// initialises (see docs/howtos/artsd-mcop-notes).
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

    bool record() { return m_record; }
    void record(bool recordFlag) { m_record = recordFlag; }
    void setMidiThru(Arts::MidiPort port) { m_midiThru = port; }
  
    virtual Arts::TimeStamp time();
#if (ARTS_MAJOR_VERSION >= 1) || ((ARTS_MINOR_VERSION >= 9) && \
                                  (ARTS_MICRO_VERSION >= 9))
    virtual Arts::TimeStamp playTime();
#endif
    std::vector<Arts::MidiEvent> *getQueue();

private:
    bool                          m_record;
    Arts::MidiPort                m_midiThru;
    std::vector<Arts::MidiEvent> *m_midiEventQueue;

    void addToList(const Arts::MidiEvent &mE);
};

}

#endif // _ROSEGARDEN_MIDI_RECORD_H_
