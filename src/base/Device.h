/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.
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
#include <list>

// A Device can query underlying hardware/sound APIs to
// generate a list of Instruments.
//

namespace Rosegarden
{


class Composition;
class Instrument;
class Controllable;
class AllocateChannels;
class DeviceObserver;

typedef unsigned int DeviceId;
typedef std::vector<Instrument *> InstrumentList;

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
      m_name(name), m_type(type), m_id(id), m_notificationsBlocked(false) { }

    ~Device() override;

    /**
     * Return a Controllable if we are a subtype that also inherits
     * from Controllable, otherwise return nullptr
     **/
    const Controllable *getControllable() const;

    /**
     * Return our AllocateChannels if we are a subtype that tracks
     * free channels, otherwise return nullptr
     **/
    virtual AllocateChannels *getAllocator() const;

    void setType(DeviceType type) { m_type = type; notifyDeviceModified(); }
    DeviceType getType() const { return m_type; }

    void setName(const std::string &name) { m_name = name; renameInstruments(); }
    std::string getName() const { return m_name; }

    void setId(DeviceId id) { m_id = id; notifyDeviceModified(); }
    DeviceId getId() const { return m_id; }

    virtual bool isInput() const = 0;
    virtual bool isOutput() const = 0;

    /// All Instruments on a Device.
    virtual InstrumentList getAllInstruments() const = 0;
    /// All Instruments that a user is allowed to select.
    /**
     * For SoftSynthDevice and AudioDevice, this is the same as
     * getAllInstruments().
     *
     * For MidiDevice, this is different.  It omits the "special" Instruments.
     * Any Instrument with an ID less than MidiInstrumentBase is dropped from
     * this list.  See MidiDevice::generatePresentationList().
     */
    virtual InstrumentList getPresentationInstruments() const = 0;
    /// Returns an InstrumentId that is not currently on a Track.
    /**
     * composition can be specified when working with a new Composition
     * that isn't "current" yet (e.g. during import).  Specify nullptr to
     * use the current Composition.
     */
    InstrumentId getAvailableInstrument(
            const Composition *composition = nullptr) const;

    /// Send channel setups to each instrument in the device.
    /**
     * This is mainly a MidiDevice thing.  Not sure if we should push it down.
     */
    void sendChannelSetups() const;

    /// Observer management
    void addObserver(DeviceObserver *obs);
    void removeObserver(DeviceObserver *obs);

    virtual void blockNotify(bool block);

protected:
    virtual void addInstrument(Instrument *) = 0;
    virtual void renameInstruments() = 0;

    void notifyDeviceModified();

    InstrumentList     m_instruments;
    std::string        m_name;
    DeviceType         m_type;
    DeviceId           m_id;

    bool m_notificationsBlocked;

 private:
    typedef std::list<DeviceObserver *> ObserverList;
    ObserverList m_observers;
};

class DeviceObserver
{
 public:
    virtual ~DeviceObserver() {}

    virtual void deviceModified(Device*) { }

};

}

#endif // RG_DEVICE_H
