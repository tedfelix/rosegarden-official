
/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2001
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "notationsets.h"
#include "notationproperties.h"
#include "rosedebug.h"
#include "Equation.h"
#include <cstring>

using Rosegarden::Event;
using Rosegarden::String;
using Rosegarden::Int;
using Rosegarden::Bool;
using Rosegarden::Clef;
using Rosegarden::Key;
using Rosegarden::Note;
using Rosegarden::Equation;

NotationSet::NotationSet(const NotationElementList &nel,
                         NELIterator i, bool quantized) :
    m_nel(nel),
    m_initial(nel.end()),
    m_final(nel.end()),
    m_shortest(nel.end()),
    m_longest(nel.end()),
    m_highest(nel.end()),
    m_lowest(nel.end()),
    m_quantized(quantized),
    m_baseIterator(i)
{
    // ...
}

void
NotationSet::initialise()
{
    NELIterator &i(m_baseIterator);
    if (i == m_nel.end() || !test(i)) return;
    sample(i);

    NELIterator j;

    // first scan back to find an element not in the desired set, and
    // leave ipair.first pointing to the one after it

    for (j = i; i != m_nel.begin() && test(--j); i = j) {
        sample(j);
    }
    m_initial = i;

    // then scan forwards to find an element not in the desired set,
    // and leave ipair.second pointing to the one before it

    for (j = i; ++j != m_nel.end() && test(j); i = j) {
        sample(j);
    }
    m_final = i;
}

void
NotationSet::sample(const NELIterator &i)
{
    Event::timeT d(durationOf(i, m_quantized));

    if (d > 0) {
        if (m_longest == m_nel.end() ||
            d > durationOf(m_longest, m_quantized)) {
            m_longest = i;
        }
        if (m_shortest == m_nel.end() ||
            d < durationOf(m_shortest, m_quantized)) {
            m_shortest = i;
        }
    }

    if ((*i)->isNote()) {
        long p = (*i)->event()->get<Int>("pitch");

        if (m_highest == m_nel.end() ||
            p > (*m_highest)->event()->get<Int>("pitch")) {
            m_highest = i;
        }
        if (m_lowest == m_nel.end() ||
            p < (*m_lowest)->event()->get<Int>("pitch")) {
            m_lowest = i;
        }
    }
}

Event::timeT
NotationSet::durationOf(const NELIterator &i, bool quantized)
{
    if (quantized) {
        long d;
        bool done = (*i)->event()->get<Int>(P_QUANTIZED_DURATION, d);
        if (done) {
            return d;
        } else {
            Quantizer().quantize((*i)->event());
            return (*i)->event()->get<Int>(P_QUANTIZED_DURATION);
        }
    } else {
        return (*i)->event()->getDuration();
    }
}

NotationSet::NELIterator
NotationSet::getInitialNote() const
{
    NELIterator i(getInitialElement());
    if ((*i)->isNote()) return i;

    NELIterator j(getFinalElement());
    ++j;
    while (i != j) {
        if ((*i)->isNote()) return i;
        ++i;
    }

    return getList().end();
}

NotationSet::NELIterator
NotationSet::getFinalNote() const
{
    NELIterator i(getFinalElement());
    if ((*i)->isNote()) return i;

    NELIterator j(getInitialElement());
    --j;
    while (i != j) {
        if ((*i)->isNote()) return i;
        --i;
    }

    return getList().end();
}


class PitchGreater {
public:
    bool operator()(const NotationElementList::iterator &a,
                    const NotationElementList::iterator &b) {
        try {
            return ((*a)->event()->get<Int>("pitch") <
                    (*b)->event()->get<Int>("pitch"));
        } catch (Event::NoData) {
            kdDebug(KDEBUG_AREA) << "Bad karma: PitchGreater failed to find one or both pitches" << endl;
            return false;
        }
    }
};


Chord::Chord(const NotationElementList &nel, NELIterator i, bool quantized) :
    NotationSet(nel, i, quantized),
    m_time((*i)->getAbsoluteTime())
{
    initialise();

    if (size() > 1) {
        std::stable_sort(begin(), end(), PitchGreater());
    }

    kdDebug(KDEBUG_AREA) << "Chord::Chord: pitches are:" << endl;
    for (unsigned int i = 0; i < size(); ++i) {
        try {
            kdDebug(KDEBUG_AREA) << i << ": " << (*(*this)[i])->event()->get<Int>("pitch") << endl;
        } catch (Event::NoData) {
            kdDebug(KDEBUG_AREA) << i << ": no pitch property" << endl;
        }
    }
}


NotationGroup::NotationGroup(const NotationElementList &nel,
                             NELIterator i, const Clef &clef, const Key &key) :
    NotationSet(nel, i, false),
    //!!! What if the clef and/or key change in the course of the group?
    m_clef(clef),
    m_key(key),
    m_weightAbove(0),
    m_weightBelow(0),
    m_type(Beamed)
{
    if (!(*i)->event()->get<Rosegarden::Int>(P_GROUP_NO, m_groupNo))
        m_groupNo = -1;

    initialise();
    
    if ((i = getInitialElement()) != getList().end()) {

        try {
            std::string t = (*i)->event()->get<String>(P_GROUP_TYPE);
            if (strcasecmp(t.c_str(), "beamed")) {
                m_type = Beamed;
            } else if (strcasecmp(t.c_str(), "tupled")) {
                m_type = Tupled;
            } else if (strcasecmp(t.c_str(), "grace")) {
                m_type = Grace;
            } else {
                kdDebug(KDEBUG_AREA) << "NotationGroup::NotationGroup: Warning: Unknown GroupType \"" << t << "\", defaulting to Beamed" << endl;
            }
        } catch (Rosegarden::Event::NoData) {
            kdDebug(KDEBUG_AREA) << "NotationGroup::NotationGroup: Warning: No GroupType in grouped element, defaulting to Beamed" << endl;
        }
    }
}

void
NotationGroup::sample(const NELIterator &i)
{
    NotationSet::sample(i);

    // If the sum of the distances from the middle line to the notes
    // above the middle line exceeds the sum of the distances from the
    // middle line to the notes below, then the beam goes below.  We
    // can calculate the weightings here, as we construct the group.

    if (!(*i)->isNote()) return;

    // The code that uses the Group should not rely on the presence of
    // e.g. P_BEAM_GRADIENT to indicate that a beam should be drawn;
    // it's possible the gradient might be left over from a previous
    // calculation and the group might have changed since.  Instead it
    // should test P_BEAM_NECESSARY, which may be false even if there
    // is a gradient present.
    (*i)->event()->setMaybe<Bool>(P_BEAM_NECESSARY, false);

    int h = height(i);
    if (h > 4) m_weightAbove += h - 4;
    if (h < 4) m_weightBelow += 4 - h;
}

int
NotationGroup::height(const NELIterator &i)
{
    long h;
    if ((*i)->event()->get<Int>(P_HEIGHT_ON_STAFF, h)) return h;
    int pitch = (*i)->event()->get<Int>("pitch");
    Rosegarden::NotationDisplayPitch p(pitch, m_clef, m_key);
    h = p.getHeightOnStaff();
    // not setMaybe, as we know the property is absent:
    (*i)->event()->set<Int>(P_HEIGHT_ON_STAFF, h, false);
    return h;
}

void
NotationGroup::applyBeam()
{
    Beam beam;
    beam.aboveNotes = !(m_weightAbove > m_weightBelow);
    beam.gradient = 0;
    beam.startHeight = 0;
    beam.necessary = false;
    
    NELIterator initialNote(getInitialNote()),
                  finalNote(  getFinalNote());

    if (initialNote == getList().end() ||
        initialNote == finalNote) {
        return; // no notes, no case to answer
    }

    Chord initialChord(getList(), initialNote),
            finalChord(getList(),   finalNote);

    if (initialChord.getInitialElement() == finalChord.getInitialElement()) {
        return;
    }

    int initialHeight, finalHeight, extremeHeight;
    NELIterator extremeNote;

    if (beam.aboveNotes) {
        initialHeight = height(initialChord.getHighestNote());
          finalHeight = height(  finalChord.getHighestNote());
        extremeHeight = height(             getHighestNote());
        extremeNote = getHighestNote();

    } else {
        initialHeight = height(initialChord.getLowestNote());
          finalHeight = height(  finalChord.getLowestNote());
        extremeHeight = height(             getLowestNote());
        extremeNote = getLowestNote();
    }
        
    int diff = initialHeight - finalHeight;
    if (diff < 0) diff = -diff;

    bool linear =
        (beam.aboveNotes ?
         (extremeHeight <= std::max(initialHeight, finalHeight)) :
         (extremeHeight >= std::min(initialHeight, finalHeight)));

    if (!linear) {
        if (diff > 2) diff = 1;
        else diff = 0;
    }

    // some magic numbers
    if (diff > 4) beam.gradient = 30;
    else if (diff > 3) beam.gradient = 17;
    else if (diff > 0) beam.gradient = 10;
    else beam.gradient = 0;

    if (initialHeight > finalHeight) beam.gradient = -beam.gradient;

    // Now, we need to judge the height of the beam such that the
    // nearest note of the whole group, the nearest note of the first
    // chord and the nearest note of the final chord are all at least
    // two note-body-heights away from it, and at least one of the
    // start and end points is at least the usual note stalk-length
    // away from it.  This is a straight-line equation y = mx + c,
    // where we have m and two x,y pairs and need to find c.
    
    int  initialX = (int)(*initialNote)->getLayoutX();
    int   finalDX = (int)  (*finalNote)->getLayoutX() - initialX;
    int extremeDX = (int)(*extremeNote)->getLayoutX() - initialX;

    int c0 = initialHeight, c1, c2;
    double dgrad = (double)beam.gradient / 100.0;
    Equation::solve(Equation::C, extremeHeight, dgrad, extremeDX, c1);
    Equation::solve(Equation::C,   finalHeight, dgrad,   finalDX, c2);

    using std::max;
    using std::min;
    long shortestNoteType = Note::Quaver;
    if (!(*getShortestElement())->event()->get<Int>(P_NOTE_TYPE,
                                                    shortestNoteType)) {
        kdDebug(KDEBUG_AREA) << "NotationGroup::calculateBeam: WARNING: Shortest element has no note-type; should this be possible?" << endl;
    }

    if (beam.aboveNotes) {
        beam.startHeight =
            max(max(c0, c1),
                max(c2, min(initialHeight + 6,
                              finalHeight + 6)));
        if (shortestNoteType < Note::Quaver)
            beam.startHeight += (Note::Quaver - shortestNoteType);
    } else {
        beam.startHeight =
            min(min(c0, c1),
                min(c2, max(initialHeight - 6,
                              finalHeight - 6)));
        if (shortestNoteType < Note::Quaver)
            beam.startHeight -= (Note::Quaver - shortestNoteType);
    }  

    Event::timeT crotchet = Note(Note::Crotchet).getDuration();
    beam.necessary =
         (*initialNote)->event()->getDuration() < crotchet
        && (*finalNote)->event()->getDuration() < crotchet;

    kdDebug(KDEBUG_AREA) << "NotationGroup::applyBeam: beam data:" << endl
                         << "gradient: " << beam.gradient << endl
                         << "start height: " << beam.startHeight << endl
                         << "aboveNotes: " << beam.aboveNotes << endl
                         << "necessary: " << beam.necessary << endl;

    for (NELIterator i = initialNote; i != getList().end(); ++i) {

        if ((*i)->isNote()) {
            (*i)->event()->setMaybe<Bool>(P_BEAM_NECESSARY, beam.necessary);

            if (beam.necessary) {
                (*i)->event()->setMaybe<Bool>(P_STALK_UP, beam.aboveNotes);
                (*i)->event()->setMaybe<Bool>(P_DRAW_TAIL, false);
                (*i)->event()->setMaybe<Int>(P_BEAM_GRADIENT, beam.gradient);
                (*i)->event()->setMaybe<Int>(P_BEAM_START_HEIGHT,
                                             beam.startHeight);
                (*i)->event()->setMaybe<Int>(P_BEAM_RELATIVE_X,
                                             (int)(*i)->getLayoutX()-initialX);
            }
        }

        if (i == finalNote) break;
    }
}

