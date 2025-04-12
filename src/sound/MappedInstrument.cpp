/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of the
  License, or (at your option) any later version.  See the file
  COPYING included with this distribution for more information.
*/

#include "MappedInstrument.h"
#include <misc/Strings.h>

namespace Rosegarden
{


MappedInstrument::MappedInstrument():
        m_type(Instrument::Midi),
        m_id(0),
        m_name(std::string("")),
        m_device(0),
        m_audioChannels(0)
{}

MappedInstrument::MappedInstrument(Instrument::InstrumentType type,
                                   MidiByte /*channel*/,
                                   InstrumentId id):
        m_type(type),
        m_id(id),
        m_name(std::string("")),
        m_device(0),
        m_audioChannels(0)
{}

MappedInstrument::MappedInstrument(Instrument::InstrumentType type,
                                   MidiByte /*channel*/,
                                   InstrumentId id,
                                   const std::string &name,
                                   DeviceId device):
        m_type(type),
        m_id(id),
        m_name(name),
        m_device(device),
        m_audioChannels(0)
{}

MappedInstrument::MappedInstrument(const Instrument &instrument):
        m_type(instrument.getType()),
        m_id(instrument.getId()),
        m_name(instrument.getName()),
        m_device((instrument.getDevice())->getId()),
        m_audioChannels(instrument.getNumAudioChannels())
{}

MappedInstrument::MappedInstrument(Instrument *instrument):
        m_type(instrument->getType()),
        m_id(instrument->getId()),
        m_name(instrument->getName()),
        m_device(instrument->getDevice()->getId()),
        m_audioChannels(instrument->getNumAudioChannels())
{}

/* unused
QDataStream&
operator>>(QDataStream &dS, MappedInstrument *mI)
{
    unsigned int type, channel, id, device, audioChannels;
    QString name;

    dS >> type;
    dS >> channel;
    dS >> id;
    dS >> name;
    dS >> device;
    dS >> audioChannels;

    mI->setType(Instrument::InstrumentType(type));
    mI->setId(InstrumentId(id));
	mI->setName( qStrToStrLocal8(name) );
    mI->setDevice(DeviceId(device));
    mI->setAudioChannels(audioChannels);

    return dS;
}
*/

/* unused
QDataStream&
operator>>(QDataStream &dS, MappedInstrument &mI)
{
    unsigned int type, channel, id, device, audioChannels;
    QString name;

    dS >> type;
    dS >> channel;
    dS >> id;
    dS >> name;
    dS >> device;
    dS >> audioChannels;

    mI.setType(Instrument::InstrumentType(type));
    mI.setId(InstrumentId(id));
	mI.setName( qStrToStrLocal8(name) );
    mI.setDevice(DeviceId(device));
    mI.setAudioChannels(audioChannels);

    return dS;
}
*/

QDataStream&
operator<<(QDataStream &dS, MappedInstrument *mI)
{
    dS << (unsigned int)mI->getType();
    dS << (unsigned int)-1;
    dS << (unsigned int)mI->getId();
    ;
    dS << QString(mI->getName().c_str());
    dS << (unsigned int)mI->getDevice();
    dS << (unsigned int)mI->getAudioChannels();

    return dS;
}


QDataStream&
operator<<(QDataStream &dS, const MappedInstrument &mI)
{
    dS << (unsigned int)mI.getType();
    dS << (unsigned int)-1;
    dS << (unsigned int)mI.getId();
    ;
    dS << QString(mI.getName().c_str());
    dS << (unsigned int)mI.getDevice();
    dS << (unsigned int)mI.getAudioChannels();

    return dS;
}

}
