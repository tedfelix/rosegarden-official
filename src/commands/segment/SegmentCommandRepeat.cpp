/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "SegmentCommandRepeat.h"

#include "base/Segment.h"
#include "SegmentCommand.h"
#include <QString>


namespace Rosegarden
{

SegmentCommandRepeat::SegmentCommandRepeat
(const std::vector<Segment*>& segments,
 bool repeat)
    : SegmentCommand(tr("Repeat Segments"), segments),
      m_newRepeatState(repeat)
{}

void SegmentCommandRepeat::execute()
{
    segmentlist::iterator it;

    for (it = m_segments.begin(); it != m_segments.end(); ++it) {
        bool oldRepeat = (*it)->isRepeating();
        m_oldRepeatState.push_back(oldRepeat);
        (*it)->setRepeating(m_newRepeatState);
    }
}

void SegmentCommandRepeat::unexecute()
{
    for(unsigned int i = 0; i < m_segments.size(); i++) {
        m_segments[i]->setRepeating(m_oldRepeatState[i]);
    }
}

}
