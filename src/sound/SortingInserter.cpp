/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "SortingInserter.h"

namespace Rosegarden
{

void
SortingInserter::
insertSorted(MappedInserterBase &exporter)
{
    static MappedEventCmp merc;
    // std::list sort is stable, so we get same-time events in the
    // order we inserted them, important for NoteOffs.
    m_list.sort(merc);
    std::list<MappedEvent>::const_iterator i = m_list.begin();
    if (i != m_list.end() && i->getEventTime() < RealTime::zeroTime) {
        // Negative time if the composition starts before the bar 1
        RealTime timeOffset = - i->getEventTime();
        for(; i != m_list.end(); ++i) {
            MappedEvent *mE = new MappedEvent(*i);
            mE->setEventTime(mE->getEventTime() + timeOffset);
            exporter.insertCopy(*mE);
        }
    } else {
        for(; i != m_list.end(); ++i) {
            exporter.insertCopy(*i);
        }
    }
}

void
SortingInserter::
insertCopy(const MappedEvent &evt)
{
    m_list.push_back(evt);
}
  
}


