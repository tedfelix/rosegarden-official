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
#include "FastVector.h"
#include <string>

namespace Rosegarden {

struct StandardQuantization;

/**
   The Quantizer class rounds the starting times and durations of note
   and rest events according to one of a set of possible criteria.
*/

class Quantizer
{
public:
    static const std::string RawEventData;
    static const std::string DefaultTarget;

    enum QuantizationType {
	PositionQuantize,// Snap absolute times to unit boundaries
	UnitQuantize,    // Snap absolute times and durations to unit boundaries
	NoteQuantize,    // Snap times to units, durations to note durations
	LegatoQuantize   // Note quantize that rounds up notes into following rests
    };

    /**
     * Construct a quantizer programmed to do a single sort of
     * quantization.
     *
     * \arg source, target : Description of where to find the
     * times to be quantized, and where to put the quantized results.
     * 
     * These may be strings, specifying a prefix for the names
     * of properties to contain the timings, or may be the special
     * value RawEventData in which case the event's absolute time
     * and duration will be used instead of properties.
     * 
     * If source specifies a property prefix for properties that are
     * found not to exist, they will be pre-filled from the original
     * timings in the target values before being quantized and then
     * set back into the target.  (This permits a quantizer to write
     * directly into the Event's absolute time and duration without
     * losing the original values, because they are backed up
     * automatically into the source properties.)
     * 
     * Note that because it's impossible to modify the duration or
     * absolute time of an event after construction, if target is
     * RawEventData the quantizer must re-construct each event in
     * order to adjust its timings.  This operation (deliberately)
     * loses any non-persistent properties in the events.  This
     * does not happen if target is a property prefix.
     *
     *   Examples:
     *
     *   -- if source == RawEventData and target == "MyPrefix",
     *   values will be read from the event's absolute time and
     *   duration, quantized, and written into MyPrefixAbsoluteTime
     *   and MyPrefixDuration properties on the event.  A call to
     *   unquantize will simply delete these properties.
     *
     *   -- if source == "MyPrefix" and target == RawEventData,
     *   the MyPrefixAbsoluteTime and MyPrefixDuration will be
     *   populated if necessary from the event's absolute time and
     *   duration, and then quantized and written back into the
     *   event's values.  A call to unquantize will write the
     *   MyPrefix-property timings back into the event's values,
     *   and delete the MyPrefix properties.
     *
     *   -- if source == "YourPrefix" and target == "MyPrefix",
     *   values will be read from YourPrefixAbsoluteTime and
     *   YourPrefixDuration, quantized, and written into the
     *   MyPrefix-properties.  This may be useful for piggybacking
     *   onto another quantizer's output.
     *
     *   -- if source == RawEventData and target == RawEventData,
     *   values will be read from the event's absolute time and
     *   duration, quantized, and written back to these values.
     *
     * \arg type : Type of quantization to carry out, as follows:
     *
     *   "PositionQuantize": For note events, starting time is rounded
     *   to the nearest multiple of a given unit duration (by default,
     *   the duration of the shortest available note).  Rests are
     *   quantized in the same way, except where preceded by a note
     *   that has been relocated by quantization, in which case the
     *   rest is adjusted correspondingly before rounding.   This is
     *   the simplest sort of quantization.
     *
     *   "UnitQuantize": For note events, starting time and duration
     *   are rounded to the nearest multiple of a given unit duration
     *   (by default, the duration of the shortest available note).
     *   Rests are quantized in the same way, except where preceded by
     *   a note that has been lengthened by quantization, in which
     *   case the rest is shortened correspondingly before rounding.
     * 
     *   "Note": Starting time is quantized as in unit quantization,
     *   but duration is first quantized by unit and then rounded to
     *   the nearest available note duration with a maximum of a given
     *   number of dots.
     * 
     *   "Legato": As for note quantization, except that the given
     *   unit (for the initial unit-quantization step) is only taken
     *   into account if examining a note event whose duration will be
     *   caused to increase and that is followed by enough rest space
     *   to permit that increase.  Otherwise, the minimum unit is
     *   used.  It is therefore normal to perform legato quantization
     *   with larger units than the other kinds.
     *
     *   [For example, say you have an event with duration 178.  A
     *   unit quantizer with a demisemi unit (duration 12) will
     *   quantize this to duration 180 (the nearest value divisible by
     *   12).  But 180 is not a good note duration: a note quantizer
     *   would instead quantize to 192 (the nearest note duration: a
     *   minim).]
     *
     * \arg unit : Quantization unit.  Default is the shortest note
     * duration.
     *
     * \arg maxDots : how many dots to allow on a note before
     * declaring it not a valid note type.  Only of interest for
     * note or legato quantization.
     *
     * Note that although the quantizer may give rest events a
     * duration of zero, it will never do so to note events -- you
     * can't make a note disappear completely by quantizing it.
     *
     * For best results, always quantize a whole segment or section
     * of segment at once.  The quantizer can only do the right thing
     * for rest events if given a whole section to consider at once.
     *
     * The configuration of a Quantizer can't be changed after
     * construction.  Instead, construct a new one and assign it
     * if necessary.  (Construction and assignment are cheap.)
     */
    Quantizer(std::string source = RawEventData,
	      std::string target = DefaultTarget,
	      QuantizationType type = UnitQuantize,
	      timeT unit = -1, int maxDots = 2);

    /**
     * Construct a quantizer based on a standard quantization
     * setup.
     */
    Quantizer(const StandardQuantization &,
	      std::string source = RawEventData,
	      std::string target = DefaultTarget);

    Quantizer(const Quantizer &);

    /**
     * Construct a quantizer by copying from another quantizer,
     * but with a different source and/or target (not defaulted
     * to avoid collision with the plain copy constructor, whose
     * source and target values have to come from the quantizer
     * being copied from)
     */
    Quantizer(const Quantizer &, std::string source, std::string target);

    Quantizer &operator=(const Quantizer &);
    bool operator==(const Quantizer &) const;
    bool operator!=(const Quantizer & c) const { return !(*this == c); }
    
    ~Quantizer();

    /**
     * Get the type of quantization this Quantizer performs.
     */
    QuantizationType getType() const { return m_type; }

    /**
     * Get the unit of the Quantizer.
     */
    timeT getUnit() const { return m_unit; }

    /**
     * If this is a Note or Legato Quantizer, get the maximum
     * number of dots permissible on a note before the quantizer
     * decides it's not a legal note.
     */
    int getMaxDots() const { return m_maxDots; }

    /**
     * Quantize a section of a Segment.  This is the recommended
     * method for general quantization.
     */
    void quantize(Segment *,
		  Segment::iterator from, Segment::iterator to) const;

    /**
     * Quantize a section of a Segment, and force the quantized
     * results into the formal absolute time and duration of
     * the events.  This is a destructive operation that should
     * not be carried out except on a user's explicit request.
     */
    void fixQuantizedValues(Segment *,
			    Segment::iterator from, Segment::iterator to)
	const;

    /**
     * Return the quantized duration of the event, by retrieving
     * it from the target if possible and otherwise by quantizing
     * the source duration.  In no case does this ever write the
     * quantized values into the event, so this is not a good
     * substitute for actually quantizing something.
     *
     * (If the target is RawEventData, this will always return
     * the raw duration regardless of whether the event appears
     * to have actually been quantized.  This method is thus
     * only useful if target is not RawEventData.)
     */
    timeT getQuantizedDuration(Event *el) const;

    /**
     * Return the quantized absolute time of the event, by
     * retrieving it from the target if possible and otherwise by
     * quantizing the source time.  In no case does this ever
     * write the quantized values into the event, so this is not
     * a good substitute for actually quantizing something.
     *
     * (If the target is RawEventData, this will always return
     * the raw time regardless of whether the event appears
     * to have actually been quantized.  This method is thus
     * only useful if target is not RawEventData.)
     */
    timeT getQuantizedAbsoluteTime(Event *el) const;

    /**
     * Return the unquantized absolute time of the event --
     * the absolute time that would be restored by a call to
     * unquantize.
     */
    timeT getUnquantizedAbsoluteTime(Event *el) const;

    /**
     * Return the unquantized absolute time of the event --
     * the absolute time that would be restored by a call to
     * unquantize.
     */
    timeT getUnquantizedDuration(Event *el) const;

    /**
     * Treat the given time as if it were the absolute time of
     * an Event, and return a quantized value.  (This may be
     * necessary for Note and Legato quantizers, to avoid rounding
     * absolute times to note-duration lengths.)
     */
    timeT quantizeAbsoluteTime(timeT absoluteTime) const;

    /**
     * Treat the given time as if it were the duration of
     * an Event, and return a quantized value.
     */
    timeT quantizeDuration(timeT duration) const;

    /**
     * Unquantize all events in the given range, for this
     * quantizer.  Properties set by other quantizers with
     * different propertyNamePrefix values will remain.
     */
    void unquantize(Segment *,
		    Segment::iterator from, Segment::iterator to) const;

protected:
    class SingleQuantizer {
    public:
	virtual ~SingleQuantizer();
	virtual timeT quantize(int unit, int maxDots, timeT duration,
			       timeT followingRestDuration,
			       bool isAbsoluteTime) const = 0;
    };

    class PositionQuantizer : public SingleQuantizer {
    public:
	virtual ~PositionQuantizer();
	virtual timeT quantize(int unit, int maxDots, timeT duration,
			       timeT followingRestDuration,
			       bool isAbsoluteTime) const;
    };

    class UnitQuantizer : public SingleQuantizer {
    public:
	virtual ~UnitQuantizer();
	virtual timeT quantize(int unit, int maxDots, timeT duration,
			       timeT followingRestDuration,
			       bool isAbsoluteTime) const;
    };

    class NoteQuantizer : public SingleQuantizer {
    public:
	virtual ~NoteQuantizer();
	virtual timeT quantize(int unit, int maxDots, timeT duration,
			       timeT followingRestDuration,
			       bool isAbsoluteTime) const;
    };

    class LegatoQuantizer : public NoteQuantizer {
    public:
	virtual ~LegatoQuantizer();
	virtual timeT quantize(int unit, int maxDots, timeT duration,
			       timeT followingRestDuration,
			       bool isAbsoluteTime) const;
    };

    void quantize(Segment *s, Segment::iterator from, Segment::iterator to,
		  const SingleQuantizer &absq, const SingleQuantizer &dq)
	const;

    timeT findFollowingRestDuration(Segment::iterator from,
				    Segment::iterator to) const;

    QuantizationType m_type;
    timeT m_unit;
    int m_maxDots;

    std::string m_source;
    std::string m_target;
    enum ValueType { AbsoluteTimeValue = 0, DurationValue = 1 };
    PropertyName m_sourceProperties[2];
    PropertyName m_targetProperties[2];

    timeT getFromSource(Event *, ValueType) const;
    timeT getFromTarget(Event *, ValueType) const;
    void setToTarget(Segment *, Segment::iterator, timeT t, timeT d) const;
    void removeProperties(Event *) const;
    void removeTargetProperties(Event *) const;
    void makePropertyNames();

    mutable FastVector<Event *> m_toInsert;
    void insertNewEvents(Segment *) const;
};

struct StandardQuantization {
    Quantizer::QuantizationType type;
    timeT			unit;
    int				maxDots;
    std::string			name;
    std::string			noteName;  // empty string if none
    
    StandardQuantization(Quantizer::QuantizationType _type,
			 timeT _unit, int _maxDots,
			 std::string _name,
			 std::string _noteName) :
	type(_type), unit(_unit), maxDots(_maxDots),
	name(_name), noteName(_noteName) { }

    static std::vector<StandardQuantization> getStandardQuantizations();
};

}

#endif
