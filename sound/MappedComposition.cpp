// -*- c-basic-offset: 4 -*-

/*
  Rosegarden-4 v0.1
  A sequencer and musical notation editor.

  This program is Copyright 2000-2001
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
#include "TrackPerformanceHelper.h"
#include <iostream>

namespace Rosegarden
{

using std::cerr;
using std::cout;
using std::endl;

// We use some globals here for speed - we're
// making a lot of these conversions when sending
// this class over DCOP
//
MappedCompositionIterator it;
MappedEvent               *insertEvent;
int                       sliceSize;
int                       pitch;
timeT                     absTime;
timeT                     duration;
instrumentT               instrument;
velocityT                 velocity;



// Converts a Rosegarden Composition into a MappedComposition
// suitable for the RosegardenSequencer to quickly iterate
// through.  Playback Event filters etc. can go in at this
// stage.  [rwb]
//
MappedComposition::MappedComposition(Rosegarden::Composition &comp,
                                     const Rosegarden::timeT &sT,
                                     const Rosegarden::timeT &eT):
    m_startTime(sT),
    m_endTime(eT)
{
    assert(m_endTime >= m_startTime);
    
    Rosegarden::timeT eventTime;

    for (Composition::iterator i = comp.begin(); i != comp.end(); i++ )
    {
	// Skip the Track if it starts too late to be of
	// interest to our slice.
	if ( (*i)->getStartIndex() > int(m_endTime) )
	    continue;

	TrackPerformanceHelper helper(**i);

	for ( Track::iterator j = (*i)->begin(); j != (*i)->end(); j++ )
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
	    timeT duration = helper.getSoundingDuration(j);

	    if (duration == 0) // probably in a tied series, but not as first note
		continue;

	    // get the eventTime
	    eventTime = (unsigned int) (*j)->getAbsoluteTime();

	    // As events are stored chronologically we can escape if
	    // we're already beyond our event horizon for this slice.
	    //
	    if ( eventTime > m_endTime )
		break;

	    // Eliminate events before our required time
	    if ( eventTime >= m_startTime && eventTime <= m_endTime)
	    {
		// insert event
		MappedEvent *me = new MappedEvent(**j, duration);
		me->setInstrument((*i)->getInstrument());
		this->insert(me);
	    }
	}
    }
}



// turn a MappedComposition into a data stream
//
QDataStream&
operator<<(QDataStream &dS, const MappedComposition &mC)
{
  
    dS << mC.size();

    for ( it = mC.begin(); it != mC.end(); ++it )
    {
	dS << (*it)->getPitch();
	dS << (*it)->getAbsoluteTime();
	dS << (*it)->getDuration();
	dS << (*it)->getVelocity();
	dS << (*it)->getInstrument();
    }

    return dS;
}


// turn a data stream into a MappedComposition
//
QDataStream& 
operator>>(QDataStream &dS, MappedComposition &mC)
{
    dS >> sliceSize;

    while (!dS.atEnd() && sliceSize)
    {
	dS >> pitch;
	dS >> absTime;
	dS >> duration;
	dS >> velocity;
	dS >> instrument;

	insertEvent = new MappedEvent(pitch, absTime, duration,
				      velocity, instrument);
	mC.insert(insertEvent);

	sliceSize--;

    }

    if (sliceSize)
    {
	cerr << "operator>> - wrong number of events received" << endl;
    }
    

    return dS;
}



}


