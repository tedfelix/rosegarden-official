
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
  *@author Guillaume Laurent, Chris Cannam, Rich Bown
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
    Quantizer() { }

    static const std::string DurationProperty;
    static const std::string NoteDurationProperty;

    /**
     * Quantizes a section of a track.  Sets the DurationProperty on
     * all events; does not change the event duration.  Sets the
     * property even on zero-duration (non-note) events.
     */
    void quantizeByUnit(Track::iterator from,
                        Track::iterator to,
                        int unit = Note::Shortest);

    /**
     * Quantizes a section of a track.  Sets the DurationProperty,
     * NoteDurationProperty, Note::NoteType and Note::NoteDots
     * properties on all events; does not change the event duration.
     * Sets the first of those properties even on zero-duration
     * (non-note) events, but the others only on notes and rests.
     */
    void quantizeByNote(Track::iterator from,
                        Track::iterator to,
                        int maxDots = 2);

    /**
     * Quantizes one event.  Sets the DurationProperty; does not
     * change the event duration
     */
    void quantizeByUnit(Rosegarden::Event *el, int unit = Note::Shortest);

    /**
     * Quantizes one event.  Sets the DurationProperty,
     * NoteDurationProperty, Note::NoteType and Note::NoteDots
     * properties; does not change the event duration.  If the event
     * is not a note or rest, only sets the first of those properties.
     */
    void quantizeByNote(Rosegarden::Event *el, int maxDots = 2);

protected:

    void quantizeByNote(timeT duration, int dots,
                        timeT &low,  Note &lowNote,
                        timeT &high, Note &highNote);
};

}

#endif
