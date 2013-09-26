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

#ifndef RG_CHORDSEGMENT_H
#define RG_CHORDSEGMENT_H

#include <map>

namespace Rosegarden
{
  class Segment;
  
// @class ChordSegment
// Basically one segment providing chords.  The ID is just a cached
// representation of a tag in the segment.
struct ChordSegment
{
  ChordSegment(Segment *s, int ID) :
    m_s(s),
    m_ID(ID)
    {}
  Segment*       m_s;
  int            m_ID;
};

// @class ChordSegmentMap
// @remarks The apparent duplication of the ID int is deliberate and
// the IDs aren't neccessarily the same.  Multiple IDs may map to the
// same ChordSegment in the case of chord source segments that got
// merged, so that old IDs are treated correctly while new generated
// regions get the same chord source ID.
class ChordSegmentMap : public std::map<int, ChordSegment>
{
public:
    void addSource(Segment * s, int id);
};

}

#endif /* ifndef RG_CHORDSEGMENT_H */
