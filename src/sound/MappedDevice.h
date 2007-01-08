// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-

/*
  Rosegarden
  A sequencer and musical notation editor.

  This program is Copyright 2000-2007
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
#include <vector>

#include <qdatastream.h>

#include "Device.h"
#include "MidiDevice.h"
#include "MappedCommon.h"

#ifndef _MAPPEDDEVICE_H_
#define _MAPPEDDEVICE_H_

// A DCOP wrapper to get MappedInstruments across to the GUI
//

namespace Rosegarden
{

class MappedInstrument;

class MappedDevice : public std::vector<MappedInstrument*>
{
public:
    MappedDevice();
    MappedDevice(DeviceId id,
                 Device::DeviceType type,
                 std::string name,
                 std::string connection = "");

    MappedDevice(const MappedDevice &mD);
    ~MappedDevice();
 
    // Clear down
    //
    void clear();

    MappedDevice& operator+(const MappedDevice &mD);
    MappedDevice& operator=(const MappedDevice &mD);

    friend QDataStream& operator>>(QDataStream &dS, MappedDevice *mD);
    friend QDataStream& operator<<(QDataStream &dS, MappedDevice *mD);
    friend QDataStream& operator>>(QDataStream &dS, MappedDevice &mD);
    friend QDataStream& operator<<(QDataStream &dS, const MappedDevice &mD);

    std::string getName() const { return m_name; }
    void setName(const std::string &name) { m_name = name; }

    DeviceId getId() const { return m_id; }
    void setId(DeviceId id) { m_id = id; }

    Device::DeviceType getType() const { return m_type; }
    void setType(Device::DeviceType type) { m_type = type; }

    std::string getConnection() const { return m_connection; }
    void setConnection(std::string connection) { m_connection = connection; }

    MidiDevice::DeviceDirection getDirection() const { return m_direction; }
    void setDirection(MidiDevice::DeviceDirection direction) { m_direction = direction; }
    
    bool isRecording() const { return m_recording; }
    void setRecording(bool recording) { m_recording = recording; }

protected:

    DeviceId            m_id;
    Device::DeviceType  m_type;
    std::string                     m_name;
    std::string                     m_connection;
    MidiDevice::DeviceDirection     m_direction;
    bool                            m_recording;
};

typedef std::vector<MappedInstrument*>::const_iterator
    MappedDeviceConstIterator;

typedef std::vector<MappedInstrument*>::iterator
    MappedDeviceIterator;

}

#endif // _MAPPEDDEVICE_H_

