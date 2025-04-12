/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[MatrixViewSegment]"
#define RG_NO_DEBUG_PRINT 1

#include "MatrixViewSegment.h"

#include "MatrixScene.h"
#include "MatrixElement.h"

#include "base/NotationTypes.h"
#include "base/SnapGrid.h"
#include "base/MidiProgram.h"
#include "base/SnapGrid.h"
#include "base/SnapGrid.h"

#include "misc/Debug.h"

namespace Rosegarden
{

MatrixViewSegment::MatrixViewSegment(MatrixScene *scene,
                                     Segment *segment,
                                     bool drumMode) :
    ViewSegment(*segment),
    m_scene(scene),
    m_drum(drumMode),
    m_refreshStatusId(segment->getNewRefreshStatusId())
{
}

MatrixViewSegment::~MatrixViewSegment()
{
}

SegmentRefreshStatus &
MatrixViewSegment::getRefreshStatus() const
{
    return m_segment.getRefreshStatus(m_refreshStatusId);
}

/* unused
void
MatrixViewSegment::resetRefreshStatus()
{
    m_segment.getRefreshStatus(m_refreshStatusId).setNeedsRefresh(false);
}
*/

bool
MatrixViewSegment::wrapEvent(Event* e)
{
    return e->isa(Note::EventType) && ViewSegment::wrapEvent(e);
}

void
MatrixViewSegment::eventAdded(const Segment *segment,
                              Event *event)
{
    ViewSegment::eventAdded(segment, event);
    m_scene->handleEventAdded(event);
}

void
MatrixViewSegment::eventRemoved(const Segment *segment,
                                Event *event)
{
    // !!! This deletes the associated MatrixElement.
    ViewSegment::eventRemoved(segment, event);

    // At this point, the MatrixElement is gone.  See ViewElement::erase().
    // Any clients that respond to the following and any handlers of
    // signals emitted by the following must not try to use the MatrixElement.
    m_scene->handleEventRemoved(event);
}

ViewElement *
MatrixViewSegment::makeViewElement(Event* e)
{
    //RG_DEBUG << "makeViewElement(): event at " << e->getAbsoluteTime();

    // transpose bits
    long pitchOffset = getSegment().getTranspose();

    //RG_DEBUG << "  I am segment \"" << getSegment().getLabel() << "\"";

    return new MatrixElement(m_scene, e, m_drum, pitchOffset, &getSegment());
}

void
MatrixViewSegment::endMarkerTimeChanged(const Segment *segment, bool shorten)
{
    ViewSegment::endMarkerTimeChanged(segment, shorten);
    if (m_scene) m_scene->segmentEndMarkerTimeChanged(segment, shorten);
}

void
MatrixViewSegment::updateElements(timeT from, timeT to)
{
    if (!m_viewElementList)
        return;

    ViewElementList::iterator viewElementIter =
            m_viewElementList->findTime(from);
    ViewElementList::iterator endIter = m_viewElementList->findTime(to);

    // For each ViewElement in the from-to range...
    while (viewElementIter != m_viewElementList->end()) {
        MatrixElement *e = static_cast<MatrixElement *>(*viewElementIter);
        // Update the item on the display.
        e->reconfigure();

        if (viewElementIter == endIter)
            break;
        ++viewElementIter;
    }
}

void
MatrixViewSegment::updateAll()
{
    for(ViewElementList::iterator i = m_viewElementList->begin();
        i != m_viewElementList->end();
        ++i) {
        MatrixElement *e = static_cast<MatrixElement *>(*i);
        e->reconfigure();
    }
}

}
