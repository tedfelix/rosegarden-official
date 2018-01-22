/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

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
                                     bool drum) :
    ViewSegment(*segment),
    m_scene(scene),
    m_drum(drum),
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

void
MatrixViewSegment::resetRefreshStatus()
{
    m_segment.getRefreshStatus(m_refreshStatusId).setNeedsRefresh(false);
}

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
    ViewSegment::eventRemoved(segment, event);
    m_scene->handleEventRemoved(event);
}

ViewElement *
MatrixViewSegment::makeViewElement(Event* e)
{
    MATRIX_DEBUG << "MatrixViewSegment::makeViewElement: event at "
                 << e->getAbsoluteTime() << endl;

    // transpose bits
    long pitchOffset = getSegment().getTranspose();

//    std::cout << "I am segment \"" << getSegment().getLabel() << "\"" << std::endl;

    return new MatrixElement(m_scene, e, m_drum, pitchOffset);
}

void
MatrixViewSegment::endMarkerTimeChanged(const Segment *s, bool shorten)
{
    ViewSegment::endMarkerTimeChanged(s, shorten);
    if (m_scene) m_scene->segmentEndMarkerTimeChanged(s, shorten);
}

void
MatrixViewSegment::updateElements(timeT from, timeT to)
{
    if (!m_viewElementList) return;
    ViewElementList::iterator i = m_viewElementList->findTime(from);
    ViewElementList::iterator j = m_viewElementList->findTime(to);
    while (i != m_viewElementList->end()) {
        MatrixElement *e = static_cast<MatrixElement *>(*i);
        e->reconfigure();
        if (i == j) break;
        ++i;
    }
}

}

