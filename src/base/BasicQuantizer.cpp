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

#include "BasicQuantizer.h"

#include "base/BaseProperties.h"

namespace Rosegarden
{


BasicQuantizer::BasicQuantizer(timeT unit, bool doDurations,
                               int swing, int iterate) :
    Quantizer(RawEventData),
    m_unit(unit),
    m_durations(doDurations),
    m_swing(swing),
    m_iterate(iterate)
{
    if (m_unit < 0)
        m_unit = Note(Note::Shortest).getDuration();
}

BasicQuantizer::BasicQuantizer(std::string source, std::string target,
                               timeT unit, bool doDurations,
                               int swing, int iterate) :
    Quantizer(source, target),
    m_unit(unit),
    m_durations(doDurations),
    m_swing(swing),
    m_iterate(iterate)
{
    if (m_unit < 0)
        m_unit = Note(Note::Shortest).getDuration();
}

// ??? Move this to Event.
static void
removeMark(Event *event, const Mark &markToRemove)
{
    // Marks are stored as properties named "mark1", "mark2", etc....
    // This makes removing one a bit convoluted.  We'll move them to
    // a std::set to make it easy to erase() one.  Then put them back
    // into the Event.
    //
    // This begs the question...  Why weren't marks implemented as
    // individual properties?  E.g.:
    //
    //   // Add staccato
    //   event->set<Int>(BaseProperties::STACCATO, 1);
    //   // Remove staccato
    //   event->unset<Int>(BaseProperties::STACCATO);
    //
    // That would be so much easier.

    long markCount{};
    event->get<Int>(BaseProperties::MARK_COUNT, markCount);

    if (markCount == 0)
        return;

    // Read all the marks into a std::set so we can work with them.

    std::set<Mark> markSet;

    // For each mark, insert it into markSet.
    for (long i = 0; i < markCount; ++i) {
        Mark currentMark;
        event->get<String>(BaseProperties::getMarkPropertyName(i), currentMark);
        markSet.insert(currentMark);
    }

    // Erase.  If not found, bail.
    if (!markSet.erase(markToRemove))
        return;

    // Write all the marks back to the Event.

    long newMarkCount = markSet.size();

    // unset() the extra marks that will not be needed.
    for (long i = newMarkCount; i < markCount; ++i) {
        event->unset(BaseProperties::getMarkPropertyName(i));
    }

    // Write out the marks to the Event.
    long index = 0;
    for (const Mark &currentMark : markSet) {
        event->set<String>(
                BaseProperties::getMarkPropertyName(index++), currentMark);
    }

    // Update the MARK_COUNT
    if (newMarkCount == 0)
        event->unset(BaseProperties::MARK_COUNT);
    else
        event->set<Int>(BaseProperties::MARK_COUNT, newMarkCount);
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
    //            After setToTarget(), event points to freed memory.
    if (m_removeArticulations) {
        // ??? The removeMark() process is pretty convoluted.  It should
        //     take a list of marks to remove.
        removeMark(event, Marks::Tenuto);
        removeMark(event, Marks::Staccato);
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

std::vector<timeT>
BasicQuantizer::getStandardQuantizations()
{
    static std::vector<timeT> standardQuantizations;

    // If cache is empty, fill it.
    if (standardQuantizations.empty())
    {
        // For each note type from semibreve to hemidemisemiquaver
        for (Note::Type nt = Note::Semibreve; nt >= Note::Shortest; --nt) {

            // For quavers and smaller, offer the triplet variation
            const int variations = (nt <= Note::Quaver ? 1 : 0);

            // For the base note (0) and the triplet variation (1)
            for (int i = 0; i <= variations; ++i) {

                // Compute divisor, e.g. crotchet is 4, quaver is 8...
                int divisor = (1 << (Note::Semibreve - nt));

                // If we're doing the triplet variation, adjust the divisor
                if (i)
                    divisor = divisor * 3 / 2;

                // Compute the number of MIDI clocks.
                const timeT unit = Note(Note::Semibreve).getDuration() / divisor;

                standardQuantizations.push_back(unit);
            }
        }
    }

    return standardQuantizations;
}


}
