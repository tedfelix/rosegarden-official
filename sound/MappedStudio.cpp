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

#include "MappedStudio.h"

namespace Rosegarden
{


MappedStudio::MappedStudio():m_mappedStudioId(0)
{
}

// Create a MIDI or Audio slider and return a unique id -
// the slider can then be connected to an Instrument.
//
// 
MappedStudioId
MappedStudio::createSlider(Device::DeviceType type,
                           MappedStudioItem item)
{
    MappedStudioId id = m_mappedStudioId++;
    return id;
}


bool
MappedStudio::destroyItem(MappedStudioId id)
{
    return true;
}

bool
MappedStudio::connectItem(InstrumentId iId,
                          MappedStudioId msId)
{
    return true;
}


}

