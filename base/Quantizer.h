
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

/**
  *@author Guillaume Laurent, Chris Cannam, Richard Bown
  */

namespace Rosegarden {

class Quantizer
{
public:
    /** The quantizer can do two sorts of quantization: unit and note.
        Unit quantization is the usual sort a sequencer will expect;
        each event has its duration rounded to an integral multiple of
        a given unit duration.  Note quantization instead rounds each
        event to the closest duration expressible as a single note
        with a maximum number of dots.

        For example, say you have an event with duration 110.  A unit
        quantizer with a hemidemisemi unit (duration 6) will quantize
        this to duration 108 (the nearest value divisible by 6).  But
        108 is not a good note duration: a note quantizer would
        quantize to 96 (the nearest note duration: a crotchet).

        If you request note quantization, it does unit quantization as
        well -- the results are stored in separate properties and do
        not conflict.
    */
    Quantizer(int unit = -1, int maxDots = 2) :
	m_unit(unit), m_maxDots(maxDots) {
	if (unit < 0) setUnit(Note(Note::Shortest));
    }

    ~Quantizer() { }

    static const std::string DurationProperty;
    static const std::string NoteDurationProperty;

    void setUnit(int unit)        { m_unit = unit; }
    void setUnit(Note note)	  { m_unit = note.getDuration(); }
    void setMaxDots(int maxDots)  { m_maxDots = maxDots; }

    /**
     * Quantizes a section of a track.  Sets the DurationProperty on
     * all events; does not change the event duration.  Sets the
     * property even on zero-duration (non-note) events.
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
     * Quantizes a section of a track.  Sets the DurationProperty,
     * NoteDurationProperty, Note::NoteType and Note::NoteDots
     * properties on all events; does not change the event duration.
     * Sets the first of those properties even on zero-duration
     * (non-note) events, but the others only on notes and rests.
     */
    void quantizeByNote(Track::iterator from, Track::iterator to) const;

    /**
     * Quantizes one event.  Sets the DurationProperty; does not
     * change the event duration
     *
     * @return Quantized duration (same as DurationProperty)
     */
    timeT quantizeByUnit(Rosegarden::Event *el) const;

    /**
     * Quantizes a duration.  
     *
     * @return Quantized duration
     */
    timeT quantizeByUnit(timeT duration) const;

    /**
     * Quantizes one event.  Sets the DurationProperty,
     * NoteDurationProperty, Note::NoteType and Note::NoteDots
     * properties; does not change the event duration.  If the event
     * is not a note or rest, only sets the first of those properties.
     *
     * @return Quantized note duration (same as NoteDurationProperty)
     */
    timeT quantizeByNote(Rosegarden::Event *el) const;

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
    Note requantizeByNote(timeT &unitQuantizedDuration) const;

    int m_unit;
    int m_maxDots;
};

}

#endif
