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

#include "Instrument.h"
#include "Device.h"

#ifndef _MAPPEDSTUDIO_H_
#define _MAPPEDSTUDIO_H_


// A sequencer-side representation of certain elements in the
// gui that enables us to control outgoing or incoming audio
// and MIDI with run-time only persistence.  Placeholders for
// our Studio elements on the sequencer.
//
//

namespace Rosegarden
{

// We give everything we create a unique MappedStudioId for
// this session.
//

typedef unsigned int MappedStudioId;

class MappedStudio
{
public:
    MappedStudio();

    typedef enum
    {
        AudioVolume
    } MappedStudioItem;

    // Create a new slider of a certain type for a certain
    // type of device.
    //
    MappedStudioId createSlider(Device::DeviceType type,
                                MappedStudioItem item);

    // Connect an Instrument to a MappedStudioItem
    //
    bool connectItem(InstrumentId iId,
                     MappedStudioId msId);

    // Destroy a MappedStudio item
    //
    bool destroyItem(MappedStudioId id);

protected:

private:
    MappedStudioId m_mappedStudioId;
};

}

#endif // _MAPPEDSTUDIO_H_
