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

#include "SegmentID.h"

#include "base/BaseProperties.h"
#include "base/NotationTypes.h" // Just for Text
#include "misc/Strings.h"
#include <QString>

namespace Rosegarden
{
   //SegmentID event types
const std::string SegmentID::EventType = "segment ID";
const int SegmentID::EventSubOrdering = -190;
const PropertyName SegmentID::IDPropertyName = "ID";
const PropertyName SegmentID::SubtypePropertyName = "Subtype";

const std::string SegmentID::Uninvolved = "uninvolved";
const std::string SegmentID::ChordSource = "chord source";
const std::string SegmentID::FigurationSource = "figuration source";
const std::string SegmentID::Target = "figuration target";
  
SegmentID::SegmentID(const Event &e) :
    m_ID(-1),
    m_type(Uninvolved)
{
    if (e.getType() != EventType) {
        throw Event::BadType("SegmentID model event",
                             EventType, e.getType());
    }

    e.get<Int>(IDPropertyName, m_ID);
    e.get<String>(SubtypePropertyName, m_type);
}

  SegmentID::SegmentID(const std::string type, int ID) :
    m_ID(ID),
    m_type(type)
    {}

Event *
SegmentID::getAsEvent(timeT absoluteTime) const
{
    Event *e = new Event(EventType, absoluteTime, 0, EventSubOrdering);
    e->set<Int>(IDPropertyName, m_ID);
    e->set<String>(SubtypePropertyName, m_type);
    return e;
}

const std::string
SegmentID::NotationString() const
{
  if (m_type == ChordSource)
      { return qStrToStrLocal8(QObject::tr("Chord Source Segment")); }
  if (m_type == FigurationSource)
    { return qStrToStrLocal8(QObject::tr("Figuration Source Segment")); }
  if (m_type == Target)
    { return qStrToStrLocal8(QObject::tr("Generated Segment")); }
  return qStrToStrLocal8(QObject::tr("Segment of unknown type"));
}

}


