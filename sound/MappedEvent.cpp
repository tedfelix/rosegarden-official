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

#include "MappedEvent.h"
#include "BaseProperties.h"
#include "MidiTypes.h"
 
namespace Rosegarden
{

MappedEvent::MappedEvent(InstrumentId id,
                         const Event &e,
                         const Rosegarden::RealTime &eventTime,
                         const Rosegarden::RealTime &duration):
       m_instrument(id),
       m_type(MidiNote),
       m_data1(0),
       m_data2(MidiMaxValue),
       m_eventTime(eventTime),
       m_duration(duration),
       m_audioStartMarker(0, 0),
       m_dataBlock("")
{
    if (e.isa(Rosegarden::Note::EventType))
    {
        if (e.get<Int>(BaseProperties::PITCH))
            m_data1 = e.get<Int>(BaseProperties::PITCH);
        else
            m_data1 = 0;

        if (e.has(BaseProperties::VELOCITY))
        {
	    try {
	        m_data2 = e.get<Int>(BaseProperties::VELOCITY);
	    }
	    catch(...)
	    {
	        m_data2 = MidiMaxValue;
	    }
        }

        m_type = MidiNote;
    }
    else if (e.isa(Rosegarden::PitchBend::EventType))
    {
        if (e.has(Rosegarden::PitchBend::MSB))
            m_data1 = e.get<Int>(Rosegarden::PitchBend::MSB);
        else
            m_data1 = 0;

        if (e.has(Rosegarden::PitchBend::LSB))
            m_data2 = e.get<Int>(Rosegarden::PitchBend::LSB);
        else
            m_data2 = 0;

        m_type = MidiPitchBend;
    }
    else if (e.isa(Rosegarden::Controller::EventType))
    {
        if (e.has(Rosegarden::Controller::DATA1))
            m_data1 = e.get<Int>(Rosegarden::Controller::DATA1);
        else
            m_data1 = 0;

        if (e.has(Rosegarden::Controller::DATA2))
            m_data2 = e.get<Int>(Rosegarden::Controller::DATA2);
        else
            m_data2 = 0;

        m_type = MidiController;
    }
    else if (e.isa(Rosegarden::ProgramChange::EventType))
    {
        if (e.has(Rosegarden::ProgramChange::PROGRAM))
            m_data1 = e.get<Int>(Rosegarden::ProgramChange::PROGRAM);
        else
            m_data1 = 0;

        m_type = MidiProgramChange;
    }
    else if (e.isa(Rosegarden::KeyPressure::EventType))
    {
        if (e.has(Rosegarden::KeyPressure::PITCH))
            m_data1 = e.get<Int>(Rosegarden::KeyPressure::PITCH);
        else
            m_data1 = 0;

        if (e.has(Rosegarden::KeyPressure::PRESSURE))
            m_data2 = e.get<Int>(Rosegarden::KeyPressure::PRESSURE);
        else
            m_data2 = 0;

        m_type = MidiKeyPressure;
    }
    else if (e.isa(Rosegarden::ChannelPressure::EventType))
    {
        if (e.has(Rosegarden::ChannelPressure::PRESSURE))
            m_data1 = e.get<Int>(Rosegarden::ChannelPressure::PRESSURE);
        else
            m_data1 = 0;

        m_type = MidiChannelPressure;
    }
    else if (e.isa(Rosegarden::SystemExclusive::EventType))
    {
        m_type = MidiSystemExclusive;
    }
    else
    {
        throw(std::string("unsupported MappedEvent type"));
    }
}

bool
operator<(const MappedEvent &a, const MappedEvent &b)
{
    return a.getEventTime() < b.getEventTime();
}

MappedEvent&
MappedEvent::operator=(const MappedEvent &mE)
{
    m_instrument = mE.getInstrument();
    m_type = mE.getType();
    m_data1 = mE.getData1();
    m_data2 = mE.getData2();
    m_eventTime = mE.getEventTime();
    m_duration = mE.getDuration();
    m_audioStartMarker = mE.getAudioStartMarker();
    m_dataBlock = mE.getDataBlock();

    return *this;
}

QDataStream&
operator<<(QDataStream &dS, MappedEvent *mE)
{
    dS << (unsigned int)mE->getInstrument();
    dS << (unsigned int)mE->getType();
    dS << (unsigned int)mE->getData1();
    dS << (unsigned int)mE->getData2();
    dS << (unsigned int)mE->getEventTime().sec;
    dS << (unsigned int)mE->getEventTime().usec;
    dS << (unsigned int)mE->getDuration().sec;
    dS << (unsigned int)mE->getDuration().usec;
    dS << (unsigned int)mE->getAudioStartMarker().sec;
    dS << (unsigned int)mE->getAudioStartMarker().usec;

    dS << (unsigned int)mE->getDataBlock().length();

    for (unsigned int i = 0; mE->getDataBlock().length(); i++)
        dS << (unsigned int)mE->getDataBlock()[i];

    return dS;
}

QDataStream&
operator<<(QDataStream &dS, MappedEvent &mE)
{
    dS << (unsigned int)mE.getInstrument();
    dS << (unsigned int)mE.getType();
    dS << (unsigned int)mE.getData1();
    dS << (unsigned int)mE.getData2();
    dS << (unsigned int)mE.getEventTime().sec;
    dS << (unsigned int)mE.getEventTime().usec;
    dS << (unsigned int)mE.getDuration().sec;
    dS << (unsigned int)mE.getDuration().usec;
    dS << (unsigned int)mE.getAudioStartMarker().sec;
    dS << (unsigned int)mE.getAudioStartMarker().usec;

    dS << (unsigned int)mE.getDataBlock().length();

    for (unsigned int i = 0; mE.getDataBlock().length(); i++)
        dS << (unsigned int)mE.getDataBlock()[i];

    return dS;
}

QDataStream&
operator>>(QDataStream &dS, MappedEvent *mE)
{
    unsigned int instrument, type, data1, data2, dataElement;
    long eventTimeSec, eventTimeUsec, durationSec, durationUsec,
         audioSec, audioUsec;
    std::string dataBlock;
    unsigned int dataLength;

    dS >> instrument;
    dS >> type;
    dS >> data1;
    dS >> data2;
    dS >> eventTimeSec;
    dS >> eventTimeUsec;
    dS >> durationSec;
    dS >> durationUsec;
    dS >> audioSec;
    dS >> audioUsec;

    dS >> dataLength;
    for (unsigned int i = 0; dataLength; i++)
    {
        dS >> dataElement;
        dataBlock += (unsigned char)dataElement;
    }

    mE->setInstrument((InstrumentId)instrument);
    mE->setType((MappedEvent::MappedEventType)type);
    mE->setData1((Rosegarden::MidiByte)data1);
    mE->setData2((Rosegarden::MidiByte)data2);
    mE->setEventTime(Rosegarden::RealTime(eventTimeSec, eventTimeUsec));
    mE->setDuration(Rosegarden::RealTime(durationSec, durationUsec));
    mE->setAudioStartMarker(Rosegarden::RealTime(audioSec, audioUsec));
    mE->setDataBlock(dataBlock);

    return dS;
}

QDataStream&
operator>>(QDataStream &dS, MappedEvent &mE)
{
    unsigned int instrument, type, data1, data2, dataElement;
    long eventTimeSec, eventTimeUsec, durationSec, durationUsec,
         audioSec, audioUsec;
    std::string dataBlock;
    unsigned int dataLength;
         
    dS >> instrument;
    dS >> type;
    dS >> data1;
    dS >> data2;
    dS >> eventTimeSec;
    dS >> eventTimeUsec;
    dS >> durationSec;
    dS >> durationUsec;
    dS >> audioSec;
    dS >> audioUsec;

    dS >> dataLength;
    for (unsigned int i = 0; dataLength; i++)
    {
        dS >> dataElement;
        dataBlock += (unsigned char)dataElement;
    }

    mE.setInstrument((InstrumentId)instrument);
    mE.setType((MappedEvent::MappedEventType)type);
    mE.setData1((Rosegarden::MidiByte)data1);
    mE.setData2((Rosegarden::MidiByte)data2);
    mE.setEventTime(Rosegarden::RealTime(eventTimeSec, eventTimeUsec));
    mE.setDuration(Rosegarden::RealTime(durationSec, durationUsec));
    mE.setAudioStartMarker(Rosegarden::RealTime(audioSec, audioUsec));
    mE.setDataBlock(dataBlock);

    return dS;
}


}

