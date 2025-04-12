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

#define RG_MODULE_STRING "[SegmentOrderer]"

#include "SegmentOrderer.h"

#include "misc/Debug.h"
#include "base/Composition.h"
#include "base/Segment.h"


namespace Rosegarden
{

/* unused
void SegmentOrderer::segmentClicked(const Segment* s)
{
    m_segmentZs[s] = ++m_currentMaxZ;
    RG_DEBUG << "SegmentOrderer::segmentClicked() s = " << s << " - max Z = " << m_currentMaxZ;
}
*/

unsigned int SegmentOrderer::getZForSegment(const Rosegarden::Segment* s)
{
    return m_segmentZs[s];
}

}
