/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_VIEW_SEGMENT_H
#define RG_VIEW_SEGMENT_H

#include "ViewElement.h"
#include "base/Segment.h"

#include <cassert>

namespace Rosegarden 
{

class ViewSegmentObserver;

/**
 * ViewSegment is the base class for classes which represent a Segment as an
 * on-screen graphic.  It manages the relationship between Segment/Event
 * and specific implementations of ViewElement.
 *
 * ViewSegment was formerly known as Staff, and before that as
 * ViewElementsManager.  It was renamed from Staff to ViewSegment to
 * avoid confusion with classes that draw staff lines and other
 * surrounding context.  All this does is manage the view elements.
 */
class ViewSegment : public SegmentObserver
{
public: 
    ~ViewSegment() override;

    /**
     * Create a new ViewElementList wrapping all Events in the
     * segment, or return the previously created one
     */
    ViewElementList *getViewElementList();

    /**
     * Return the Segment wrapped by this object 
     */
    Segment &getSegment() { return m_segment; }

    /**
     * Return the Segment wrapped by this object 
     */
    const Segment &getSegment() const { return m_segment; }

    /**
     * Return the location of the given event in this ViewSegment
     */
    ViewElementList::iterator findEvent(Event *);

    /**
     * SegmentObserver method - called after the event has been added to
     * the segment
     */
    void eventAdded(const Segment *, Event *) override;

    /**
     * SegmentObserver method - called after the event has been removed
     * from the segment, and just before it is deleted
     */
    void eventRemoved(const Segment *, Event *) override;

    /** 
     * SegmentObserver method - called after the segment's end marker
     * time has been changed
     */
    void endMarkerTimeChanged(const Segment *, bool shorten) override;

    /**
     * SegmentObserver method - called from Segment dtor
     */
    void segmentDeleted(const Segment *) override;

    void addObserver   (ViewSegmentObserver *obs) { m_observers.push_back(obs); }
    void removeObserver(ViewSegmentObserver *obs) { m_observers.remove(obs); }

protected:
    ViewSegment(Segment &);
    virtual ViewElement* makeViewElement(Event*) = 0;
    
    /**
     * Return true if the event should be wrapped
     * Useful for piano roll where we only want to wrap notes
     * (always true by default)
     */
    virtual bool wrapEvent(Event *);

    void notifyAdd(ViewElement *) const;
    void notifyRemove(ViewElement *) const;
    void notifySourceDeletion() const;

    //--------------- Data members ---------------------------------

    Segment &m_segment;
    ViewElementList *m_viewElementList;

    typedef std::list<ViewSegmentObserver*> ObserverSet;
    ObserverSet m_observers;

    bool m_modified;
    timeT m_modStart;
    timeT m_modEnd;

private: // not provided
    ViewSegment(const ViewSegment &);
    ViewSegment &operator=(const ViewSegment &);
};

class ViewSegmentObserver
{
public:
    virtual ~ViewSegmentObserver() {}
    virtual void elementAdded(const ViewSegment *, ViewElement *) = 0;
    virtual void elementRemoved(const ViewSegment *, ViewElement *) = 0;

    /// called when the observed object is being deleted
    virtual void viewSegmentDeleted(const ViewSegment *) = 0;
};



}

#endif

