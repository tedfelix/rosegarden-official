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

#include "FigChord.h"
#include "base/NotationTypes.h"
#include "base/NotationQuantizer.h"

namespace Rosegarden
{

/***** ChordFromCounterpoint *****/

/// Return true if i might be within the chord timewise.  We allow any
/// preceding event within m_preDuration of the target time.
/// @author Tom Breton (Tehom)
bool
ChordFromCounterpoint::test(const Iterator &i)
{
    timeT onTime = (*i)->getAbsoluteTime();
    // We don't use event's own duration here.  Using it wouldn't help
    // much, because we'd still mostly lose preceding notes if we
    // underestimate preDuration.
    return
        (onTime <= m_time) &&
        (onTime + m_preDuration > m_time);
}

/// Return true if i is definitely within the chord timewise.  Mostly
/// works via the base functions, which also do some assigning
/// @author Tom Breton (Tehom)
bool
ChordFromCounterpoint::sample(const Iterator &i, bool goingForwards)
{
    Event *e = getAsEvent(i);
    timeT onTime    = e->getAbsoluteTime();
    timeT duration  = e->getDuration();
    
    if (onTime + duration <= m_time) { return false; }
    if (onTime > m_time) { return false; }
    return GenericChord<Element, Container, singleStaff>::sample(i, goingForwards);
}

/***** FigChord itself *****/

// Choose a whole note as a reasonable maximum duration for
// preceding notes to have.
const timeT
FigChord::
m_preDuration = Note(Note::WholeNote).getDuration();

NotationQuantizer * FigChord::m_nq = 0;

const Quantizer *
FigChord::getQuantizer(void)
{
  if (!m_nq) { m_nq = new NotationQuantizer; }
  return m_nq;
}

FigChord *
FindFigChords::
getChordNow(timeT timeLimit)
{
    if (m_iter == m_chordSource->end())
        { return 0; }
    else if ((*m_iter)->getAbsoluteTime() >= timeLimit)
        { return 0; }
    else
        { return new FigChord(*m_chordSource, m_iter); }
}

FindFigChords &
FindFigChords::
operator++(void)
{
    for (;m_iter != m_chordSource->end(); ++m_iter) {
        Event *e = *m_iter;
        // Events that are part of a previous chord don't imply a new
        // chord. 
        if (e->getAbsoluteTime() <= m_timePreviousChord)
            { continue; }
        // Non-notes don't imply a new chord.
        if (!e->isa(Note::EventType))
            { continue; }

        // OK, we have a new chord.
        const timeT timeNow = e->getAbsoluteTime();
        m_timePreviousChord = timeNow;
        break;
    }
    return *this;
}

}
