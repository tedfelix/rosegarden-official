
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

namespace Rosegarden
{

using std::cerr;
using std::cout;
using std::endl;

// We use some globals here for speed - we're
// making a lot of these conversions.
//
MappedCompositionIterator it;
MappedEvent               *insertEvent;
int                       sliceSize;
int                       pitch;
timeT                     absTime;
timeT                     duration;
instrumentT               instrument;
velocityT                 velocity;

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


