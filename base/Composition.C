
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
      m_tempo(0),
      m_position(0)
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
    track->addObserver(this);

    return res.first;
}

void
Composition::deleteTrack(Composition::iterator i)
{
    if (i == end()) return;

    Track *p = (*i);
    p->removeObserver(this);

    delete p;
    m_tracks.erase(i);
}

bool
Composition::deleteTrack(Track *p)
{
    iterator i = find(begin(), end(), p);
    
    if (i != end()) {
	p->removeObserver(this);
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


static Track::iterator findTimeSig(Track *t, timeT time)
{
    Track::iterator a, b;
    t->getTimeSlice(time, a, b);
    for (Track::iterator i = a; i != b; ++i) {
	if ((*i)->isa(TimeSignature::EventType)) return i;
    }
    return t->end();
}


// If a time signature is added to one track, add it to all other
// tracks as well.  Suppress observer semantics for each track as we
// add it, to avoid evil recursion.  We don't have to suppress
// observer for m_timeReference, as we aren't an observer for it
// anyway

void Composition::eventAdded(Track *t, Event *e)
{
    if (e->isa(TimeSignature::EventType)) {

	timeT sigTime = e->getAbsoluteTime();

	std::cerr << "Composition: noting addition of time signature at "
                  << sigTime << std::endl;


	Track::iterator found = findTimeSig(&m_timeReference, sigTime);
	if (found != m_timeReference.end()) m_timeReference.erase(found);
	m_timeReference.insert(new Event(*e));

	for (iterator i = begin(); i != end(); ++i) {
	    std::cerr << "Composition: comparing with a track" << std::endl;
	    if (*i != t) {
		(*i)->removeObserver(this);
		found = findTimeSig(*i, sigTime);
		if (found != (*i)->end()) (*i)->erase(found);
		(*i)->insert(new Event(*e));
		(*i)->addObserver(this);
	    } else std::cerr << "Composition: skipping" << std::endl;
	    (*i)->calculateBarPositions();
	}
    }
}


// If a time signature is removed from one track, remove it from all
// other tracks as well.  Suppress observer semantics for each track
// as we remove, to avoid evil recursion.  We don't have to suppress
// observer for m_timeReference, as we aren't an observer for it
// anyway

void Composition::eventRemoved(Track *t, Event *e)
{
    if (e->isa(TimeSignature::EventType)) {

	timeT sigTime = e->getAbsoluteTime();

	std::cerr << "Composition: noting removal of time signature at "
	     << sigTime << std::endl;

	Track::iterator found = findTimeSig(&m_timeReference, sigTime);
	if (found != m_timeReference.end()) m_timeReference.erase(found);

	for (iterator i = begin(); i != end(); ++i) {
	    std::cerr << "Composition: comparing with a track" << std::endl;
	    if (*i != t) {
		(*i)->removeObserver(this);
		found = findTimeSig(*i, sigTime);
		if (found != (*i)->end()) (*i)->erase(found);
		(*i)->addObserver(this);
	    } else std::cerr << "Composition: skipping" << std::endl;
	    (*i)->calculateBarPositions();
	}
    }
}


void Composition::trackDeleted(Track *)
{
    // now really, who gives a shit?
}


}
