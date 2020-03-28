/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_SOFT_SYNTH_DEVICE_H
#define RG_SOFT_SYNTH_DEVICE_H

#include <string>

#include "Device.h"
#include "Instrument.h"
#include "Controllable.h"
#include "MidiMetronome.h"

namespace Rosegarden
{

class SoftSynthDevice : public Device, public Controllable
{

public:
    SoftSynthDevice();
    SoftSynthDevice(DeviceId id, const std::string &name);
    ~SoftSynthDevice() override;

    bool isOutput() const  override { return true; }
    bool isInput() const  override { return false; }

    void addInstrument(Instrument*) override;

    // Turn into XML string
    //
    std::string toXmlString() const override;

    InstrumentList getAllInstruments() const override { return m_instruments; }
    InstrumentList getPresentationInstruments() const override
        { return m_instruments; }

    // implemented from Controllable interface
    //
    const ControlList &getControlParameters() const override { return m_controlList; }
    const ControlParameter *getControlParameter(int index) const override;
    const ControlParameter *getControlParameter(const std::string &type,
                                                        MidiByte controllerNumber) const override;
    void setMetronome(const MidiMetronome &);
    const MidiMetronome* getMetronome() const { return m_metronome; }

private:
    // Hide copy constructor and op=
    SoftSynthDevice(const SoftSynthDevice &);
    SoftSynthDevice &operator=(const SoftSynthDevice &);

    MidiMetronome *m_metronome;
    static ControlList m_controlList;
    static void checkControlList();
    void createInstruments();
    void renameInstruments() override;
};

}

#endif
