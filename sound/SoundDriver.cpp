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

namespace Rosegarden
{

SoundDriver::SoundDriver(const std::string &name):m_name(name)
{
}


SoundDriver::~SoundDriver()
{
}

MappedInstrument*
SoundDriver::getMappedInstrument(InstrumentId id)
{
    std::vector<Rosegarden::MappedInstrument*>::iterator it;

    for (it = m_instruments.begin(); it != m_instruments.end(); it++)
    {
        if ((*it)->getID() == id)
            return (*it);
    }

    return 0;
}


}

