
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

#include "Composition.h"
#include "Track.h"

namespace Rosegarden 
{

Composition::Composition()
    : m_nbTicksPerBar(384),
      m_tempo(0)
{
//     cerr << "Composition:(" << nbTracks << ") : this = "
//          << this <<  " - size = "
//          << m_tracks.size() << endl;
}

Composition::~Composition()
{
    clear();
}

void Composition::swap(Composition& c)
{
    unsigned int t = m_nbTicksPerBar;
    m_nbTicksPerBar = c.m_nbTicksPerBar;
    c.m_nbTicksPerBar = t;

    t = m_tempo;
    m_tempo = c.m_tempo;
    c.m_tempo = t;

    m_tracks.swap(c.m_tracks);
}



Composition::iterator
Composition::addTrack(Track *track)
{
    if (!track) return end();
    
    std::pair<iterator, bool> res = m_tracks.insert(track);

    return res.first;
}

void
Composition::deleteTrack(Composition::iterator i)
{
    if (i == end()) return;

    Track *p = (*i);
    delete p;
    m_tracks.erase(i);
}

bool
Composition::deleteTrack(Track *p)
{
    iterator i = find(begin(), end(), p);
    
    if (i != end()) {
        delete p;
        m_tracks.erase(i);
        return true;
    }

    return false;
}

unsigned int
Composition::getDuration() const
{
    unsigned int maxDuration = 0;

    for (trackcontainer::const_iterator i = m_tracks.begin();
         i != m_tracks.end(); ++i) {

        unsigned int trackTotal = (*i)->getDuration() + (*i)->getStartIndex();
        
        if ((*i) && trackTotal > maxDuration) {
            maxDuration = trackTotal;
        }
    }
    
    return maxDuration;
}

void
Composition::clear()
{
    for(trackcontainer::iterator i = m_tracks.begin();
        i != m_tracks.end(); ++i) {
        delete (*i);
    }
    m_tracks.erase(begin(), end());
    
}



// Create a Mapped Composition over a slice of time - used by
// the Sequencer.
//
MappedComposition*
Composition::getMappedComposition(const unsigned int &sliceStart,
                                  const unsigned int &sliceEnd)
{
  MappedComposition *returnSlice = new MappedComposition(sliceStart, sliceEnd);
  unsigned int eventTime;

  assert(sliceEnd >= sliceStart);

  for (Composition::iterator i = begin(); i != end(); i++ )
  {
    if ( (*i)->getStartIndex() >= int(sliceEnd) )
      continue;

    for ( Track::iterator j = (*i)->begin(); j != (*i)->end(); j++ )
    {
      // for the moment ensure we're all positive
      //assert((*j)->getAbsoluteTime() >= 0 );

      // Skip this event if it doesn't have pitch
      // (check that the event has "velocity" soon as well)
      //
      if (!(*j)->has("pitch"))
        continue;

      // get the eventTime
      eventTime = (unsigned int) (*j)->getAbsoluteTime();
  
      // eventually filter only for the events we're interested in
      if ( eventTime >= sliceStart && eventTime <= sliceEnd )
      {
        // insert event
        MappedEvent *me = new MappedEvent(**j);
        me->setInstrument((*i)->getInstrument());
        returnSlice->insert(me);
      }
    }
  }

  return returnSlice;
}


 
}
