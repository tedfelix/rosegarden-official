// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-

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

#include "MappedDevice.h"
#include "MappedInstrument.h"

namespace Rosegarden
{

MappedDevice::MappedDevice():
    std::vector<Rosegarden::MappedInstrument*>()
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

    return *this;
}


QDataStream&
operator>>(QDataStream &dS, MappedDevice *mD)
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

        mD->push_back(new MappedInstrument((Instrument::InstrumentType)type,
                                           (MidiByte)channel,
                                           (InstrumentId)id,
                                           std::string(name.data()),
                                           (DeviceId)device));

        instruments--;
    }

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
                                          std::string(name.data()),
                                          (DeviceId)device));

        instruments--;
    }

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

    return dS;
}


}

