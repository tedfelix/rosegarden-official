// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-

/*
  Rosegarden-4
  A sequencer and musical notation editor.

  This program is Copyright 2000-2003
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
                         const RealTime &eventTime,
                         const RealTime &duration):
       m_trackId(0),
       m_instrument(id),
       m_type(MidiNote),
       m_data1(0),
       m_data2(0),
       m_eventTime(eventTime),
       m_duration(duration),
       m_audioStartMarker(0, 0),
       m_dataBlock(""),
       m_isPersistent(false)
{
    try {

	// For each event type, we set the properties in a particular
	// order: first the type, then whichever of data1 and data2 fits
	// less well with its default value.  This way if one throws an
	// exception for no data, we still have a good event with the
	// defaults set.

	if (e.isa(Note::EventType))
	{
	    m_type = MidiNote;
	    long v = MidiMaxValue;
	    e.get<Int>(BaseProperties::VELOCITY, v);
	    m_data2 = v;
            m_data1 = e.get<Int>(BaseProperties::PITCH);
	}
	else if (e.isa(PitchBend::EventType))
	{
	    m_type = MidiPitchBend;
	    PitchBend pb(e);
	    m_data1 = pb.getMSB();
	    m_data2 = pb.getLSB();
	}
	else if (e.isa(Controller::EventType))
	{
	    m_type = MidiController;
	    Controller c(e);
	    m_data1 = c.getNumber();
	    m_data2 = c.getValue();
	}
	else if (e.isa(ProgramChange::EventType))
	{
	    m_type = MidiProgramChange;
	    ProgramChange pc(e);
	    m_data1 = pc.getProgram();
	}
	else if (e.isa(KeyPressure::EventType))
	{
	    m_type = MidiKeyPressure;
	    KeyPressure kp(e);
	    m_data1 = kp.getPitch();
	    m_data2 = kp.getPressure();
	}
	else if (e.isa(ChannelPressure::EventType))
	{
	    m_type = MidiChannelPressure;
	    ChannelPressure cp(e);
	    m_data1 = cp.getPressure();
	}
	else if (e.isa(SystemExclusive::EventType))
	{
	    m_type = MidiSystemExclusive;
	    SystemExclusive s(e);
	    m_dataBlock = s.getRawData();
	}
	else 
	{
	    m_type = InvalidMappedEvent;
	}
    } catch (MIDIValueOutOfRange r) {
	std::cerr << "MIDI value out of range in MappedEvent ctor"
		  << std::endl;
    } catch (Event::NoData d) {
	std::cerr << "Caught Event::NoData in MappedEvent ctor, message is:"
		  << std::endl << d.getMessage() << std::endl;
    } catch (Event::BadType b) {
	std::cerr << "Caught Event::BadType in MappedEvent ctor, message is:"
		  << std::endl << b.getMessage() << std::endl;
    } catch (SystemExclusive::BadEncoding e) {
	std::cerr << "Caught bad SysEx encoding in MappedEvent ctor"
		  << std::endl;
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
    if (&mE == this) return *this;

    m_trackId = mE.getTrackId();
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

const size_t MappedEvent::streamedSize = 11 * sizeof(unsigned int);

QDataStream&
operator<<(QDataStream &dS, MappedEvent *mE)
{
    dS << (unsigned int)mE->getTrackId();
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

    for (unsigned int i = 0; i < mE->getDataBlock().length(); i++)
        dS << (unsigned int)(mE->getDataBlock()[i]);

    return dS;
}

QDataStream&
operator<<(QDataStream &dS, const MappedEvent &mE)
{
    dS << (unsigned int)mE.getTrackId();
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

    for (unsigned int i = 0; i < mE.getDataBlock().length(); i++)
        dS << (unsigned int)(mE.getDataBlock()[i]);

    return dS;
}

QDataStream&
operator>>(QDataStream &dS, MappedEvent *mE)
{
    unsigned int trackId, instrument, type, data1, data2;
    long eventTimeSec, eventTimeUsec, durationSec, durationUsec,
         audioSec, audioUsec;
    std::string dataBlock("");
    unsigned int dataLength, dataElement;

    dS >> trackId;
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

    for (unsigned int i = 0; i < dataLength; i++)
    {
        dS >> dataElement;
        dataBlock += (char)dataElement;
    }

    mE->setTrackId((TrackId)trackId);
    mE->setInstrument((InstrumentId)instrument);
    mE->setType((MappedEvent::MappedEventType)type);
    mE->setData1((MidiByte)data1);
    mE->setData2((MidiByte)data2);
    mE->setEventTime(RealTime(eventTimeSec, eventTimeUsec));
    mE->setDuration(RealTime(durationSec, durationUsec));
    mE->setAudioStartMarker(RealTime(audioSec, audioUsec));
    mE->setDataBlock(dataBlock);

    return dS;
}

QDataStream&
operator>>(QDataStream &dS, MappedEvent &mE)
{
    unsigned int trackId, instrument, type, data1, data2;
    long eventTimeSec, eventTimeUsec, durationSec, durationUsec,
         audioSec, audioUsec;
    std::string dataBlock("");
    unsigned int dataLength, dataElement;
         
    dS >> trackId;
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

    for (unsigned int i = 0; i < dataLength; i++)
    {
        dS >> dataElement;
        dataBlock += dataElement;
    }

    mE.setTrackId((TrackId)trackId);
    mE.setInstrument((InstrumentId)instrument);
    mE.setType((MappedEvent::MappedEventType)type);
    mE.setData1((MidiByte)data1);
    mE.setData2((MidiByte)data2);
    mE.setEventTime(RealTime(eventTimeSec, eventTimeUsec));
    mE.setDuration(RealTime(durationSec, durationUsec));
    mE.setAudioStartMarker(RealTime(audioSec, audioUsec));
    mE.setDataBlock(dataBlock);

    return dS;
}

// Add a single byte to the DataBlock
//
void
MappedEvent::addDataByte(MidiByte byte)
{
    m_dataBlock += byte;
}

void 
MappedEvent::addDataString(const std::string &data)
{
    m_dataBlock += data;
}


}


