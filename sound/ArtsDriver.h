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

#include "SoundDriver.h"


// Specialisation of SoundDriver to support aRts
//
//


#ifndef _ARTSDRIVER_H_
#define _ARTSDRIVER_H_

namespace Rosegarden
{

class ArtsDriver : public SoundDriver
{
public:
    ArtsDriver();
    virtual ~ArtsDriver();

    virtual void generateInstruments();
    virtual void initialiseMidi();
    virtual void initialiseAudio();
    virtual void initialisePlayback();
    virtual void resetPlayback();
    virtual void allNotesOff();
    virtual void processNotesOff(const RealTime &time);
    virtual void processAudioQueue();

    virtual RealTime getSequencerTime();

    virtual void
        immediateProcessEventsOut(MappedComposition &mC);

    virtual MappedComposition*
        getMappedComposition(const RealTime &playLatency);
    
    virtual void processMidiOut(const MappedComposition &mC,
                                const RealTime &playLatency,
                                bool now);

private:

};

}

#endif // _ARTSDRIVER_H_

