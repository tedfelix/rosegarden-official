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

#include "Track.h"
#include "NotationTypes.h"

namespace Rosegarden 
{
using std::cerr;
using std::endl;
    
Track::Track(unsigned int nbTimeSteps, unsigned int startIdx,
             unsigned int stepsPerBar)
    : std::multiset<Event*, Event::EventCmp>(),
    m_startIdx(startIdx),
    m_instrument(0)
{
    unsigned int initialTime = m_startIdx;
    
    // fill up with whole-note rests
    //
    for (unsigned int i = 0; i < nbTimeSteps; i += stepsPerBar) {
        Event *e = new Event;
        e->setType("rest");
        e->setDuration(384); // TODO : get rid of this magic number
        e->setAbsoluteTime(initialTime);
        insert(e);
        initialTime += 384; // btw, it comes from xmlstorableevent.cpp
    }
}

Track::~Track()
{
    // delete content
    for(iterator it = begin(); it != end(); ++it)
        delete (*it);
}

unsigned int Track::getNbTimeSteps() const
{
    const_iterator lastEl = end();
    --lastEl;
    unsigned int nbBars = ((*lastEl)->getAbsoluteTime() +
                           (*lastEl)->getDuration());

    return nbBars;
}


void Track::setNbTimeSteps(unsigned int nbTimeSteps)
{
    TimeSignature &signatureAtEnd = getTimeSigAtEnd();

    unsigned int currentNbTimeSteps = getNbTimeSteps();

    cerr << "Track::setNbBars() : current = " << currentNbTimeSteps
         << " - new : " << nbTimeSteps << endl;

    if (nbTimeSteps == currentNbTimeSteps) return; // nothing to do
    
    if (nbTimeSteps > currentNbTimeSteps) { // fill up with rests
        
        iterator lastEl = end();
        --lastEl;
        unsigned int newElTime = (*lastEl)->getAbsoluteTime() + (*lastEl)->getDuration();

        //!!! This is still not correct.  Although the "startOffset"
        // argument to TimeSignature::getDurationListForInterval() may
        // be an offset from the start of the whole piece (i.e. it's
        // allowed to be arbitrarily large), it will only be
        // meaningful if there are no time signature changes duration
        // the period of the offset, which in this case means no time
        // signature changes in the whole piece so far...  We should
        // be using the elapsed time since the start of the last bar,
        // instead of using newElTime here.

        DurationList dlist;
        signatureAtEnd.getDurationListForInterval
            (dlist, nbTimeSteps - currentNbTimeSteps, newElTime);

        Event::timeT acc = newElTime;
        for (DurationList::iterator i = dlist.begin(); i != dlist.end(); ++i) {
            Event *e = new Event("rest");
            e->setDuration(*i);
            e->setAbsoluteTime(acc);
            insert(e);
            acc += *i;
        }

    } else { // shrink

        // NOT IMPLEMENTED YET : move an internal marker

//         if (nbBars == 0) { // no fuss
//             erase(begin(), end());
//             return;
//         }

//         unsigned int cutTime = ((nbBars-1) * 384) + getStartIndex();

//         iterator lastElToKeep = std::upper_bound(begin(), end(),
//                                                  cutTime,
//                                                  Event::compareTime2Event);
//         if (lastElToKeep == end()) {
//             cerr << "Track::setNbBars() : upper_bound returned end\n";
//             return;
//         }

//         erase(lastElToKeep, end());
    }
    
}

void Track::setStartIndex(unsigned int idx)
{
    int idxDiff = idx - m_startIdx;

    // reset the time of all events
    for (iterator i = begin(); i != end(); ++i)
        (*i)->addAbsoluteTime(idxDiff);
        
    m_startIdx = idx;
}

TimeSignature& Track::getTimeSigAtEnd() const
{
    // temporary dummy implementation
    static TimeSignature sig44(4,4);

    return sig44;
}

 
}
