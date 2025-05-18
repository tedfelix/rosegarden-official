/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_TIMESIGNATURE_H
#define RG_TIMESIGNATURE_H

#include <rosegardenprivate_export.h>

#include "Event.h"
#include "NotationTypes.h"
#include "PropertyName.h"
#include "TimeT.h"

#include <list>


namespace Rosegarden
{


typedef std::list<int> DurationList;

/// A time signature represented as a numerator and a denominator.
/**
 * TimeSignature contains arithmetic methods relevant to time
 * signatures and bar durations, including code for splitting long
 * rest intervals into bite-sized chunks.  Although there is a
 * TimeSignature::EventType, time signature Events don't appear in
 * regular Segments but only in the Composition's reference Segment.
 */
class ROSEGARDENPRIVATE_EXPORT TimeSignature
{
public:

    // Ctors throw this if numerator or denominator are less than 1.
    typedef Exception BadTimeSignature;

    /// Default 4/4 time signature.
    TimeSignature()  { updateCache(); }

    /// Time signature for a given numerator and denominator.
    /**
     * @param[in] preferCommon Display the time signature as common or cut time.
     * @param[in] hidden Do not display the time signature.
     * @param[in] hiddenBars The bar lines between this time signature and the
     *            next will not be shown.
     */
    TimeSignature(int numerator, int denominator,
                  bool preferCommon = false,
                  bool hidden = false,
                  bool hiddenBars = false);

    // Called by Composition.
    explicit TimeSignature(const Event &e);

    int getNumerator() const  { return m_numerator; }
    int getDenominator() const  { return m_denominator; }

    /// Show as common or cut time.  If false, displays as 4/4 or 2/2.
    /**
     * @see NotePixmapFactory::makeTimeSig()
     */
    bool isCommon() const  { return m_common; }

    /// Whether the TimeSignature is displayed in notation.
    /**
     * @see NotationStaff::insertTimeSignature()
     */
    bool isHidden() const  { return m_hidden; }

    /// Whether to hide the bar lines between this time signature and the next.
    /**
     * @see StaffLayout::insertBar()
     */
    bool hasHiddenBars() const  { return m_hiddenBars; }

    // Computation Functions

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
    timeT getBeatDuration() const  { return m_beatDuration; }

    timeT getBarDuration() const  { return m_barDuration; }

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
     *
     * This doesn't consider subdivisions of the bar larger than a beat in
     * any time other than 4/4, but it should handle the usual time signatures
     * correctly (compound time included).
     *
     * @see Segment::fillWithRests()
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

    bool operator==(const TimeSignature &ts) const;
    bool operator!=(const TimeSignature &ts) const  { return !operator==(ts); }
    bool operator<(const TimeSignature &rhs) const;

private:

    int m_numerator = 4;
    int m_denominator = 4;
    bool m_common = false;

    bool m_hidden = false;
    bool m_hiddenBars = false;

    // Cached computed values.
    int m_barDuration = 0;
    int m_beatDuration = 0;
    int m_beatDivisionDuration = 0;
    bool m_dotted = false;
    void updateCache();

    /**
     * Get the "optimal" list of rest durations to make up a bar in
     * this time signature.
     */
    void getDurationListForBar(DurationList &dlist) const;

};


}


#endif
