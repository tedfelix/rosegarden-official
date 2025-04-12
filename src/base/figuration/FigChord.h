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

#ifndef RG_FIGCHORD_H
#define RG_FIGCHORD_H

#include "base/Sets.h"
#include "base/Segment.h"

namespace Rosegarden
{

  class NotationQuantizer;

/// Class to describe a chord whose notes might not start at the same
/// time
/// @class ChordFromCounterpoint
/// @author Tom Breton (Tehom)
class ChordFromCounterpoint : public GenericChord<Event, Segment, false>
{
    typedef Event    Element;
    typedef Segment  Container;
    static const bool singleStaff = false;

public:
    ChordFromCounterpoint(Container &c,
                      Iterator elementInChord,
                      const Quantizer *quantizer,
                      timeT preDuration)
        : GenericChord<Element, Container, singleStaff>
              (c, elementInChord, quantizer),
          m_preDuration(preDuration)
          { initialise(); }

protected:
    bool     test(const Iterator &i) override;
    bool     sample(const Iterator &i, bool goingForwards) override;
    // The longest duration we expect a preceding note to have.
    timeT    m_preDuration;
};

class FigChord : public ChordFromCounterpoint
{
public:
  FigChord(Segment& s, Segment::iterator i) :
    ChordFromCounterpoint(s, i, getQuantizer(), m_myPreDuration)
    {}
  static const Quantizer * getQuantizer();
private:
  static const timeT m_myPreDuration;
  static NotationQuantizer * m_nq;
};

class FindFigChords
{
 public:
 FindFigChords(Segment *chordSource, timeT startTime) :
    m_chordSource(chordSource),
        m_iter(chordSource->findTime(startTime)),
        m_timePreviousChord(startTime)
        {}

    FigChord * getChordNow(timeT timeLimit);
    FindFigChords &operator++();
    timeT timeNow() const { return m_timePreviousChord; }

 private:
    Segment *m_chordSource;
    Segment::iterator m_iter;
    timeT m_timePreviousChord;
};

}

#endif /* ifndef RG_FIGCHORD_H */
