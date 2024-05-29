/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[BasicQuantizer]"
#define RG_NO_DEBUG_PRINT

#include "BasicQuantizer.h"

#include "base/BaseProperties.h"
#include "misc/Debug.h"

namespace Rosegarden
{


BasicQuantizer::BasicQuantizer(timeT unit, bool doDurations,
                               int swingPercent, int iteratePercent) :
    Quantizer(RawEventData),
    m_unit(unit),
    m_durations(doDurations),
    m_swing(swingPercent),
    m_iterate(iteratePercent)
{
    if (m_unit < 0)
        m_unit = Note(Note::Shortest).getDuration();
}

BasicQuantizer::BasicQuantizer(const std::string& source,
                               const std::string& target,
                               timeT unit, bool doDurations,
                               int swingPercent, int iteratePercent) :
    Quantizer(source, target),
    m_unit(unit),
    m_durations(doDurations),
    m_swing(swingPercent),
    m_iterate(iteratePercent)
{
    if (m_unit < 0)
        m_unit = Note(Note::Shortest).getDuration();
}

void
BasicQuantizer::quantizeSingle(
        Segment *segment, Segment::iterator eventIter) const
{
    Event *event = *eventIter;

    const timeT originalDuration = getFromSource(event, DurationValue);

    // Erase events that are zero duration or smaller than m_removeSmaller.
    if (event->isa(Note::EventType)  &&
        (originalDuration == 0  ||  originalDuration < m_removeSmaller)) {
        segment->erase(eventIter);
        return;
    }

    // No quantization?  Bail.
    if (m_unit == 0)
        return;

    const timeT originalTime = getFromSource(event, AbsoluteTimeValue);

    timeT newTime(originalTime);

    const timeT barStart = segment->getBarStartForTime(newTime);

    // Adjust newTime to be relative to the bar.
    newTime -= barStart;

    // Compute the quantization grid cell number for this note.
    int cellNumber = newTime / m_unit;

    // Compute cell start and end times.
    const timeT startTime = cellNumber * m_unit;
    const timeT endTime = startTime + m_unit;

    // If the note is closer to the start of the cell.
    // Same as (newTime < startTime + m_unit/2).
    if (endTime - newTime > newTime - startTime) {
        newTime = startTime;
    } else {  // The note is closer to the end of the cell
        newTime = endTime;
        // Move to the next cell so we'll get the swing right.
        ++cellNumber;
    }

    // "/ 300" is "* (1/3) * (1/100)"
    // 1/3 is full swing offset
    // 1/100 since m_swing is percent.
    const timeT swingOffset = (m_unit * m_swing) / 300;

    // Swing every other cell.
    if (cellNumber % 2 == 1)
        newTime += swingOffset;

    timeT newDuration(originalDuration);

    // If we are quantizing durations
    if (m_durations  &&  newDuration != 0) {

        const timeT durationLow = (newDuration / m_unit) * m_unit;
        const timeT durationHigh = durationLow + m_unit;

        if (durationLow > 0  &&
            durationHigh - newDuration > newDuration - durationLow) {
            newDuration = durationLow;
        } else {
            newDuration = durationHigh;
        }

        const int endCellNumber = cellNumber + newDuration / m_unit;

        if (cellNumber % 2 == 0) { // start not swung
            if (endCellNumber % 2 == 0) { // end not swung
                // do nothing
            } else { // end swung
                newDuration += swingOffset;
            }
        } else { // start swung
            if (endCellNumber % 2 == 0) { // end not swung
                newDuration -= swingOffset;
            } else {
                // do nothing
            }
        }
    }

    // Adjust newTime to be relative to the Composition.
    newTime += barStart;

    // If we are doing something other than full quantization...
    if (m_iterate != 100) {
        // Keep track of the fully quantized time/duration.
        const timeT fullQTime(newTime);
        const timeT fullQDuration(newDuration);

        // Apply the partial (iterative) quantization.
        newTime = (newTime - originalTime) * m_iterate / 100 + originalTime;
        newDuration = (newDuration - originalDuration) * m_iterate / 100 +
                originalDuration;

        // if an iterative quantize results in something much closer than
        // the shortest actual note resolution we have, just snap it
        const timeT close = Note(Note::Shortest).getDuration() / 2;
        if (newTime >= fullQTime - close  &&  newTime <= fullQTime + close)
            newTime = fullQTime;
        if (newDuration >= fullQDuration - close  &&
            newDuration <= fullQDuration + close)
            newDuration = fullQDuration;
    }

    // Important: We have to do this prior to the call to setToTarget().
    //            After setToTarget(), event is invalid.
    if (m_removeArticulations) {
        Marks::removeMark(*event, Marks::Tenuto);
        Marks::removeMark(*event, Marks::Staccato);
    }

    // If there was a change, adjust the event.
    if (originalTime != newTime  ||  originalDuration != newDuration)
    {
        setToTarget(segment, eventIter, newTime, newDuration);
        // The Event object and eventIter are rendered invalid by setToTarget().
        // Make sure they don't get used inadvertently.
        event = nullptr;
        eventIter = segment->end();
    }
}


}
