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

#ifndef RG_DEVICE_H
#define RG_DEVICE_H

#include "XmlExportable.h"
#include "Instrument.h"
#include <string>
#include <vector>

// A Device can query underlying hardware/sound APIs to
// generate a list of Instruments.
//

namespace Rosegarden
{

typedef unsigned int DeviceId;

class Instrument;
typedef std::vector<Instrument *> InstrumentList;
class Controllable;
class AllocateChannels;
    
class Device : public XmlExportable
{
public:
    typedef enum 
    {
        Midi,
        Audio,
        SoftSynth
    } DeviceType;

    // special device ids
    static const DeviceId NO_DEVICE;
    static const DeviceId ALL_DEVICES;
    // The "external controller" ALSA port that we create.
    static const DeviceId EXTERNAL_CONTROLLER;

    Device(DeviceId id, const std::string &name, DeviceType type):
        m_name(name), m_type(type), m_id(id) { }

    ~Device() override;

    /**
     * Return a Controllable if we are a subtype that also inherits
     * from Controllable, otherwise return nullptr
     **/
    Controllable *getControllable();

    /**
     * Return our AllocateChannels if we are a subtype that tracks
     * free channels, otherwise return nullptr
     **/
    virtual AllocateChannels *getAllocator();

    void setType(DeviceType type) { m_type = type; }
    DeviceType getType() const { return m_type; }

    void setName(const std::string &name) { m_name = name; renameInstruments(); }
    std::string getName() const { return m_name; }

    void setId(DeviceId id) { m_id = id; }
    DeviceId getId() const { return m_id; }

    virtual bool isInput() const = 0;
    virtual bool isOutput() const = 0;

    // Accessing instrument lists - Devices should only
    // show the world what they want it to see
    //
    // Two functions - one to return all Instruments on a
    // Device - one to return all Instruments that a user
    // is allowed to select (Presentation Instruments).
    //
    virtual InstrumentList getAllInstruments() const = 0;
    virtual InstrumentList getPresentationInstruments() const = 0;

    /// Send channel setups to each instrument in the device.
    /**
     * This is mainly a MidiDevice thing.  Not sure if we should push it down.
     */
    void sendChannelSetups();

protected:
    virtual void addInstrument(Instrument *) = 0;
    virtual void renameInstruments() = 0;

    InstrumentList     m_instruments;
    std::string        m_name;
    DeviceType         m_type;
    DeviceId           m_id;
};

}

#endif // RG_DEVICE_H
