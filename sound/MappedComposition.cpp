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

// We declare some globals here just for speed - we're
// making a lot of these conversions when sending this
// class over DCOP so if we keep the top level object
// reasonably persistent we can save some overhead.
//
//
MappedCompositionIterator it;
MappedEvent               *insertEvent;
int                       sliceSize;
int                       pitch;
Rosegarden::RealTime      absTime;
Rosegarden::RealTime      duration;
trackT                    track;
velocityT                 velocity;



// Converts a Rosegarden Composition into a MappedComposition
// suitable for the RosegardenSequencer to quickly iterate
// through.  Playback Event filters etc. can go in at this
// stage.  [rwb]
//
MappedComposition::MappedComposition(Rosegarden::Composition &comp,
                                     const Rosegarden::RealTime &sT,
                                     const Rosegarden::RealTime &eT):
    m_startTime(sT),
    m_endTime(eT)
{
    assert(m_endTime >= m_startTime);
    
    Rosegarden::RealTime eventTime;
    Rosegarden::RealTime duration;

    for (Composition::iterator i = comp.begin(); i != comp.end(); i++ )
    {
	// Skip the Segment if it starts too late to be of
	// interest to our slice.
	if ( comp.getElapsedRealTime((*i)->getStartIndex()) > m_endTime )
	    continue;

	SegmentPerformanceHelper helper(**i);

	for ( Segment::iterator j = (*i)->begin(); j != (*i)->end(); j++ )
	{
	    // for the moment ensure we're all positive
	    assert((*j)->getAbsoluteTime() >= 0 );

	    // Skip this event if it isn't a note
	    //
	    if (!(*j)->isa(Note::EventType))
		continue;

	    // Find the performance duration, i.e. taking into account any
	    // ties etc that this note may have  --cc
	    // 
	    duration = helper.getRealSoundingDuration(j);

	    if (duration > Rosegarden::RealTime(0, 0)) // probably in a tied series, but not as first note
		continue;

	    // get the eventTime
	    eventTime = comp.getElapsedRealTime((*j)->getAbsoluteTime());

	    // As events are stored chronologically we can escape if
	    // we're already beyond our event horizon for this slice.
	    //
	    if ( eventTime > m_endTime )
		break;

	    // Eliminate events before our required time
	    if ( eventTime >= m_startTime && eventTime <= m_endTime)
	    {
		// insert event
		MappedEvent *me = new MappedEvent(**j, eventTime, duration);
		me->setTrack((*i)->getTrack());
		this->insert(me);
	    }
	}
    }
}



// Turn a MappedComposition into a QDataStream
//
QDataStream&
operator<<(QDataStream &dS, const MappedComposition &mC)
{
  
    dS << mC.size();

    for ( it = mC.begin(); it != mC.end(); ++it )
    {
	dS << (*it)->getPitch();
	dS << (*it)->getAbsoluteTime().sec;
	dS << (*it)->getAbsoluteTime().usec;
	dS << (*it)->getDuration().sec;
	dS << (*it)->getDuration().usec;
	dS << (*it)->getVelocity();
	dS << (*it)->getTrack();
    }

    return dS;
}


// Turn a QDataStream into a MappedComposition
//
QDataStream& 
operator>>(QDataStream &dS, MappedComposition &mC)
{
    dS >> sliceSize;

    while (!dS.atEnd() && sliceSize)
    {
	dS >> pitch;
        dS >> absTime.sec;
	dS >> absTime.usec;
	dS >> duration.sec;
	dS >> duration.usec;
	dS >> velocity;
	dS >> track;

	insertEvent = new MappedEvent(pitch, absTime, duration,
				      velocity, track);
	mC.insert(insertEvent);

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
    for (it = this->begin(); it != this->end(); it++)
    {
        (*it)->setAbsoluteTime((*it)->getAbsoluteTime() + mT);
        cout << "NEW ST = " << (*it)->getAbsoluteTime() << endl;
    }

    m_startTime = m_startTime + mT;
    m_endTime = m_endTime + mT;

}


MappedComposition
MappedComposition::operator+(const MappedComposition &c)
{
    for (it = c.begin(); it != c.end(); it++)
    {
        this->insert((*it));
    }

    return *this;
}






}


