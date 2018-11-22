/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2012 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "GeneratedRegion.h"
#include "base/BaseProperties.h"
#include "misc/Strings.h"
#include <QString>

namespace Rosegarden
{


const std::string GeneratedRegion::EventType = "generated region";
const int GeneratedRegion::EventSubOrdering = -180;
const PropertyName GeneratedRegion::ChordPropertyName = "chord source ID";
const PropertyName GeneratedRegion::FigurationPropertyName = "figuration source ID";

GeneratedRegion::GeneratedRegion(const Event &e) :
    m_chordSourceID(-1),
    m_figurationSourceID(-1)
{
    if (e.getType() != EventType) {
        throw Event::BadType("GeneratedRegion model event",
                             EventType, e.getType());
    }

    e.get<Int>(ChordPropertyName, m_chordSourceID);
    e.get<Int>(FigurationPropertyName, m_figurationSourceID);
    m_duration = e.getDuration();
}

GeneratedRegion::GeneratedRegion(int chordSourceID, int figurationSourceID, timeT duration) :
    m_chordSourceID(chordSourceID),
    m_figurationSourceID(figurationSourceID),
    m_duration(duration)
    {}

Event *
GeneratedRegion::getAsEvent(timeT absoluteTime) const
{
    Event *e = new Event(EventType, absoluteTime, m_duration, EventSubOrdering);
    e->set<Int>(ChordPropertyName, m_chordSourceID);
    e->set<Int>(FigurationPropertyName, m_figurationSourceID);
    return e;
}

const std::string
GeneratedRegion::NotationString() const
{
    return std::string("G");
}
}

