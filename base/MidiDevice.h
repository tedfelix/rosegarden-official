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
#include "Instrument.h"

#ifndef _MIDIDEVICE_H_
#define _MIDIDEVICE_H_

namespace Rosegarden
{

class MidiDevice : public Device
{

public:
    MidiDevice();
    MidiDevice(const std::string &name);
    virtual ~MidiDevice();

    virtual void createInstruments();

private:
    // Brief (probably incorrect) synopsis of bank select 
    // messages for XG:
    //
    // MSB = 0 for Normal Voices
    // MSB = 64 for SFX
    // MSG = 126 for SFX Drum
    // MSG = 127 for Drum kits
    //
    // and then fiddle with LSB for individual banks
    //
    // For Soundblaster use MSG = 0 and then LSB to
    // required bank
    //
    //
    MidiByte m_bankMSB;   // Send as Controller 0
    MidiByte m_bankLSB;   // Send as Controller 32

    bool m_sendBankSelect; // send BS?

};

}

#endif // _MIDIDEVICE_H_
