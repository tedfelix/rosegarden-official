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

#include <qdatastream.h>
#include "MappedComposition.h"
#include "MappedEvent.h"
#include "SegmentPerformanceHelper.h"
#include <iostream>

namespace Rosegarden
{

using std::cerr;
using std::cout;
using std::endl;

MappedComposition::~MappedComposition()
{
    clear();
}

// copy constructor
MappedComposition::MappedComposition(const MappedComposition &mC):
  std::multiset<MappedEvent *, MappedEvent::MappedEventCmp>()
{
    clear();

    // deep copy
    for (MappedCompositionIterator it = mC.begin(); it != mC.end(); it++)
        this->insert(new MappedEvent(**it));

}

// Turn a MappedComposition into a QDataStream
//
QDataStream&
operator<<(QDataStream &dS, MappedComposition *mC)
{
    dS << mC->size();

    for (MappedCompositionIterator it = mC->begin(); it != mC->end(); ++it )
    {
	dS << (*it)->getInstrument();
        dS << (*it)->getType();
	dS << (*it)->getData1();  // can be pitch 
	dS << (*it)->getData2();  // can be velocity
	dS << (*it)->getEventTime().sec;
	dS << (*it)->getEventTime().usec;
	dS << (*it)->getDuration().sec;
	dS << (*it)->getDuration().usec;
	dS << (*it)->getAudioStartMarker().sec;
	dS << (*it)->getAudioStartMarker().usec;
    }

    return dS;
}


QDataStream&
operator<<(QDataStream &dS, const MappedComposition &mC)
{
    dS << mC.size();

    for (MappedCompositionIterator it = mC.begin(); it != mC.end(); ++it )
    {
	dS << (*it)->getInstrument();
        dS << (*it)->getType();
	dS << (*it)->getData1();  // can be pitch
	dS << (*it)->getData2();  // can be velocity
	dS << (*it)->getEventTime().sec;
	dS << (*it)->getEventTime().usec;
	dS << (*it)->getDuration().sec;
	dS << (*it)->getDuration().usec;
	dS << (*it)->getAudioStartMarker().sec;
	dS << (*it)->getAudioStartMarker().usec;
    }

    return dS;
}


// Turn a QDataStream into a MappedComposition
//
QDataStream& 
operator>>(QDataStream &dS, MappedComposition *mC)
{
    int sliceSize;

    // our conversion types
    Rosegarden::RealTime absTime, duration, audioStartMarker;
    MidiByte data1, data2;
    InstrumentId instrument;
    int type;

    dS >> sliceSize;

    while (!dS.atEnd() && sliceSize)
    {
	dS >> instrument;
        dS >> type;
	dS >> data1;
	dS >> data2;
        dS >> absTime.sec;
	dS >> absTime.usec;
	dS >> duration.sec;
	dS >> duration.usec;
	dS >> audioStartMarker.sec;
	dS >> audioStartMarker.usec;

        try
        {
	    mC->insert(new MappedEvent(instrument,
                                       (MappedEvent::MappedEventType)type,
                                       data1,
                                       data2,
                                       absTime,
                                       duration,
                                       audioStartMarker));
        }
        catch(...) {;}

	sliceSize--;

    }

    if (sliceSize)
    {
	cerr << "operator>> - wrong number of events received" << endl;
    }
    

    return dS;
}

QDataStream& 
operator>>(QDataStream &dS, MappedComposition &mC)
{
    int sliceSize;

    // our conversion types
    Rosegarden::RealTime absTime, duration, audioStartMarker;
    MidiByte data1, data2;
    InstrumentId instrument;
    int type;

    dS >> sliceSize;

    while (!dS.atEnd() && sliceSize)
    {
	dS >> instrument;
        dS >> type;
	dS >> data1;
	dS >> data2;
        dS >> absTime.sec;
	dS >> absTime.usec;
	dS >> duration.sec;
	dS >> duration.usec;
	dS >> audioStartMarker.sec;
	dS >> audioStartMarker.usec;

        try
        {
	    mC.insert(new MappedEvent(instrument,
                                      (MappedEvent::MappedEventType)type,
                                      data1,
                                      data2,
                                      absTime,
                                      duration,
                                      audioStartMarker));
        }
        catch(...) {;}

	sliceSize--;

    }

    if (sliceSize)
    {
	cerr << "operator>> - wrong number of events received" << endl;
    }
    

    return dS;
}

// Move the start time of this MappedComposition and all its events
//
//
void
MappedComposition::moveStartTime(const Rosegarden::RealTime &mT)
{
    MappedCompositionIterator it;

    for (it = this->begin(); it != this->end(); it++)
        (*it)->setEventTime((*it)->getEventTime() + mT);

    m_startTime = m_startTime + mT;
    m_endTime = m_endTime + mT;

}


// Concatenate MappedComposition
//
MappedComposition&
MappedComposition::operator+(const MappedComposition &mC)
{
    for (MappedCompositionIterator it = mC.begin(); it != mC.end(); it++)
        this->insert(new MappedEvent(**it)); // deep copy

    return *this;
}

// Assign (clear and deep copy)
//
MappedComposition&
MappedComposition::operator=(const MappedComposition &mC)
{
    clear();

    // deep copy
    for (MappedCompositionIterator it = mC.begin(); it != mC.end(); it++)
        this->insert(new MappedEvent(**it));

    return *this;
}

void
MappedComposition::clear()
{
    for (MappedCompositionIterator it = this->begin(); it != this->end(); it++)
        delete (*it);

    this->erase(this->begin(), this->end());
}



}


