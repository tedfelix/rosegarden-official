// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2004
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

#include <string>

#include "Device.h"
#include "Instrument.h"

#ifndef _SOFT_SYNTH_DEVICE_H_
#define _SOFT_SYNTH_DEVICE_H_

namespace Rosegarden
{

class SoftSynthDevice : public Device
{

public:
    SoftSynthDevice();
    SoftSynthDevice(DeviceId id, const std::string &name);
    virtual ~SoftSynthDevice();

    // Copy constructor
    //
    SoftSynthDevice(const SoftSynthDevice &);

    virtual void addInstrument(Instrument*);

    // Turn into XML string
    //
    virtual std::string toXmlString(); 

    virtual InstrumentList getAllInstruments() const { return m_instruments; }
    virtual InstrumentList getPresentationInstruments() const
        { return m_instruments; }

private:

};

}

#endif
