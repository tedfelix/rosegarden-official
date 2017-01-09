/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/



#ifndef RG_OVERLAPS_H
#define RG_OVERLAPS_H

#include <vector>
#include <map>

#include "Event.h"
#include "Segment.h"
#include "OverlapRange.h"



namespace Rosegarden
{

/**
 * This class aims to pack in an unique object the list of notation
 * inconsistencies got when segments with differents clef, key or
 * transposition are overlapping on the same track :
 *    1 - To help to show a warning if such an inconsistency is displayed
 *        in notation editor
 *    2 - To help to print out these inconsistencies as a text
 *
 * The template parameter may be Clef, Key or int (the last one to find out
 * transposition inconsistencies)
 */

template <class T>
class Overlaps : public std::map<timeT, OverlapRange<T> >
{

public :

    Overlaps(
             std::vector<Segment *> segments
)
{

    std::map<timeT, OverlapRange<T> >::clear();

    for (std::vector<Segment *>::iterator i=segments.begin();
         i!=segments.end();
         ++i) {

        OverlapRange<T> range = OverlapRange<T>();

        timeT segStart = (*i)->getStartTime();
        timeT segEnd = (*i)->getEndMarkerTime();

        // Start and ends of segments always are range limits
        this->insert(std::pair<timeT, OverlapRange<T> >(segStart, range));
        this->insert(std::pair<timeT, OverlapRange<T> >(segEnd, range));

        timeT currentTime, propertyTime;
        currentTime = segStart;
        for (;;) {
            bool fnd = getNextPropertyTime((*i), currentTime, propertyTime);
            if (!fnd) break;
            this->insert(std::pair<timeT, OverlapRange<T> >(propertyTime, range));
            currentTime = propertyTime;
        }
    }

    // Complete the OverlapRange map
    typename std::map<timeT, OverlapRange<T> >::iterator it, next;
    for (it = this->begin();
         it != this->end(); ++it) {
        timeT t1 = it->first;
        next = it;
        ++next;
        if (next == this->end()) {
            // The last element is only used to mark the end of the time range
            // this->erase(it);
            break;
        }
        timeT t2 = next->first;

        // Fill up the vector with segments
        for (std::vector<Segment *>::iterator i=segments.begin();
            i!=segments.end();
            ++i) {
            timeT segStart = (*i)->getStartTime();
            timeT segEnd = (*i)->getEndMarkerTime();
            if ((segStart <= t1) && (t2 <= segEnd)) {
                it->second.pushSegment(*i, t1);
            }
        }

    }

}

    ~Overlaps() {}

    bool
    isConsistent(timeT t1, timeT t2)
    {
        typename std::map<timeT, OverlapRange<T> >::iterator i;
        if (!getFirst(t1, t2, i)) return true;
        if (! isConsistent(i)) return false;
        while (getNext(t2, i)) {
            if (! isConsistent(i)) return false; 
        }
        return true;
    }

    bool
    isConsistent()
    {
        typename std::map<timeT, OverlapRange<T> >::iterator i, j;
        i = this->begin();
        if (! isConsistent(i)) return false;
        ++i;
        j = i;
        ++j;
        while (j != this->end()) {
            if (! isConsistent(i)) return false;
            ++i;
            ++j;
        }
        return true;
    }

    bool
    getFirst(timeT t1, timeT t2,
             typename std::map<timeT, OverlapRange<T> >::iterator &it)
    {
        typename std::map<timeT, OverlapRange<T> >::iterator i, j;
        i = this->upper_bound(t1);
        if (i == this->end()) return false;
        j = i;
        --j;
        if (j == this->end()) {
            if (i->first > t2) return false;
            it = i;
            return true;
        }
        it = j;
        return true;
    }

    bool
    getNext(timeT t2, typename std::map<timeT, OverlapRange<T> >::iterator &it)
    {
        it++;
        if (it == this->end()) return false;

        typename std::map<timeT, OverlapRange<T> >::iterator next = it;
        next++;
        if (next == this->end()) return false;

        if (it->first > t2) return false;
        return true;
    }

    void
    getTimeRange(typename std::map<timeT, OverlapRange<T> >::iterator it,
                      timeT &t1, timeT &t2)
    {
        t1 = it->first;
        it++;
        t2 = it->first;
    }

    bool
    isConsistent(typename std::map<timeT, OverlapRange<T> >::iterator it)
    {
        return ! it->second.isInconsistent();
    }

    const std::vector<Segment *> *
    getSegments(typename std::map<timeT, OverlapRange<T> >::iterator it)
    {
        return it->second.getSegments();
    }

    T getPropertyAtTime(Segment *segment, timeT time) const;


protected :

    bool getNextPropertyTime(Segment *, timeT time, timeT &nextTime) const;


private :
    //--------------- Data members ---------------------------------

};


//------------- Specialized functions -----------------------------------

template <>
inline Clef
Overlaps<Clef>::getPropertyAtTime(Segment *seg, timeT time) const
{
    return  seg->getClefAtTime(time);
}

template <>
inline bool
Overlaps<Clef>::getNextPropertyTime(Segment *seg, timeT time, timeT &nextTime) const
{
    return  seg->getNextClefTime(time, nextTime);
}


template <>
inline Key
Overlaps<Key>::getPropertyAtTime(Segment *seg, timeT time) const
{
    return  seg->getKeyAtTime(time);
}

template <>
inline bool
Overlaps<Key>::getNextPropertyTime(Segment *seg, timeT time, timeT &nextTime) const
{
    return  seg->getNextKeyTime(time, nextTime);
}


template <>
inline int
Overlaps<int>::getPropertyAtTime(Segment *seg, timeT) const
{
    return  seg->getTranspose();
}

template <>
inline bool
Overlaps<int>::getNextPropertyTime(Segment *, timeT time, timeT &nextTime) const
{
    nextTime = time;
    return  false;
}


}


#endif // RG_OVERLAPS_H

