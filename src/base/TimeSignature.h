/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

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

#ifndef RG_TIMESIGNATURE_H
#define RG_TIMESIGNATURE_H

#include <list>

#include <rosegardenprivate_export.h>

#include "Event.h"
#include "NotationTypes.h"

namespace Rosegarden
{


typedef std::list<int> DurationList;

/**
 * TimeSignature contains arithmetic methods relevant to time
 * signatures and bar durations, including code for splitting long
 * rest intervals into bite-sized chunks.  Although there is a time
 * signature Event type, these Events don't appear in regular Segments
 * but only in the Composition's reference segment.
 */
class ROSEGARDENPRIVATE_EXPORT TimeSignature
{
public:

    // Ctors throw this if numerator or denominator are less than 1.
    typedef Exception BadTimeSignature;

    // Default 4/4
    TimeSignature() :
        m_numerator(4),
        m_denominator(4),
        m_common(false),
        m_hidden(false),
        m_hiddenBars(false)
    { }

    /**
     * Construct a TimeSignature object describing a time signature
     * with the given numerator and denominator.  If preferCommon is
     * true and the time signature is a common or cut-common time, the
     * constructed object will return true for isCommon; if hidden is
     * true, the time signature is intended not to be displayed and
     * isHidden will return true; if hiddenBars is true, the bar lines
     * between this time signature and the next will not be shown.
     */
    TimeSignature(int numerator, int denominator,
                  bool preferCommon = false,
                  bool hidden = false,
                  bool hiddenBars = false);

    // Called by Composition.
    explicit TimeSignature(const Event &e);

    int getNumerator() const  { return m_numerator; }
    int getDenominator() const  { return m_denominator; }

    /// Show as "C", common/cut time?  If false, displays as 4/4 or 2/2.
    /**
     * @see NotePixmapFactory::makeTimeSig()
     */
    bool isCommon() const  { return m_common; }

    /// Whether the TimeSignature is displayed in notation.
    /**
     * @see NotationStaff::insertTimeSignature()
     */
    bool isHidden() const  { return m_hidden; }

    /// Hide the bar line for this TimeSignature.
    /**
     * @see StaffLayout::insertBar()
     */
    bool hasHiddenBars() const  { return m_hiddenBars; }

    timeT getBarDuration() const;

    /**
     * Return the unit of the time signature.  This is the note
     * implied by the denominator.  For example, the unit of 4/4 time
     * is the crotchet, and that of 6/8 is the quaver.  (The numerator
     * of the time signature gives the number of units per bar.)
     */
    Note::Type getUnit() const;

    /**
     * Return the duration of the unit of the time signature.
     * See also getUnit().  In most cases getBeatDuration() gives
     * a more meaningful value.
     */
    timeT getUnitDuration() const;

    /**
     * Return the duration of the beat of the time signature.  For
     * example, the beat of 4/4 time is the crotchet, the same as its
     * unit, but that of 6/8 is the dotted crotchet (there are only
     * two beats in a 6/8 bar).  The beat therefore depends on whether
     * the signature indicates dotted or undotted time.
     */
    timeT getBeatDuration() const;

    /**
     * Return the number of beats in a complete bar.
     */
    int getBeatsPerBar() const
            { return getBarDuration() / getBeatDuration(); }

    /**
     * Get the "optimal" list of rest durations to make up a time
     * interval of the given total duration, starting at the given
     * offset after the start of a bar, assuming that the interval
     * is entirely in this time signature.
     */
    void getDurationListForInterval(DurationList &dlist,
                                    timeT duration,
                                    timeT startOffset = 0) const;

    /**
     * Get the level of emphasis for a position in a bar. 4 is lots
     * of emphasis, 0 is none.
     */
    int getEmphasisForTime(timeT offset) const;

    /**
     * Return a list of divisions, subdivisions, subsubdivisions
     * etc of a bar in this time, up to the given depth.  For example,
     * if the time signature is 6/8 and the depth is 3, return a list
     * containing 2, 3, and 2 (there are 2 beats to the bar, each of
     * which is best subdivided into 3 subdivisions, each of which
     * divides most neatly into 2).
     */
    void getDivisions(int depth, std::vector<int> &divisions) const;

    /// Returned event is on heap; caller takes responsibility for ownership
    Event *getAsEvent(timeT absoluteTime) const;

    static const std::string EventType;
    static const PropertyName NumeratorPropertyName;
    static const PropertyName DenominatorPropertyName;
    static const PropertyName ShowAsCommonTimePropertyName;
    static const PropertyName IsHiddenPropertyName;
    static const PropertyName HasHiddenBarsPropertyName;

    // Operators

    bool operator==(const TimeSignature &ts) const
    {
        return ts.m_numerator == m_numerator && ts.m_denominator == m_denominator;
    }
    bool operator!=(const TimeSignature &ts) const  { return !operator==(ts); }
    bool operator<(const TimeSignature &rhs) const
    {
        // We don't really need ordered time signatures, but to be able to
        // create a map keyed with time signatures. We want to distinguish
        // 4/4 from 2/4 as well as 4/4 from 2/2.

        const double ratioLHS = (double)m_numerator / (double)m_denominator;
        const double ratioRHS =
                (double)rhs.m_numerator / (double)rhs.m_denominator;

        if (ratioLHS == ratioRHS)
            return m_denominator > rhs.m_denominator;
        else
            return ratioLHS < ratioRHS;
    }

private:

    int m_numerator;
    int m_denominator;
    bool m_common;

    bool m_hidden;
    bool m_hiddenBars;

    // Cached values.
    //
    // ??? These are recomputed every time they are set or used.  That's
    //     not what a cache is for.  Generally a cache is an optimization
    //     that avoids work.  This just creates more.  Recommend moving toward
    //     a more sensible approach.  First, remove mutable.  Then make sure
    //     these are always updated by the setters only.
    //
    mutable int m_barDuration = 0;
    mutable int m_beatDuration = 0;
    mutable int m_beatDivisionDuration = 0;
    mutable bool m_dotted = false;
    void setInternalDurations() const;

    /**
     * Get the "optimal" list of rest durations to make up a bar in
     * this time signature.
     */
    void getDurationListForBar(DurationList &dlist) const;

};


}


#endif
