// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-

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

#include <string>
#include <vector>

#include <qdatastream.h>

#include "Device.h"

#ifndef _MAPPEDDEVICE_H_
#define _MAPPEDDEVICE_H_

// A DCOP wrapper to get MappedInstruments across to the GUI
//

namespace Rosegarden
{

class MappedInstrument;

class MappedDevice : public std::vector<Rosegarden::MappedInstrument*>
{
public:
    MappedDevice();
    MappedDevice(Rosegarden::DeviceId id,
                 Rosegarden::Device::DeviceType type,
                 const std::string &name, bool duplex);
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

    bool getDuplex() const { return m_duplex; }
    void setDuplex(bool duplex) { m_duplex = duplex; }

    std::string getName() const { return m_name; }
    void setName(const std::string &name) { m_name = name; }

    Rosegarden::DeviceId getId() const { return m_id; }
    void setId(Rosegarden::DeviceId id) { m_id = id; }

    Rosegarden::Device::DeviceType getType() const { return m_type; }
    void setType(Rosegarden::Device::DeviceType type) { m_type = type; }

    int getClient() const { return m_client; }
    void setClient(int client) { m_client = client; }

    int getPort() const { return m_port; }
    void setPort(int port) { m_port = port; }

protected:

    Rosegarden::DeviceId            m_id;
    Rosegarden::Device::DeviceType  m_type;
    std::string                     m_name;
    bool                            m_duplex;

    // Sequencer-local information - we don't need to transmit this anywhere.
    //
    int                             m_client;
    int                             m_port;
};

typedef std::vector<Rosegarden::MappedInstrument*>::const_iterator
    MappedDeviceConstIterator;

typedef std::vector<Rosegarden::MappedInstrument*>::iterator
    MappedDeviceIterator;

}

#endif // _MAPPEDDEVICE_H_

