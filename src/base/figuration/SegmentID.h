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

#ifndef RG_SEGMENTID_H
#define RG_SEGMENTID_H

#include "base/Event.h"

namespace Rosegarden
{
  class Text;
  
/**
 * SegmentID supplies ID information for a segment, used for updating
 * figurations.
 */
class SegmentID
{
 public:
  static const std::string EventType;
  static const int EventSubOrdering;
  static const PropertyName IDPropertyName;
  static const PropertyName SubtypePropertyName;

  static const std::string Uninvolved;
  static const std::string ChordSource;
  static const std::string FigurationSource;
  static const std::string Target;
  
  SegmentID(const Event &e);
  SegmentID(const std::string type, int ID = -1);

  /// Returned event is on heap; caller takes responsibility for ownership
  Event *getAsEvent(timeT absoluteTime) const;
  int    getID() const
  { return m_ID; }
  std::string getType() const
    { return m_type; }
  const std::string NotationString() const;

 private:
  long m_ID;
  std::string m_type;
};

}

#endif /* ifndef RG_SEGMENTID_H */
