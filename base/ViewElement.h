
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

#ifndef _VIEWELEMENT_H_
#define _VIEWELEMENT_H_

#include "Event.h"
//#include "Track.h"

namespace Rosegarden 
{

/**
 * The base class which represents an Event as an on-screen
 * graphic item (a note, a rectangle on a piano roll)
 */
class ViewElement
{
public:
    ViewElement(Event*);
    virtual ~ViewElement();

    const Event* event() const { return m_event; }
    Event*       event()       { return m_event; }

    timeT getAbsoluteTime() const  { return event()->getAbsoluteTime(); }
    void  setAbsoluteTime(timeT d) { event()->setAbsoluteTime(d); }

    timeT getDuration() const  { return event()->getDuration(); }
    void  setDuration(timeT d) { event()->setDuration(d); }

    void dump(std::ostream&) const;

    friend bool operator<(const ViewElement&, const ViewElement&);

protected:
    Event *m_event;
};

//class ViewElementsManagerBase
//{
//public:
//    virtual void insert(Track::iterator from, Track::iterator to) = 0;
//    virtual void erase (Track::iterator from, Track::iterator to) = 0;
//};

 
}


#endif

