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

// EXPERIMENTAL - SHOULDN'T EVEN BE IN THE BUILD YET!!!!
//
// rwb 04.04.2002


#include <string>

#include "Instrument.h"
#include "Device.h"
#include "MappedComposition.h"
#include "RealTime.h"

// Abstract base to support SoundDrivers such as aRts and ALSA
//
//

#ifndef _SOUNDDRIVER_H_
#define _SOUNDDRIVER_H_

namespace Rosegarden
{

class SoundDriver
{
public:
    SoundDriver(const std::string &name):m_name(name) {;}
    virtual ~SoundDriver() {;}

    virtual void generateInstruments() = 0;

    virtual void initialiseMidi() = 0;
    virtual void initialiseAudio() = 0;
    virtual void initialisePlayback() = 0;
    virtual void resetPlayback() = 0;
    virtual void allNotesOff() = 0;
    virtual void processNotesOff(const RealTime &time) = 0;
    virtual void processAudioQueue() = 0;
    
    virtual RealTime getSequencerTime() = 0;

    virtual void
        immediateProcessEventsOut(MappedComposition &mC) = 0;

    virtual MappedComposition*
        getMappedComposition(const RealTime &playLatency) = 0;

    virtual void processMidiOut(const MappedComposition &mC,
                                const RealTime &playLatency,
                                bool now) = 0;

protected:
    std::string    m_name;
    InstrumentList m_instruments;

};

}

#endif // _ALSADRIVER_H_

