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

#ifndef RG_SEGMENTFIGDATA_H
#define RG_SEGMENTFIGDATA_H

#include "base/figuration/FigurationSourceMap.h"
#include <string>
#include <vector>

namespace Rosegarden
{

  class Segment;
  class Event;
  class ChordSegment;
  class ChordSegmentMap;
  class FigurationSourceMap;
  class MacroCommand;

// @class SegmentFigData
// Data about a segment, for figuration purposes.
struct SegmentFigData
{
public:
  typedef std::map<Segment*, SegmentFigData> SegmentFigDataMap;
  enum SegTypes {
    Uninvolved,
    ChordSource,
    FigurationSource,
    Target,
    Unavailable, // For audio segments.
  };

  SegmentFigData(SegTypes  type,
		 bool      needsTag,
		 int       id) :
    m_type(type),
    m_needsTag(needsTag),
    m_id(id)
  {}
  explicit SegmentFigData(Segment* s);

  void addTagIfNeeded(Segment *s, MacroCommand* command);
  int getID() const { return m_id; }
  bool needsTag() const { return m_needsTag; }
  SegTypes getType() const { return m_type; }
  void convertType(SegTypes type) {
    m_type = type;
    m_needsTag = true;
  }
  bool isa(SegTypes type) const { return m_type == type; }

  static int getUnusedSegmentID() {
    ++m_maxID;
    return m_maxID;
  }
  static bool IsValidId(int id) { return id >= 0; }
  static SegmentFigDataMap getInvolvedSegments(bool onlyIfNeedTag,
					       MacroCommand* command);
  static SegmentFigData &findOrAdd(SegmentFigDataMap &map, Segment *s);
  static void updateSourceIDs(MacroCommand* command)
  { (void)getInvolvedSegments(true, command); }

  static void addTag(Segment* s, MacroCommand* command,
		     std::string type, int id = getUnusedSegmentID());
  // Try to expand the figuration wrt these parameters.
  // @return The time at the end of the expansion.  If no expansion
  // was done, return startTime.
  static timeT expand(SourcedFiguration& sourcedFiguration,
                      ChordSegment chordSource,
                      Segment*    target,
                      timeT       startTime);

  static void updateComposition(MacroCommand* command);
  static bool eventShouldPass(Event *e);

private:
  SegTypes      m_type;
  bool          m_needsTag;
  int           m_id;
  static int m_maxID;
};

}

#endif /* ifndef RG_SEGMENTFIGDATA_H */
