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

#include "MappedDevice.h"
#include "MappedInstrument.h"
#include <iostream>

namespace Rosegarden
{

MappedDevice::MappedDevice():
    std::vector<Rosegarden::MappedInstrument*>(),
    m_id(0),
    m_type(Rosegarden::Device::Midi),
    m_name("MappedDevice default name"),
    m_duplex(false),
    m_client(-1),
    m_port(-1)
{
}

MappedDevice::MappedDevice(Rosegarden::DeviceId id,
                           Rosegarden::Device::DeviceType type,
                           const std::string &name, bool duplex):
    std::vector<Rosegarden::MappedInstrument*>(),
    m_id(id),
    m_type(type),
    m_name(name),
    m_duplex(duplex),
    m_client(-1),
    m_port(-1)
{
}

MappedDevice::~MappedDevice()
{
}

MappedDevice::MappedDevice(const MappedDevice &mD):
    std::vector<Rosegarden::MappedInstrument*>()
{
    clear();

    for (MappedDeviceConstIterator it = mD.begin(); it != mD.end(); it++)
        this->push_back(new MappedInstrument(*it));

    m_id = mD.getId();
    m_type = mD.getType();
    m_name = mD.getName();
    m_duplex = mD.getDuplex();
}

void
MappedDevice::clear()
{
    MappedDeviceIterator it;

    for (it = this->begin(); it != this->end(); it++)
        delete (*it);

    this->erase(this->begin(), this->end());
}

MappedDevice&
MappedDevice::operator+(const MappedDevice &mD)
{
    for (MappedDeviceConstIterator it = mD.begin(); it != mD.end(); it++)
        this->push_back(new MappedInstrument(*it));

    return *this;
}

MappedDevice&
MappedDevice::operator=(const MappedDevice &mD)
{
    clear();

    for (MappedDeviceConstIterator it = mD.begin(); it != mD.end(); it++)
        this->push_back(new MappedInstrument(*it));

    m_id = mD.getId();
    m_type = mD.getType();
    m_name = mD.getName();
    m_duplex = mD.getDuplex();

    return *this;
}


QDataStream&
operator>>(QDataStream &dS, MappedDevice *mD)
{
    int instruments = 0;
    dS >> instruments;

    unsigned int type = 0, channel = 0, id = 0, device = 0;
    QString name;

    while (!dS.atEnd() && instruments)
    {
        dS >> type;
        dS >> channel;
        dS >> id;
        dS >> name;
        dS >> device;

        mD->push_back(new MappedInstrument((Instrument::InstrumentType)type,
                                           (MidiByte)channel,
                                           (InstrumentId)id,
                                           name.utf8().data(),
                                           (DeviceId)device));

        instruments--;
    }

    // Name and duplex state
    //
    unsigned int duplex;
    int dType;
    dS >> id;
    dS >> dType;
    dS >> name;
    dS >> duplex;
    mD->setId(id);
    mD->setType(Rosegarden::Device::DeviceType(dType));
    mD->setName(std::string(name.data()));
    mD->setDuplex(bool(duplex));

    if (instruments)
    {
        std::cerr << "MappedDevice::operator>> - "
                  << "wrong number of events received" << std::endl;
    }

    return dS;
}


QDataStream&
operator>>(QDataStream &dS, MappedDevice &mD)
{
    int instruments;
    dS >> instruments;

    unsigned int type, channel, id, device;
    QString name;

    while (!dS.atEnd() && instruments)
    {
        dS >> type;
        dS >> channel;
        dS >> id;
        dS >> name;
        dS >> device;

        mD.push_back(new MappedInstrument((Instrument::InstrumentType)type,
                                          (MidiByte)channel,
                                          (InstrumentId)id,
                                          name.utf8().data(),
                                          (DeviceId)device));

        instruments--;
    }

    // Name and duplex state
    //
    unsigned int duplex;
    int dType;
    dS >> id;
    dS >> dType;
    dS >> name;
    dS >> duplex;
    mD.setId(id);
    mD.setType(Rosegarden::Device::DeviceType(dType));
    mD.setName(std::string(name.data()));
    mD.setDuplex(bool(duplex));

    if (instruments)
    {
        std::cerr << "MappedDevice::operator>> - "
                  << "wrong number of events received" << std::endl;
    }

    return dS;
}

QDataStream&
operator<<(QDataStream &dS, MappedDevice *mD)
{
    dS << mD->size();

    for (MappedDeviceIterator it = mD->begin(); it != mD->end(); it++)
    {
        dS << (unsigned int)(*it)->getType();
        dS << (unsigned int)(*it)->getChannel();
        dS << (unsigned int)(*it)->getId();
        dS << QString((*it)->getName().c_str());
        dS << (unsigned int)(*it)->getDevice();
    }

    // ID, name and duplex state
    //
    dS << (unsigned int)(mD->getId());
    dS << (int)(mD->getType());
    dS << QString(mD->getName().c_str());
    dS << (unsigned int)(mD->getDuplex());

    return dS;
}

QDataStream&
operator<<(QDataStream &dS, const MappedDevice &mD)
{
    dS << mD.size();

    for (MappedDeviceConstIterator it = mD.begin(); it != mD.end(); it++)
    {
        dS << (unsigned int)(*it)->getType();
        dS << (unsigned int)(*it)->getChannel();
        dS << (unsigned int)(*it)->getId();
        dS << QString((*it)->getName().c_str());
        dS << (unsigned int)(*it)->getDevice();
    }

    // ID, name and duplex state
    //
    dS << (unsigned int)(mD.getId());
    dS << (int)(mD.getType());
    dS << QString(mD.getName().c_str());
    dS << (unsigned int)(mD.getDuplex());

    return dS;
}


}

