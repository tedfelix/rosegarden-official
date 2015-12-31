/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/



#ifndef RG_OVERLAP_RANGE_H
#define RG_OVERLAP_RANGE_H

#include <vector>
#include <map>

#include "Event.h"



namespace Rosegarden
{

class Composition;
class Segment;
class StaffHeader;


/**
 * This class defines the indivisible range used as elements by the
 * Overlaps class.
 *
 * The template parameter may be Clef, Key or int (the last one to find out
 * transposition inconsistencies)
 */

template <class T>
class OverlapRange
{

public :
    OverlapRange() :
        m_inconsistancy(false)
{
}

    ~OverlapRange() { }

    // Following method is const because it must not be used to add segments :
    // usage of pushSegment() is mandatory to keep m_inconsistancy up to date.
    const std::vector<Segment *> *getSegments() const { return &m_segments; }

    T getRangeProperty(Segment *segment, timeT t);

    bool isInconsistent() { return m_inconsistancy; }

    void pushSegment(Segment *segment, timeT t) {
        int previousSize = m_segments.size();
        m_segments.push_back(segment);
        T property = getRangeProperty(segment, t);
        if (previousSize) {
            if (property != m_property) m_inconsistancy = true;
        } else {
            m_property = property;
            m_inconsistancy = false;
        }
    }


private :
    //--------------- Data members ---------------------------------

    std::vector<Segment *> m_segments;
    bool           m_inconsistancy;
    T              m_property;

};


//------------- Specialized functions -----------------------------------

template <>
inline Clef
OverlapRange<Clef>::getRangeProperty(Segment *segment, timeT t)
{
    return segment->getClefAtTime(t);
}

template <>
inline Key
OverlapRange<Key>::getRangeProperty(Segment *segment, timeT t)
{
    return segment->getKeyAtTime(t);
}

template <>
inline int
OverlapRange<int>::getRangeProperty(Segment *segment, timeT)
{
    return segment->getTranspose();
}


}

#endif // RG_OVERLAP_RANGE_H

