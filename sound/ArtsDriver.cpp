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


#include "ArtsDriver.h"
#include <arts/artsversion.h>


namespace Rosegarden
{

ArtsDriver::ArtsDriver():SoundDriver(std::string("Arts ")
                                     + std::string(ARTS_VERSION))
{
    std::cout << "Rosegarden ArtsDriver - " << m_name << std::endl;
    generateInstruments();
}

ArtsDriver::~ArtsDriver()
{
}

void
ArtsDriver::generateInstruments()
{
}

void
ArtsDriver::initialiseMidi()
{
}

void
ArtsDriver::initialiseAudio()
{
}

void
ArtsDriver::initialisePlayback()
{
}

void
ArtsDriver::resetPlayback()
{
}

void
ArtsDriver::allNotesOff()
{
}

void
ArtsDriver::processNotesOff(const RealTime & /*time*/)
{
}

void
ArtsDriver::processAudioQueue()
{
}

RealTime
ArtsDriver::getSequencerTime()
{
    RealTime rT;
    return rT;
}

void
ArtsDriver::immediateProcessEventsOut(MappedComposition & /*mC*/)
{
}

MappedComposition*
ArtsDriver::getMappedComposition(const RealTime & /*playLatency*/)
{
    MappedComposition *mC = new MappedComposition();
    return mC;
}
    
void
ArtsDriver::processMidiOut(const MappedComposition &/*mC*/,
                           const RealTime &/*playLatency*/,
                           bool /*now*/)
{
}

}


