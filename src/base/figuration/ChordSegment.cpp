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

#include "ChordSegment.h"
#include "base/Segment.h"
#include "base/figuration/SegmentID.h"

namespace Rosegarden
{

  void
  ChordSegmentMap::
  addSource(Segment * s, int id) {
    // Always insert this id itself, just in case somehow we don't
    // encounter it in the segment.
    insert(value_type(id, ChordSegment(s, id)));
    // Scan the whole segment looking for SegmentID and map all their
    // ids to this value.
    for (Segment::iterator i = s->begin();
         i != s->end();
         /* i is incremented in loop */) {
      if ((*i)->isa(SegmentID::EventType)) {
	int localID = SegmentID(**i).getID();
	if (localID != id) {
            insert(value_type(localID, ChordSegment(s, id)));
            // Remove it if localID != id, since it's anomalous and is
            // hindering us.  No need to make this operation undoable
            // since it ideally would have already happened during
            // segment merge.  Careful how we do this: Increment the
            // iterator, erase the original, and immediately continue
            // the loop so we don't increment it twice.
            s->erase(i++);
            continue;
        } 
      }
      ++i;
    }
  };

}
