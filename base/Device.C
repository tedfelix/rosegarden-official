// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
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

namespace Rosegarden
{

#ifndef EXPERIMENTAL_ALSA_DRIVER

// Find all the unique port numbers in the Instrument list and return them.
//
//
std::vector<int>
Device::getPortNumbers() const
{
    std::vector<int> portNumbers;
    std::vector<int>::iterator pIt;

    bool addPort = false;
    InstrumentList::const_iterator it = m_instruments.begin();
    for (; it != m_instruments.end(); ++it)
    {
        addPort = true;

        // There's probably a more efficient STL way of doing this
        // but I can't be bothered to look it up.
        //
        for (pIt = portNumbers.begin(); pIt != portNumbers.end(); ++pIt)
        {
            if ((*pIt) == (*it)->getPort())
            {
                addPort = false;
                continue;
            }
        }

        // Only add positive port numbers
        //
        if ((*it)->getPort() >= 0 && addPort)
            portNumbers.push_back((*it)->getPort());
    }

    return portNumbers;
}

// Return the position of the port number in the total instrument
// hierarchy for this device.
//
int
Device::getPortNumberPosition(int port) const
{
    int position = -1;
    int lastPort = -1;

    InstrumentList::const_iterator it = m_instruments.begin();
    for (; it != m_instruments.end(); ++it)
    {
        if (lastPort != (*it)->getPort())
        {
            lastPort = (*it)->getPort();
            position++;
        }

        if (port == (*it)->getPort()) break;
    }

    return position;

}

#endif

}
