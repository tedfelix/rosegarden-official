// -*- c-basic-offset: 4 -*-

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

#ifndef QUANTIZER_H
#define QUANTIZER_H

#include "Track.h"
#include "Event.h"
#include "NotationTypes.h"

namespace Rosegarden {

/**

    The Quantizer class quantizes starting time and duration for
    Events, and will also calculate quantized values for durations not
    in Events.  When quantizing an Event, the quantized values are
    stored in the Event in properties separate from the original
    duration, so quantization is non-destructive (apart from losing
    any previously quantized values).

    The quantizer can quantize events' starting times by unit, and
    events' durations by unit or by note.  The default behaviour is to
    quantize both start times and durations, so if you ask for note
    quantization you'll still get the start times quantized by unit.

    Unit quantization is the sort usually used when tidying recorded
    music in a sequencer: each event has its starting time and
    duration rounded to an integral multiple of a given unit time.

    Note quantization instead rounds the durations to the closest
    duration expressible as a single note with a maximum number of
    dots.

    For example, say you have an event with duration 110.  A unit
    quantizer with a hemidemisemi unit (duration 6) will quantize
    this to duration 108 (the nearest value divisible by 6).  But
    108 is not a good note duration: a note quantizer would
    instead quantize to 96 (the nearest note duration: a crotchet).
    
    In any case, if you request note quantization, it does unit
    quantization of the duration as well -- the results are stored in
    separate Event properties and do not conflict.  Note quantization
    is obviously slower though.

    If you just tell the quantizer to quantize, it will do note
    quantization and will quantize both starting times and durations.

    Note that quantizing a Track, or a section of a Track, is not the
    same as individually quantizing each of the events in that Track.
    The behaviour differs for rest events, which are treated the same
    as notes if treated in isolation but which are considered as gaps
    between the notes if part of a Track.

    (This introductory comment could really do with a rewrite.)
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
    Quantizer(int unit = -1, int maxDots = 2) :
	m_unit(unit), m_maxDots(maxDots) {
	if (unit < 0) setUnit(Note(Note::Shortest));
    }

    ~Quantizer() { }

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
     * Quantizes a section of a track.  Sets the DurationProperty and
     * AbsoluteTimeProperty on all events; does not change the event
     * duration.  Sets the property even on zero-duration (non-note)
     * events.
     */
    void quantizeByUnit(Track::iterator from, Track::iterator to) const;

    /**
     * Returns the DurationProperty if it exists; otherwise quantizes
     * the event by unit and then returns that property.  (If the event's
     * duration has been changed since it was last quantized, or the
     * last quantization used a different unit, this method may return
     * the wrong value.)
     */
    timeT getUnitQuantizedDuration(Rosegarden::Event *el) const;

    /**
     * Returns the AbsoluteTimeProperty if it exists; otherwise quantizes
     * the event by unit and then returns that property.  (If the event's
     * absolute time has been changed since it was last quantized, or the
     * last quantization used a different unit, this method may return
     * the wrong value.)
     */
    timeT getUnitQuantizedAbsoluteTime(Rosegarden::Event *el) const;

    /**
     * Quantizes a section of a track.  Sets the DurationProperty,
     * NoteDurationProperty, Note::NoteType and Note::NoteDots
     * properties on all events; does not change the event duration.
     * Sets the first of those properties even on zero-duration
     * (non-note) events, but the others only on notes and rests.
     */
    void quantizeByNote(Track::iterator from, Track::iterator to) const;

    /**
     * Quantizes a section of a track.  ...
     */
    void quantizeLegato(Track::iterator from, Track::iterator to) const;

    /**
     * Quantizes a duration.  
     *
     * @return Quantized duration
     */
    timeT quantizeByUnit(timeT duration) const;

    /**
     * Quantizes a duration.
     *
     * @return Duration of quantized note
     */
    timeT quantizeByNote(timeT duration) const;

    /**
     * Returns the NoteDurationProperty if it exists, otherwise
     * quantizes the event by note and then returns that property.
     * (If the event's duration has been changed since it was last
     * quantized, or the last quantization used a different maxDots
     * value, this may return the wrong value.)
     */
    timeT getNoteQuantizedDuration(Rosegarden::Event *el) const;

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
	virtual timeT quantize(int unit, int maxDots, timeT duration,
			       timeT followingRestDuration) const;
    };

    class LegatoQuantizer : public SingleQuantizer {
    public:
	virtual ~LegatoQuantizer();
	virtual timeT quantize(int unit, int maxDots, timeT duration,
			       timeT followingRestDuration) const;
    };

    void quantize(Track::iterator from, Track::iterator to,
		  const SingleQuantizer &absq, const SingleQuantizer &dq,
		  PropertyName durationProperty, bool legato) const;

    timeT findFollowingRestDuration(Track::iterator from,
				    Track::iterator to) const;

    int m_unit;
    int m_maxDots;
};

}

#endif
