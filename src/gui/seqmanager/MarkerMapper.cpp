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

#include "MarkerMapper.h"
#include "base/Composition.h"
#include "base/Marker.h"
#include "document/RosegardenDocument.h"
#include "misc/Debug.h"
#include "sound/MappedEvent.h"

#define DEBUG_MARKER_MAPPER 1

namespace Rosegarden
{

MarkerMapper::
MarkerMapper(RosegardenDocument *doc) :
    SpecialSegmentMapper(doc)
{
    init();
}

void MarkerMapper::fillBuffer()
{
    resize(0);

    Composition& comp = m_doc->getComposition();
    Composition::markercontainer &marks = comp.getMarkers();

    for (Composition::markerconstiterator i = marks.begin();
	 i != marks.end(); ++i) {

      std::string metaMessage = (*i)->getName();
      RealTime eventTime = comp.getElapsedRealTime((*i)->getTime());
#ifdef DEBUG_MARKER_MAPPER
    SEQUENCER_DEBUG
        << "MarkerMapper::fillBuffer inserting marker message"
        << metaMessage
        << "at" << eventTime
        << endl;
#endif

      MappedEvent e;
      e.setType(MappedEvent::Marker);
      e.setEventTime(eventTime);
      e.addDataString(metaMessage);
      mapAnEvent(&e);
    }
}

int
MarkerMapper::calculateSize()
{
    return m_doc->getComposition().getMarkers().size();
}

// Markers always "play".  When we don't want them, we simply don't
// include a marker mapper.
bool
MarkerMapper::
shouldPlay(MappedEvent */*evt*/, RealTime /*startTime*/)
{ return true; }

}

