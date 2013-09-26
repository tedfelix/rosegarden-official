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

#ifndef RG_FIGURATIONSOURCEMAP_H
#define RG_FIGURATIONSOURCEMAP_H

#include <vector>
#include <map>
#include <set>

namespace Rosegarden
{
  class Composition;
  class Segment;
  class Event;
  class Key;
  class FigChord;
  class NotationQuantizer;
  class RelativeEvent;
  typedef long timeT;


  typedef std::vector<RelativeEvent *> RelativeEventVec;

// @class Figuration
// @remarks A figuration ready for expansion
// @author Tom Breton (Tehom)
class Figuration
{
public:
  timeT        getEndTime(timeT startTime)
  { return startTime + m_duration; }
  
  RelativeEventVec m_events;
  timeT            m_duration;
  // Parameter count
  unsigned int     m_NbParameters;
  // Timesignature it applies to.
  int              m_timesigNumerator;
  int              m_timesigDenominator;
};

// @typedef FigurationVector
// A set of figurations sharing one source ID.
typedef std::vector<Figuration> FigurationVector;

// @class FigurationSourceMap
// A map from ID to FigurationVector.  Used for UpdateFigurationCommand.
class FigurationSourceMap : public std::map<int, FigurationVector>
{
    friend class BaseRelativeEventAdder;
public:
  typedef std::set <RelativeEvent *>       UnsolvedNote;
  typedef std::set<UnsolvedNote> UnsolvedFiguration;

  static FigurationVector getFigurations(Segment *s);
  static UnsolvedNote getPossibleRelations(Event *e,
					   const FigChord *parameterChord,
					   const Key key,
					   timeT startTime);

  static Figuration *findMatch(FigurationVector& figVector,
                               int timeSigNumerator,
                               int timeSigDenominator,
                               unsigned int NbParameters);
  void addSource(Segment* s, int id) {
    insert(value_type(id, getFigurations(s)));
  }
 private:

  // I take ownership of "e"
  static UnsolvedNote trivialUnsolvedNote(RelativeEvent *e) {
      UnsolvedNote trivialContainer;
      trivialContainer.insert(e);
      return trivialContainer;
  };
  
};
typedef FigurationSourceMap::value_type SourcedFiguration;
}

#endif /* ifndef RG_FIGURATIONSOURCEMAP_H */
