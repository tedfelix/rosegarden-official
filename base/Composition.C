
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
#include "FastVector.h"

#include <iostream>


namespace Rosegarden 
{

const std::string Composition::BarEventType = "bar";


Composition::Composition()
    : m_nbTicksPerBar(384),
      m_tempo(0),
      m_position(0),
      m_barPositionsNeedCalculating(true)
{
    // empty
}

Composition::~Composition()
{
    clear();
}

void Composition::swap(Composition& c)
{
    // ugh.

    Composition *that = &c;
    unsigned int t;

    t = this->m_nbTicksPerBar;
    this->m_nbTicksPerBar = that->m_nbTicksPerBar;
    that->m_nbTicksPerBar = t;

    double tp = this->m_tempo;
    this->m_tempo = that->m_tempo;
    that->m_tempo = tp;

    m_timeReference.swap(c.m_timeReference);

    m_tracks.swap(c.m_tracks);

    for (trackcontainer::iterator i = that->m_tracks.begin();
	 i != that->m_tracks.end(); ++i) {
	(*i)->removeObserver(this);
	(*i)->addObserver(that);
	(*i)->setReferenceTrack(&that->m_timeReference);
    }

    for (trackcontainer::iterator i = this->m_tracks.begin();
	 i != this->m_tracks.end(); ++i) {
	(*i)->removeObserver(that);
	(*i)->addObserver(this);
	(*i)->setReferenceTrack(&this->m_timeReference);
    }
}

Composition::iterator
Composition::addTrack(Track *track)
{
    if (!track) return end();
    
    std::pair<iterator, bool> res = m_tracks.insert(track);
    track->addObserver(this);
    track->setReferenceTrack(&m_timeReference);

    return res.first;
}

void
Composition::deleteTrack(Composition::iterator i)
{
    if (i == end()) return;

    Track *p = (*i);
    p->removeObserver(this);
    p->setReferenceTrack(0);

    delete p;
    m_tracks.erase(i);
}

bool
Composition::deleteTrack(Track *p)
{
    iterator i = find(begin(), end(), p);
    if (i == end()) return false;
    
    deleteTrack(i);
    return true;
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
    for (trackcontainer::iterator i = m_tracks.begin();
        i != m_tracks.end(); ++i) {
        (*i)->removeObserver(this);
	(*i)->setReferenceTrack(0);
        delete (*i);
    }
    m_tracks.erase(begin(), end());
    m_timeReference.erase(m_timeReference.begin(), m_timeReference.end());
}

Track::iterator
Composition::addNewBar(timeT time)
{
    std::cerr << "Composition::addNewBar" << std::endl;
    Event *e = new Event(BarEventType);
    e->setAbsoluteTime(time);
    return m_timeReference.insert(e);
}

void
Composition::calculateBarPositions()
{
    std::cerr << "Composition::calculateBarPositions" << std::endl;


    Track &t = m_timeReference;
    Track::iterator i;

    FastVector<timeT> segments;
    FastVector<timeT> segmentTimes;

    for (i = t.begin(); i != t.end(); ++i) {
	if ((*i)->isa(BarEventType)) t.erase(i);
	else {
	    segments.push_back((*i)->getAbsoluteTime());
	    segmentTimes.push_back(TimeSignature(**i).getBarDuration());
	}
    }

    bool segment0isTimeSig = true;
    if (segments.size() == 0 || segments[0] != 0) {
	segments.push_front(0);
	segmentTimes.push_front(TimeSignature().getBarDuration());
	segment0isTimeSig = false;
    }    

    timeT duration = getDuration();
    segments.push_back(duration);

    for (int s = 0; s < segments.size() - 1; ++s) {

	timeT start = segments[s], finish = segments[s+1];
	timeT time;

	if (s > 0 || segment0isTimeSig) start += segmentTimes[s];

	std::cerr << "segment " << s << ": start " << start << ", finish " << finish << std::endl;

	for (time = start; time < finish; time += segmentTimes[s]) {
	    addNewBar(time);
	}

	if (s == segments.size() - 1 && time != duration) addNewBar(time);
    }

    std::cerr << "Composition::calculateBarPositions ending" << std::endl;
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

//!!! Aaargh! Of course, if a track is moved (i.e. its start time
// changes, and therefore all the times of the events including the
// time signature event) then we're well fucked.  Better not to allow
// someone to insert time signature events into a track other than the
// reference track at all

void Composition::eventAdded(const Track *t, Event *e)
{
    // in theory this should only be true if we insert a time
    // signature or something after the former end of the composition
    m_barPositionsNeedCalculating = true;

    if (e->isa(TimeSignature::EventType)) {

	timeT sigTime = e->getAbsoluteTime();

	std::cerr << "Composition: noting addition of time signature at "
                  << sigTime << std::endl;


	Track::iterator found = findTimeSig(&m_timeReference, sigTime);
	if (found != m_timeReference.end()) m_timeReference.erase(found);
	m_timeReference.insert(new Event(*e));

        //!!! Modify so as only to insert if the time sig falls within
        //the range of the track; if it's before the track starts, we
        //need to stick it at the start unless another time sig
        //appears later.  Complicated.  Maybe we should just allow
        //time sigs within a track before the nominal start time --
        //why not?

	for (iterator i = begin(); i != end(); ++i) {
	    std::cerr << "Composition: comparing with a track" << std::endl;
	    if (*i != t) {
		(*i)->removeObserver(this);
		found = findTimeSig(*i, sigTime);
		if (found != (*i)->end()) (*i)->erase(found);
		(*i)->insert(new Event(*e));
		(*i)->addObserver(this);
	    } else std::cerr << "Composition: skipping" << std::endl;
//!!!	    (*i)->calculateBarPositions();
	}
    }
}


// If a time signature is removed from one track, remove it from all
// other tracks as well.  Suppress observer semantics for each track
// as we remove, to avoid evil recursion.  We don't have to suppress
// observer for m_timeReference, as we aren't an observer for it
// anyway

void Composition::eventRemoved(const Track *t, Event *e)
{
    // in theory this should only be true if we insert a time
    // signature or something after the former end of the composition
    m_barPositionsNeedCalculating = true;

    if (e->isa(TimeSignature::EventType)) {

	timeT sigTime = e->getAbsoluteTime();

	std::cerr << "Composition: noting removal of time signature at "
	     << sigTime << std::endl;

	Track::iterator found = findTimeSig(&m_timeReference, sigTime);
	if (found != m_timeReference.end()) m_timeReference.erase(found);

        //!!! What do we do if the time sig is before the start of a
        //track, if we've modified eventAdded so as not always to add
        //them in this case?

	for (iterator i = begin(); i != end(); ++i) {
	    std::cerr << "Composition: comparing with a track" << std::endl;
	    if (*i != t) {
		(*i)->removeObserver(this);
		found = findTimeSig(*i, sigTime);
		if (found != (*i)->end()) (*i)->erase(found);
		(*i)->addObserver(this);
	    } else std::cerr << "Composition: skipping" << std::endl;
//!!!	    (*i)->calculateBarPositions();
	}
    }
}


void Composition::referenceTrackRequested(const Track *)
{
    if (m_barPositionsNeedCalculating) calculateBarPositions();
    m_barPositionsNeedCalculating = false;
}


}
