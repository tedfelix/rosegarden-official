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

namespace Rosegarden
{

MappedDevice::MappedDevice()
{
}

MappedDevice::~MappedDevice()
{
}


QDataStream&
operator>>(QDataStream &dS, MappedDevice *mD)
{
    int instruments;
    dS >> instruments;

    unsigned int type;
    unsigned int channel;
    unsigned int id;
    QString name;

    while (!dS.atEnd() && instruments)
    {
        dS >> type;
        dS >> channel;
        dS >> id;
        dS >> name;

        mD->push_back(new MappedInstrument((Instrument::InstrumentType)type,
                                           (MidiByte)channel,
                                           (InstrumentId)id,
                                           std::string(name.data())));

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

    unsigned int type;
    unsigned int channel;
    unsigned int id;
    QString name;

    while (!dS.atEnd() && instruments)
    {
        dS >> type;
        dS >> channel;
        dS >> id;
        dS >> name;

        mD.push_back(new MappedInstrument((Instrument::InstrumentType)type,
                                          (MidiByte)channel,
                                          (InstrumentId)id,
                                          std::string(name.data())));

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
    }

    return dS;
}

}

