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
    static const std::string DefaultPropertyNamePrefix;
    enum QuantizationType { UnitQuantize, NoteQuantize, LegatoQuantize };

    /**
     * Construct a quantizer programmed to do a single sort of
     * quantization.
     *
     * \arg propertyNamePrefix : common prefix for the property
     * names used to store quantized values in events; permits
     * use of more than one quantizer on a given event at a time.
     * Quantization is not destructive of the event's intrinsic
     * absolute time and duration.
     *
     * \arg type : Type of quantization to carry out, as follows:
     *
     *   "UnitQuantize": For note events, starting time and duration
     *   are rounded to the nearest multiple of a given unit duration
     *   (by default, the duration of the shortest available note).
     *   Rests are quantized in the same way, except where preceded by
     *   a note that has been lengthened by quantization, in which
     *   case the rest is shortened correspondingly before rounding.
     *   This is the simplest sort of quantization.
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
    Quantizer(std::string propertyNamePrefix = DefaultPropertyNamePrefix,
	      QuantizationType type = UnitQuantize,
	      timeT unit = -1, int maxDots = 2);

    /**
     * Construct a quantizer based on a standard quantization
     * setup.
     */
    Quantizer(const StandardQuantization &,
	      std::string propertyNamePrefix = DefaultPropertyNamePrefix);

    Quantizer(const Quantizer &);
    Quantizer &operator=(const Quantizer &);

    ~Quantizer();

    /**
     * Get the name of the property this Quantizer places the
     * quantized absolute time in on each event it quantizes.
     * This name will depend on the propertyNamePrefix passed
     * to the Quantizer's constructor.
     */
    PropertyName getAbsoluteTimeProperty() const {
	return m_absoluteTimeProperty;
    }

    /**
     * Get the name of the property this Quantizer places the
     * quantized duration in on each event it quantizes.
     * This name will depend on the propertyNamePrefix passed
     * to the Quantizer's constructor.
     */
    PropertyName getDurationProperty() const {
	return m_durationProperty;
    }

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
    void quantize(Segment::iterator from, Segment::iterator to) const;

    /**
     * Quantize a section of a Segment, and force the quantized
     * results into the formal absolute time and duration of
     * the events.  This is a destructive operation that should
     * not be carried out except on a user's explicit request.
     *
     * //!!! NB -- this will fail if quantization ever changes the
     * theoretical order of events, which might happen if two
     * events have different durations that quantize to the same
     * value but also have different suborderings.  Need to
     * consider this whole issue of allowing people to change
     * events' absolute times after they've been inserted.
     */
    void fixQuantizedValues(Segment::iterator from, Segment::iterator to)
	const;

    /**
     * Get the contents of the quantized duration property of
     * the given event, or quantize it if the property is so far
     * unset.  Could return an incorrect result if the real
     * duration of the event has changed since it was last
     * quantized.  Note that quantizing individual events may be
     * less reliable than quantizing whole Segments; this should
     * not be used as a substitute for batch-style quantization.
     */
    timeT getQuantizedDuration(Event *el) const;

    /**
     * Get the contents of the quantized absolute time property of
     * the given event, or quantize it if the property is so far
     * unset.  Note that quantizing individual events may be
     * less reliable than quantizing whole Segments; this should
     * not be used as a substitute for batch-style quantization.
     */
    timeT getQuantizedAbsoluteTime(Event *el) const;

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
    void unquantize(Segment::iterator from, Segment::iterator to) const;

    /**
     * Unquantize the given event, for this
     * quantizer.  Properties set by other quantizers with
     * different propertyNamePrefix values will remain.
     */
    void unquantize(Event *el) const;

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
		  const SingleQuantizer &absq, const SingleQuantizer &dq)
	const;

    timeT findFollowingRestDuration(Segment::iterator from,
				    Segment::iterator to) const;

    QuantizationType m_type;
    timeT m_unit;
    int m_maxDots;

    PropertyName m_absoluteTimeProperty;
    PropertyName m_durationProperty;
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
