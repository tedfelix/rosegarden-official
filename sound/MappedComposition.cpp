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
    for (MappedCompositionIterator it = this->begin(); it != this->end(); it++)
        this->erase((*it));
}



// Turn a MappedComposition into a QDataStream
//
QDataStream&
operator<<(QDataStream &dS, const MappedComposition &mC)
{
    dS << mC.size();

    for (MappedCompositionIterator it = mC.begin(); it != mC.end(); ++it )
    {
	dS << (*it)->getPitch();
	dS << (*it)->getAbsoluteTime().sec;
	dS << (*it)->getAbsoluteTime().usec;
	dS << (*it)->getDuration().sec;
	dS << (*it)->getDuration().usec;
	dS << (*it)->getAudioStartMarker().sec;
	dS << (*it)->getAudioStartMarker().usec;
	dS << (*it)->getVelocity();
	dS << (*it)->getInstrument();
        dS << (*it)->getTrack();
        dS << (*it)->getType();
    }

    return dS;
}


// Turn a QDataStream into a MappedComposition
//
QDataStream& 
operator>>(QDataStream &dS, MappedComposition &mC)
{
    int sliceSize, pitch, type, instrument, track, velocity;
    Rosegarden::RealTime absTime, duration, audioStartMarker;

    dS >> sliceSize;

    while (!dS.atEnd() && sliceSize)
    {
	dS >> pitch;
        dS >> absTime.sec;
	dS >> absTime.usec;
	dS >> duration.sec;
	dS >> duration.usec;
	dS >> audioStartMarker.sec;
	dS >> audioStartMarker.usec;
	dS >> velocity;
	dS >> instrument;
        dS >> track;
        dS >> type;

	mC.insert(new MappedEvent(pitch,
                                  absTime, duration, audioStartMarker,
                                  velocity, instrument, track,
                                  (MappedEvent::MappedEventType)type));

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
        (*it)->setAbsoluteTime((*it)->getAbsoluteTime() + mT);

    m_startTime = m_startTime + mT;
    m_endTime = m_endTime + mT;

}


MappedComposition
MappedComposition::operator+(const MappedComposition &c)
{
    for (MappedCompositionIterator it = c.begin(); it != c.end(); it++)
    {
        this->insert((*it));
    }

    return *this;
}






}


