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

#include <string>

#include "Device.h"
#include "Instrument.h"

#ifndef RG_AUDIODEVICE_H
#define RG_AUDIODEVICE_H

namespace Rosegarden
{

/// An AudioDevice defines Instruments where we can play our audio Segments.
class AudioDevice : public Device
{

public:
    AudioDevice();
    AudioDevice(DeviceId id, const std::string &name);
    virtual ~AudioDevice();

    // Copy constructor
    //
    AudioDevice(const AudioDevice &);

    virtual bool isOutput() const  { return true; }
    virtual bool isInput() const  { return false; }

    virtual void addInstrument(Instrument*);

    // An untainted Instrument we can use for playing previews
    //
    InstrumentId getPreviewInstrument();

    // Turn into XML string
    //
    virtual std::string toXmlString() const;

    virtual InstrumentList getAllInstruments() const { return m_instruments; }
    virtual InstrumentList getPresentationInstruments() const
        { return m_instruments; }

private:
    void createInstruments();
    void renameInstruments();
};

}

#endif // RG_AUDIODEVICE_H
