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

#ifndef RG_VIEWELEMENT_H
#define RG_VIEWELEMENT_H


#include "Event.h"

#include <set>
#include <list>

namespace Rosegarden 
{

/**
 * The abstract base for classes which represent an Event as an
 * on-screen graphic item (a note, a rectangle on a piano roll).
 */

class ViewElement
{
    friend class ViewElementList;
    friend class Staff;

public:
    virtual ~ViewElement();

    const Event* event() const { return m_event; }
    Event*       event()       { return m_event; }

    virtual timeT getViewAbsoluteTime() const  { return event()->getAbsoluteTime(); }
    virtual timeT getViewDuration() const      { return event()->getDuration();     }

    /**
     * Returns the X coordinate of the element, as computed by the
     * layout. This is not the coordinate of the associated canvas
     * item.
     *
     * @see getCanvasX()
     */
    virtual double getLayoutX() const   { return m_layoutX; }

    /**
     * Returns the Y coordinate of the element, as computed by the
     * layout. This is not the coordinate of the associated canvas
     * item.
     *
     * @see getCanvasY()
     */
    virtual double getLayoutY() const   { return m_layoutY; }

    /**
     * Sets the X coordinate which was computed by the layout engine
     * @see getLayoutX()
     */
    virtual void   setLayoutX(double x) { m_layoutX = x; }

    /**
     * Sets the Y coordinate which was computed by the layout engine
     * @see getLayoutY()
     */
    virtual void   setLayoutY(double y) { m_layoutY = y; }

    void dump(std::ostream&) const;

    friend bool operator<(const ViewElement&, const ViewElement&);

protected:
    ViewElement(Event *);

    double m_layoutX;
    double m_layoutY;
    
    Event *m_event;
};



class ViewElementComparator
{
public:
    bool operator()(const ViewElement *e1, const ViewElement *e2) const {
        return *e1 < *e2;
    }
};

/**
 * This class owns the objects its items are pointing at.
 */
class ViewElementList : public std::multiset<ViewElement *, ViewElementComparator >
{
    typedef std::multiset<ViewElement *, ViewElementComparator > set_type;
public:
    typedef set_type::iterator iterator;

    ViewElementList() : set_type() { }
    virtual ~ViewElementList();

    void insert(ViewElement *);
    void erase(iterator i);
    void erase(iterator from, iterator to);
    void eraseSingle(ViewElement *);

    iterator findPrevious(const std::string &type, iterator i);
    iterator findNext(const std::string &type, iterator i);

    /**
     * Returns an iterator pointing to that specific element,
     * end() otherwise
     */
    iterator findSingle(ViewElement *);

    const_iterator findSingle(ViewElement *e) const {
        return const_iterator(((const ViewElementList *)this)->findSingle(e));
    }

    /**
     * Returns first iterator pointing at or after the given time,
     * end() if time is beyond the end of the list
     */ 
    iterator findTime(timeT time);

    const_iterator findTime(timeT time) const {
        return const_iterator(((const ViewElementList *)this)->findTime(time));
    }

    /**
     * Returns iterator pointing to the first element starting at
     * or before the given absolute time
     */
    iterator findNearestTime(timeT time);

    const_iterator findNearestTime(timeT time) const {
        return const_iterator(((const ViewElementList *)this)->findNearestTime(time));
    }
};

}


#endif

