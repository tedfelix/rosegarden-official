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
       m_audioStartMarker(0, 0)
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
    }
    else if (e.isa(Rosegarden::PitchBend::EventType))
    {
        if (e.has(Rosegarden::PitchBend::MSB))
            m_data1 = e.get<Int>(Rosegarden::PitchBend::LSB);
        else
            m_data1 = 0;

        if (e.has(Rosegarden::PitchBend::LSB))
            m_data2 = e.get<Int>(Rosegarden::PitchBend::LSB);
        else
            m_data2 = 0;
    }
    else
    {
        std::cerr << "MappedEvent::MappedEvent - "
                  << "can't handle ("
                  << e.getType() << ") internal event yet" << std::endl;
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

    return dS;
}

QDataStream&
operator>>(QDataStream &dS, MappedEvent *mE)
{
    unsigned int instrument, type, data1, data2;
    long eventTimeSec, eventTimeUsec, durationSec, durationUsec,
         audioSec, audioUsec;
         
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

    mE->setInstrument((InstrumentId)instrument);
    mE->setType((MappedEvent::MappedEventType)type);
    mE->setData1((Rosegarden::MidiByte)data1);
    mE->setData2((Rosegarden::MidiByte)data2);
    mE->setEventTime(Rosegarden::RealTime(eventTimeSec, eventTimeUsec));
    mE->setDuration(Rosegarden::RealTime(durationSec, durationUsec));
    mE->setAudioStartMarker(Rosegarden::RealTime(audioSec, audioUsec));

    return dS;
}

QDataStream&
operator>>(QDataStream &dS, MappedEvent &mE)
{
    unsigned int instrument, type, data1, data2;
    long eventTimeSec, eventTimeUsec, durationSec, durationUsec,
         audioSec, audioUsec;
         
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

    mE.setInstrument((InstrumentId)instrument);
    mE.setType((MappedEvent::MappedEventType)type);
    mE.setData1((Rosegarden::MidiByte)data1);
    mE.setData2((Rosegarden::MidiByte)data2);
    mE.setEventTime(Rosegarden::RealTime(eventTimeSec, eventTimeUsec));
    mE.setDuration(Rosegarden::RealTime(durationSec, durationUsec));
    mE.setAudioStartMarker(Rosegarden::RealTime(audioSec, audioUsec));

    return dS;
}


}

