// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2006
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

#ifndef _SOFT_SYNTH_DEVICE_H_
#define _SOFT_SYNTH_DEVICE_H_

#include <string>

#include "Device.h"
#include "Instrument.h"
#include "Controllable.h"

namespace Rosegarden
{

class SoftSynthDevice : public Device, public Controllable
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

    // implemented from Controllable interface
    //
    virtual const ControlList &getControlParameters() const { return m_controlList; }
    virtual const ControlParameter *getControlParameter(int index) const;
    virtual const ControlParameter *getControlParameter(const std::string &type,
							Rosegarden::MidiByte controllerNumber) const;

private:
    static ControlList m_controlList;
    static void checkControlList();
};

}

#endif
