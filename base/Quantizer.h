// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
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

#ifndef QUANTIZER_H
#define QUANTIZER_H

#include "Segment.h"
#include "Event.h"
#include "NotationTypes.h"

namespace Rosegarden {

/**

   The Quantizer class rounds the starting times and durations of note
   and rest events according to one of a set of possible criteria.
   These criteria are:

   -- "Unit quantization": For note events, starting time and duration
   are rounded to the nearest multiple of a given unit duration (by
   default, the duration of the shortest available note).  Rests are
   quantized in the same way, except where preceded by a note that has
   been lengthened by quantization, in which case the rest is
   shortened correspondingly before rounding.  This is the simplest
   sort of quantization.

   -- "Note quantization": Starting time is quantized as in unit
   quantization, but duration is first quantized by unit and then
   rounded to the nearest available note duration with a maximum of a
   given number of dots.

   -- "Legato quantization": As for note quantization, except that the
   given unit (for the initial unit-quantization step) is only taken
   into account if examining a note event whose duration will be
   caused to increase and that is followed by enough rest space to
   permit that increase.  Otherwise, the minimum unit is used.  It is
   therefore normal to perform legato quantization with larger units
   than the other kinds.

   [For example, say you have an event with duration 178.  A unit
   quantizer with a demisemi unit (duration 12) will quantize this to
   duration 180 (the nearest value divisible by 12).  But 180 is not a
   good note duration: a note quantizer would instead quantize to 192
   (the nearest note duration: a minim).]

   The results of the quantization are stored in separate properties
   in each event; quantization is not destructive of the event's
   intrinsic absolute time and duration.

   Note that although the quantizer may give rest events a duration of
   zero, it will never do so to note events.

   For best results, always quantize a whole segment or section of segment
   at once.  The quantizer can only do the right thing for rest events
   if given a whole section to consider at once.
*/

class Quantizer
{
public:
    /**
     * Constructs a quantizer programmed to do unit quantization
     * to a resolution of "unit" time units (defaulting to the
     * shortest note duration), and note quantization with up to
     * "maxDots" dots per note.
     */
    Quantizer(int unit = -1, int maxDots = 2);
    ~Quantizer();

    static const PropertyName AbsoluteTimeProperty;
    static const PropertyName DurationProperty;
    static const PropertyName NoteDurationProperty;
    static const PropertyName LegatoDurationProperty;
    
    void setUnit(int unit)        { m_unit = unit; }
    void setUnit(Note note)	  { m_unit = note.getDuration(); }
    void setMaxDots(int maxDots)  { m_maxDots = maxDots; }

    int getUnit() const           { return m_unit; }
    int getMaxDots() const        { return m_maxDots; }

    /**
     * Unit-quantizes a section of a segment.  Sets the DurationProperty
     * and AbsoluteTimeProperty on all note and rest events; does not
     * change the event's intrinsic absoluteTime and duration.
     */
    void quantizeByUnit(Segment::iterator from, Segment::iterator to) const;

    /**
     * Note-quantizes a section of a segment.  Sets the
     * DurationProperty, NoteDurationProperty and AbsoluteTimeProperty
     * properties on all note and rest events; does not change the
     * event's intrinsic absoluteTime and duration.
     */
    void quantizeByNote(Segment::iterator from, Segment::iterator to) const;

    /**
     * Legato-quantizes a section of a segment.  Sets the
     * DurationProperty, LegatoDurationProperty and AbsoluteTimeProperty
     * properties on all note and rest events; does not change the
     * event's intrinsic absoluteTime and duration.
     */
    void quantizeLegato(Segment::iterator from, Segment::iterator to) const;

    /**
     * Unit-quantizes a section of a segment, then sets the absolute
     * time and duration of each event to its quantized values.  This
     * is a destructive operation that should only be carried out on
     * the user's explicit request.
     */
    void fixUnitQuantizedValues(Segment::iterator from, Segment::iterator to)
	const;

    /**
     * Note-quantizes a section of a segment, then sets the absolute
     * time and duration of each event to its quantized values.  This
     * is a destructive operation that should only be carried out on
     * the user's explicit request.
     */
    void fixNoteQuantizedValues(Segment::iterator from, Segment::iterator to)
	const;

    /**
     * Legato-quantizes a section of a segment, then sets the absolute
     * time and duration of each event to its quantized values.  This
     * is a destructive operation that should only be carried out on
     * the user's explicit request.
     */
    void fixLegatoQuantizedValues(Segment::iterator from, Segment::iterator to)
	const;

    /**
     * Returns the DurationProperty if it exists; otherwise quantizes
     * the event by unit and then returns that property.  If the
     * event's duration has been changed since it was last quantized,
     * or the last quantization used a different unit, this method may
     * return the wrong value.  Also, this method cannot take into
     * account the proper relationship between notes and rests; you
     * should always prefer to quantize whole segments where possible.
     */
    timeT getUnitQuantizedDuration(Rosegarden::Event *el) const;

    /**
     * Returns the AbsoluteTimeProperty if it exists; otherwise
     * quantizes the event by unit and then returns that property.  If
     * the event's absolute time has been changed since it was last
     * quantized, or the last quantization used a different unit, this
     * method may return the wrong value.  Also, this method cannot
     * take into account the proper relationship between notes and
     * rests; you should always prefer to quantize whole segments where
     * possible.
     */
    timeT getUnitQuantizedAbsoluteTime(Rosegarden::Event *el) const;

    /**
     * Returns the NoteDurationProperty if it exists, otherwise
     * quantizes the event by note and then returns that property.  If
     * the event's duration has been changed since it was last
     * quantized, or the last quantization used a different maxDots
     * value, this may return the wrong value.  Also, this method
     * cannot take into account the proper relationship between notes
     * and rests; you should always prefer to quantize whole segments
     * where possible.
     */
    timeT getNoteQuantizedDuration(Rosegarden::Event *el) const;

    /**
     * Unit-quantizes a single duration, assumed to be of a note
     * rather than a rest.
     */
    timeT quantizeByUnit(timeT duration) const;

    /**
     * Note-quantizes a single duration, assumed to be of a note
     * rather than a rest.
     */
    timeT quantizeByNote(timeT duration) const;

    /**
     * Removes the quantization properties from an event.  This is
     * necessary if you should change the duration of an event but
     * don't want to take the time to requantize it straight away.
     */
    void unquantize(Rosegarden::Event *el) const;

protected:
    class SingleQuantizer {
    public:
	virtual ~SingleQuantizer();
	virtual timeT getDuration(Event *event) const;
	virtual timeT quantize(int unit, int maxDots, timeT duration,
			       timeT followingRestDuration) const = 0;
    };

    class UnitQuantizer : public SingleQuantizer {
    public:
	virtual ~UnitQuantizer();
	virtual timeT quantize(int unit, int maxDots, timeT duration,
			       timeT followingRestDuration) const;
    };

    class NoteQuantizer : public SingleQuantizer {
    public:
	virtual ~NoteQuantizer();
	virtual timeT getDuration(Event *event) const;
	virtual timeT quantize(int unit, int maxDots, timeT duration,
			       timeT followingRestDuration) const;
    };

    class LegatoQuantizer : public NoteQuantizer {
    public:
	virtual ~LegatoQuantizer();
	virtual timeT quantize(int unit, int maxDots, timeT duration,
			       timeT followingRestDuration) const;
    };

    void quantize(Segment::iterator from, Segment::iterator to,
		  const SingleQuantizer &absq, const SingleQuantizer &dq,
		  PropertyName durationProperty, bool legato) const;

    timeT findFollowingRestDuration(Segment::iterator from,
				    Segment::iterator to) const;

    int m_unit;
    int m_maxDots;
};

}

#endif
