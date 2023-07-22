/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2023 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "TimeSignature.h"

//#include "NotationRules.h"
//#include "base/BaseProperties.h"


namespace Rosegarden
{


// This is the fundamental definition of the resolution used throughout.
// It must be a multiple of 16, and should ideally be a multiple of 96.
// ??? See "timebase" in TimeT.h.  Probably should switch to that everywhere.
constexpr timeT basePPQ = 960;


//////////////////////////////////////////////////////////////////////
// TimeSignature
//////////////////////////////////////////////////////////////////////

const std::string TimeSignature::EventType = "timesignature";

const PropertyName TimeSignature::NumeratorPropertyName("numerator");
const PropertyName TimeSignature::DenominatorPropertyName("denominator");
const PropertyName TimeSignature::ShowAsCommonTimePropertyName("common");
const PropertyName TimeSignature::IsHiddenPropertyName("hidden");
const PropertyName TimeSignature::HasHiddenBarsPropertyName("hiddenbars");

constexpr timeT crotchetTime = basePPQ;
constexpr timeT dottedCrotchetTime = basePPQ + basePPQ/2;

TimeSignature::TimeSignature(int numerator, int denominator,
                             bool preferCommon, bool hidden, bool hiddenBars)
    // throw (BadTimeSignature)
    : m_numerator(numerator), m_denominator(denominator),
      m_common(preferCommon &&
               (m_denominator == m_numerator &&
                (m_numerator == 2 || m_numerator == 4))),
      m_hidden(hidden),
      m_hiddenBars(hiddenBars)
{
    if (numerator < 1 || denominator < 1) {
        throw BadTimeSignature("Numerator and denominator must be positive");
    }
}

TimeSignature::TimeSignature(const Event &e)
    // throw (Event::NoData, Event::BadType, BadTimeSignature)
{
    if (e.getType() != EventType) {
        throw Event::BadType("TimeSignature model event", EventType, e.getType());
    }
    m_numerator = 4;
    m_denominator = 4;

    if (e.has(NumeratorPropertyName)) {
        m_numerator = e.get<Int>(NumeratorPropertyName);
    }

    if (e.has(DenominatorPropertyName)) {
        m_denominator = e.get<Int>(DenominatorPropertyName);
    }

    m_common = false;
    e.get<Bool>(ShowAsCommonTimePropertyName, m_common);

    m_hidden = false;
    e.get<Bool>(IsHiddenPropertyName, m_hidden);

    m_hiddenBars = false;
    e.get<Bool>(HasHiddenBarsPropertyName, m_hiddenBars);

    if (m_numerator < 1 || m_denominator < 1) {
        throw BadTimeSignature("Numerator and denominator must be positive");
    }
}

timeT TimeSignature::getBarDuration() const
{
    // Update the cache.
    setInternalDurations();

    return m_barDuration;
}

timeT TimeSignature::getBeatDuration() const
{
    setInternalDurations();
    return m_beatDuration;
}

timeT TimeSignature::getUnitDuration() const
{
    return crotchetTime * 4 / m_denominator;
}

Note::Type TimeSignature::getUnit() const
{
    int c, d;
    for (c = 0, d = m_denominator; d > 1; d /= 2) ++c;
    return Note::Semibreve - c;
}

Event *TimeSignature::getAsEvent(timeT absoluteTime) const
{
    constexpr int EventSubOrdering = -150;

    Event *e = new Event(EventType, absoluteTime, 0, EventSubOrdering);
    e->set<Int>(NumeratorPropertyName, m_numerator);
    e->set<Int>(DenominatorPropertyName, m_denominator);
    e->set<Bool>(ShowAsCommonTimePropertyName, m_common);
    e->set<Bool>(IsHiddenPropertyName, m_hidden);
    e->set<Bool>(HasHiddenBarsPropertyName, m_hiddenBars);
    return e;
}

// This doesn't consider subdivisions of the bar larger than a beat in
// any time other than 4/4, but it should handle the usual time signatures
// correctly (compound time included).

void TimeSignature::getDurationListForInterval(DurationList &dlist,
                                               timeT duration,
                                               timeT startOffset) const
{
    // Update the cache.
    setInternalDurations();

    timeT offset = startOffset;
    timeT durationRemaining = duration;

    while (durationRemaining > 0) {

        // Everything in this loop is of the form, "if we're on a
        // [unit] boundary and there's a [unit] of space left to fill,
        // insert a [unit] of time."

        // See if we can insert a bar of time.

        if (offset % m_barDuration == 0
            && durationRemaining >= m_barDuration) {

            getDurationListForBar(dlist);
            durationRemaining -= m_barDuration,
                offset += m_barDuration;

        }

        // If that fails and we're in 4/4 time, see if we can insert a
        // half-bar of time.

        //_else_ if!
        else if (m_numerator == 4 && m_denominator == 4
                 && offset % (m_barDuration/2) == 0
                 && durationRemaining >= m_barDuration/2) {

            dlist.push_back(m_barDuration/2);
            durationRemaining -= m_barDuration/2;
            offset += m_barDuration/2;

        }

        // If that fails, see if we can insert a beat of time.

        else if (offset % m_beatDuration == 0
                 && durationRemaining >= m_beatDuration) {

            dlist.push_back(m_beatDuration);
            durationRemaining -= m_beatDuration;
            offset += m_beatDuration;

        }

        // If that fails, see if we can insert a beat-division of time
        // (half the beat in simple time, a third of the beat in compound
        // time)

        else if (offset % m_beatDivisionDuration == 0
                 && durationRemaining >= m_beatDivisionDuration) {

            dlist.push_back(m_beatDivisionDuration);
            durationRemaining -= m_beatDivisionDuration;
            offset += m_beatDivisionDuration;

        }

        // cc: In practice, if the time we have remaining is shorter
        // than our shortest note then we should just insert a single
        // unit of the correct time; we won't be able to do anything
        // useful with any shorter units anyway.

        else if (durationRemaining <= Note(Note::Shortest).getDuration()) {

            dlist.push_back(durationRemaining);
            offset += durationRemaining;
            durationRemaining = 0;

        }

        // If that fails, keep halving the beat division until we
        // find something to insert. (This could be part of the beat-division
        // case; it's only in its own place for clarity.)

        else {

            timeT currentDuration = m_beatDivisionDuration;

            while ( !(offset % currentDuration == 0
                      && durationRemaining >= currentDuration) ) {

                if (currentDuration <= Note(Note::Shortest).getDuration()) {

                    // okay, this isn't working.  If our duration takes
                    // us past the next beat boundary, fill with an exact
                    // rest duration to there and then continue  --cc

                    timeT toNextBeat =
                        m_beatDuration - (offset % m_beatDuration);

                    if (durationRemaining > toNextBeat) {
                        currentDuration = toNextBeat;
                    } else {
                        currentDuration  = durationRemaining;
                    }
                    break;
                }

                currentDuration /= 2;
            }

            dlist.push_back(currentDuration);
            durationRemaining -= currentDuration;
            offset += currentDuration;

        }

    }

}

void TimeSignature::getDurationListForBar(DurationList &dlist) const
{
    // Note: Although this does not call setInternalDurations() to update
    //       the cache, this routine's caller does.  So the cache will
    //       be correct for this routine.

    // If the bar's length can be represented with one long symbol, do it.
    // Otherwise, represent it as individual beats.

    if (m_barDuration == crotchetTime ||
        m_barDuration == crotchetTime * 2 ||
        m_barDuration == crotchetTime * 4 ||
        m_barDuration == crotchetTime * 8 ||
        m_barDuration == dottedCrotchetTime ||
        m_barDuration == dottedCrotchetTime * 2 ||
        m_barDuration == dottedCrotchetTime * 4 ||
        m_barDuration == dottedCrotchetTime * 8) {

        dlist.push_back(getBarDuration());

    } else {

        for (int i = 0; i < getBeatsPerBar(); ++i) {
            dlist.push_back(getBeatDuration());
        }

    }

}

int TimeSignature::getEmphasisForTime(timeT offset) const
{
    setInternalDurations();

    if      (offset % m_barDuration == 0)
        return 4;
    else if (m_numerator == 4 && m_denominator == 4 &&
             offset % (m_barDuration/2) == 0)
        return 3;
    else if (offset % m_beatDuration == 0)
        return 2;
    else if (offset % m_beatDivisionDuration == 0)
        return 1;
    else
        return 0;
}


void TimeSignature::getDivisions(int depth, std::vector<int> &divisions) const
{
    divisions.clear();

    if (depth <= 0)
        return;

    // !!! Calls setInternalDurations().
    timeT base = getBarDuration();

/*
    if (m_numerator == 4 && m_denominator == 4) {
        divisions.push_back(2);
        base /= 2;
        --depth;
    }
    if (depth <= 0) return;
*/

    divisions.push_back(base / m_beatDuration);
    --depth;

    if (depth <= 0) return;

    if (m_dotted) divisions.push_back(3);
    else divisions.push_back(2);
    --depth;

    while (depth > 0) {
        divisions.push_back(2);
        --depth;
    }

    return;
}


void TimeSignature::setInternalDurations() const
{
    const int unitLength = crotchetTime * 4 / m_denominator;

    m_barDuration = m_numerator * unitLength;

    // Is 3/8 dotted time?  This will report that it isn't, because of
    // the check for m_numerator > 3 -- but otherwise we'd get a false
    // positive with 3/4

    // [rf] That's an acceptable answer, according to my theory book. In
    // practice, you can say it's dotted time iff it has 6, 9, or 12 on top.

    m_dotted = (m_numerator % 3 == 0 &&
                m_numerator > 3 &&
                m_barDuration >= dottedCrotchetTime);

    if (m_dotted) {
        m_beatDuration = unitLength * 3;
        m_beatDivisionDuration = unitLength;
    }
    else {
        m_beatDuration = unitLength;
        m_beatDivisionDuration = unitLength / 2;
    }

}


} // end namespace Rosegarden

