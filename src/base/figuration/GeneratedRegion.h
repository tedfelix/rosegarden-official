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

#ifndef RG_GENERATEDREGION_H
#define RG_GENERATEDREGION_H

#include "base/Event.h"

namespace Rosegarden
{
/**
 * GeneratedRegion indicates an automatically generated region and
 * gives the IDs of the sources that generated it.
 */
class GeneratedRegion
{
public:
  static const std::string EventType;
  static const int EventSubOrdering;
  static const PropertyName ChordPropertyName;
  static const PropertyName FigurationPropertyName;
  GeneratedRegion(const Event &e);
  GeneratedRegion(int chordSourceID, int figurationSourceID, timeT duration);

  /// Returned event is on heap; caller takes responsibility for ownership
  Event *getAsEvent(timeT absoluteTime) const;
  int    getChordSourceID(void) const
  { return m_chordSourceID; }
  int    getFigurationSourceID(void) const
  { return m_figurationSourceID; }
  const std::string NotationString(void) const;
  void setChordSourceID(int id)
  { m_chordSourceID = id; }
  void setFigurationSourceID(int id)
  { m_figurationSourceID = id; }

private:
  long m_chordSourceID;
  long m_figurationSourceID;
  timeT m_duration;
};

}

#endif /* ifndef RG_GENERATEDREGION_H */
