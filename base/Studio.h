// -*- c-basic-offset: 4 -*-

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

#include "Device.h"
#include <string>

// The Studio is where Midi and Audio devices live
// and where we can connect them together or to
// effects units or move them about or whatever.
//
// The Studio is a representation of what we can do
// to the devices and the sound in the Composition.
//
//

#ifndef _STUDIO_H_
#define _STUDIO_H_

namespace Rosegarden
{

class Studio
{

public:
    Studio();
    ~Studio();

    void createDevice(const std::string &name,
                      Device::DeviceType type);

private:

    std::vector<Device*> m_devices;

};

}

#endif // _STUDIO_H_
