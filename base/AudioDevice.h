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

#include <string>

#include "Device.h"

// An AudioDevice defines Instruments where we can play our
// audio Segments.
//
//
//
#ifndef _AUDIODEVICE_H_
#define _AUDIODEVICE_H_

namespace Rosegarden
{

class AudioDevice : public Device
{

public:
    AudioDevice();
    AudioDevice(const std::string &name);
    virtual ~AudioDevice();

    virtual void createInstruments();
    virtual void addInstrument(Instrument*);

    // Turn into XML string
    //
    virtual std::string toXmlString(); 

    virtual InstrumentList& getAllInstruments() { return m_instruments; }
    virtual InstrumentList& getPresentationInstruments()
        { return m_instruments; }

private:

};

}

#endif // _AUDIODEVICE_H_
