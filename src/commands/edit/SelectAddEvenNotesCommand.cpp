/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_NO_DEBUG_PRINT
#define RG_MODULE_STRING "[SelectAddEvenNotesCommand]"

#include "SelectAddEvenNotesCommand.h"

#include "base/Event.h"
#include "base/NotationTypes.h"
#include "base/Segment.h"
#include "base/Selection.h"
#include "misc/Debug.h"

#include <limits>
#include <cmath>

namespace Rosegarden
{

SelectAddEvenNotesCommand::BeatInterpolator::
BeatInterpolator(timeT duration,
                 timeT prevBeatDuration,
                 int numBeats)
    : m_baseBeatDuration(prevBeatDuration),
      m_logScalingPerBeat(calculateLogScalingPerBeat(duration,
                                                     prevBeatDuration,
                                                     numBeats))
{}

// Given a time interval of N beats, calculate a scaling factor that
// distributes beats evenly.
// @param duration Absolute time duration of the interval
// @param prevBeatDuration Duration of the beat before the interval,
// so that we can match it smoothly.
// @param numBeats Number of beats the interval comprises.
// @return The log of the ratio by which each successive beat expands
// by in time.  Can return a negative number if each successive beat
// contracts in time.
// @author Tom Breton (Tehom)
float
SelectAddEvenNotesCommand::BeatInterpolator::
calculateLogScalingPerBeat(timeT duration,
                           timeT prevBeatDuration,
                           int numBeats)
{
    /**
     * For example, say the interval comprises 4 beats (ie we skipped
     * 3 beats), the last known beat duration is 1000 timeT units, and
     * the interval's duration since the known beat note is 4100
     * units.  Since the expected duration is 4000 units, we have a
     * slight ritardando.
     *
     * We want to fill the interval with smoothly interpolated beats.
     * It won't do to interpolate linearly, eg in the example to
     * simply switch to a beat duration of 1025 units.  That would
     * make the first tempo too slow and the last tempo too fast.
     * That wouldn't be a ritardando, it would be a sudden tempo
     * change.
     *
     * So we need a smoothly varying tempo that starts at 1000 units
     * per beat, and after 4 beats has taken 4100 units.  We can't
     * really change the tempo with perfect smoothness - we actually
     * just change it every beat - but we can compute the beat times
     * with this and find the tempos from that.
     *
     * Suppose we know:
     *
     *  * The original tempo, in time units per beat.
     *
     *  * How fast tempo changes, in time units per beat squared.
     *    We'll represent it as the log of the ratio of successive
     *    tempi so we can write "scaling" instead of "ln(scaling)"
     * 
     * Then the total time taken across numBeats is:
     *
     * integral(beat * firstTempo * e^(scaling * beat) d beat)
     * from 0 to numBeats,
     *
     * giving:
     *
     * numBeats * firstTempo * e^(scaling * numBeats) 
     *
     * We need to find scaling per beat, which is:
     *
     * ln (duration / (numBeats * firstTempo)) / numBeats
     *
     * With this we can calculate any beat's time.
     **/

    // The time interval we would have predicted it would take.
    const timeT expectedDuration =
        prevBeatDuration * numBeats;

    // The ratio of the interval to its predicted time (can be less
    // than 1)
    const float ratioOfTotalToInterval =
        float (duration) / float(expectedDuration);

    // The log of the ratio which each successive beat expands by in
    // time (can be less than 0)
    return std::log(ratioOfTotalToInterval) / numBeats;
}

// @return The absolute time corresponding to beat "beatNumber"
// @author Tom Breton (Tehom)
timeT
SelectAddEvenNotesCommand::BeatInterpolator::
getBeatRelativeTime(int beatNumber)
{
    return 
        beatNumber *
        m_baseBeatDuration *
        std::exp(m_logScalingPerBeat * beatNumber);
}

// @param duration Absolute time duration of the interval
// @param prevBeatDuration Duration of the beat before the interval,
// so that we can match it smoothly.
// @param numBeats Number of beats the interval comprises.
// @return The duration of the last beat of the interval.
// @author Tom Breton (Tehom)
timeT
SelectAddEvenNotesCommand::BeatInterpolator::
getLastBeatRelativeTime(timeT duration,
                        timeT prevBeatDuration,
                        int numBeats)
{
    return
        BeatInterpolator(duration,
                         prevBeatDuration,
                         numBeats).
        getBeatRelativeTime(numBeats - 1);
}

// Find evenly-spaced notes
// @param firstBeat A note Event performed on the first beat.
// @param secondBeat A note Event performed on the second beat.
// @param s The segment to look in for further events.
// @return Beat-defining data as a BeatEventVector
// @author Tom Breton (Tehom)
SelectAddEvenNotesCommand::BeatEventVector
SelectAddEvenNotesCommand::findBeatEvents(Segment &s,
                                   Event *firstBeat,
                                   Event *secondBeat)
{
    // Fixed parameters
    const float marginTimeRatio = 0.77;
    const float minTimeRatio = marginTimeRatio;
    const float maxTimeRatio = 1 / minTimeRatio;

    // Storage for the results.  We add the first two beats
    // immediately.  Caller promises us that they make sense.
    BeatEventVector result;
    result.push_back(BeatEvent(firstBeat, 0));
    result.push_back(BeatEvent(secondBeat, 0));

    // State variables tracking the most recent beat we've found.
    timeT currentBeatTime = secondBeat->getAbsoluteTime();
    timeT prevKnownBeatDuration =
        currentBeatTime - firstBeat->getAbsoluteTime();
    if (prevKnownBeatDuration <= 0) { return result; }

    // State variables tracking the current noteless stretch.  It may
    // be empty.
    int numSkippedBeats = 0;
    timeT prevKnownBeatTime = currentBeatTime;

    /**
     * We assume the beat-defining notes are separated by roughly the
     * same time-interval.  We handle noteless stretches where we
     * expect beats by counting the number of beats we missed.  We
     * drop the noteless stretch after the last true beat we find
     * because it can't do anything useful.  Stretches that contain
     * just notes that are too far from rhythmic expectations are
     * treated as noteless stretches.
     **/
    
    // Find beat-defining notes in the segment
    while (true) {

        // The time we would expect a new beat if the rhythm and tempo
        // were exactly constant.
        timeT expectedNoteTime = currentBeatTime + prevKnownBeatDuration;

        // An acceptable interval for the next beat.
        timeT minNextDuration = float(prevKnownBeatDuration) * minTimeRatio;
        timeT maxNextDuration = float(prevKnownBeatDuration) * maxTimeRatio;
        timeT minNextNoteTime = currentBeatTime + minNextDuration;
        timeT maxNextNoteTime = currentBeatTime + maxNextDuration;

        Segment::const_iterator startRangeIter = s.findTime(minNextNoteTime);
        Segment::const_iterator endRangeIter = s.findTime(maxNextNoteTime);

       // Break if there won't be more notes to find.
       if (startRangeIter == s.end()) { break; }

       // Candidate variable.  NULL means nothing found.
       Event *nextBeat = 0;
       // Scoring variable, how much the best candidate note differs
       // from expectedNoteTime.  Smaller is better.
       timeT nearestMiss = std::numeric_limits<timeT>::max();
       
       for (Segment::const_iterator i = startRangeIter;
            i != endRangeIter;
            ++i) {
           Event *e = *i;
           // Only consider notes.
           if (e->isa(Note::EventType)) {
               const timeT missedBy =
                   std::abs(e->getAbsoluteTime() - expectedNoteTime);

               // Track the best candidate.
               if (missedBy < nearestMiss) {
                   nextBeat = e;
                   nearestMiss = missedBy;
               }
           }
       }

       
       if (nextBeat) {
           const timeT nextBeatTime = nextBeat->getAbsoluteTime();
           const timeT stretchDuration = nextBeatTime - prevKnownBeatTime;
           const int numIncludedBeats = numSkippedBeats + 1;
           // The absolute time of the beat immediately before
           // nextBeatTime, possibly calculated by BeatInterpolator
           timeT prevFoundBeatTime =
               (numSkippedBeats > 0) ?
               (prevKnownBeatTime +
                BeatInterpolator::
                getLastBeatRelativeTime(stretchDuration,
                                        prevKnownBeatDuration,
                                        numIncludedBeats)) :
               currentBeatTime;

           // Add this beat
           result.push_back(BeatEvent(nextBeat,
                                      numSkippedBeats,
                                      BeatInterpolator(stretchDuration,
                                                       prevKnownBeatDuration,
                                                       numIncludedBeats)));

           // Since we'll continue from a known beat, record that we
           // haven't skipped any beats.
           numSkippedBeats = 0;

           // Set variables for the next iteration of the loop.
           prevKnownBeatDuration = nextBeatTime - prevFoundBeatTime;
           currentBeatTime = nextBeatTime;
           prevKnownBeatTime = nextBeatTime;
       } else {
           // If we found no candidates, we began or are already in a
           // noteless stretch.  In either case we count one more
           // missed beat and step forward in time.
           ++numSkippedBeats;
           currentBeatTime = expectedNoteTime;
       }
    }

    return result;
}

// Find evenly-spaced notes
// @param eventSelection A selection whose first two events should be
// the first two beats.
// @return Beat-defining data as a BeatEventVector.  On failure,
// it is empty.
// @author Tom Breton (Tehom)
SelectAddEvenNotesCommand::BeatEventVector
SelectAddEvenNotesCommand::
findBeatEvents(EventSelection *eventSelection)
{
    typedef EventContainer::iterator iterator;
    EventContainer &segmentEvents = eventSelection->getSegmentEvents();

    /**
     * Get the first two note Events in selection.  If they don't
     * exist, just bail out.
     */
    iterator i1 =
        segmentEvents.findEventOfType(segmentEvents.begin(), Note::EventType);
    if (i1 == segmentEvents.end()) { return BeatEventVector(); }
    Event *e1 = *i1;
    ++i1;

    iterator i2 =
        segmentEvents.findEventOfType(i1, Note::EventType);
    if (i2 == segmentEvents.end()) { return BeatEventVector(); }
    Event *e2 = *i2;

    // Now just use the worker function.
    return findBeatEvents(eventSelection->getSegment(), e1, e2);
}

// @ctor SelectAddEvenNotesCommand
// @author Tom Breton (Tehom)
SelectAddEvenNotesCommand::
SelectAddEvenNotesCommand(BeatEventVector beatEventVector, Segment *segment) :
    BasicCommand(getGlobalName(),
                 *segment,
                 getStartTime(beatEventVector),
                 getEndTime(beatEventVector), true),
    m_beatEventVector(beatEventVector)
{
}

// @return The absolute time of the first event.
// @author Tom Breton (Tehom)
timeT
SelectAddEvenNotesCommand::
getStartTime(BeatEventVector &beatEventVector)
{
    return beatEventVector.front().m_event->getAbsoluteTime();
}

// @return The absolute time of the last event.
// @author Tom Breton (Tehom)
timeT
SelectAddEvenNotesCommand::
getEndTime(BeatEventVector &beatEventVector)
{
    return beatEventVector.back().m_event->getAbsoluteTime();
}

// Worker function of SelectAddEvenNotesCommand
// Add any extra note events needed to give evenly spaced beats.
// @author Tom Breton (Tehom)
void
SelectAddEvenNotesCommand::modifySegment()
{
    Segment &s = getSegment();
    BeatEventVector &beatEventVector = m_beatEventVector;
    // The time of the previous beat, which we have to carry to the
    // next interpolator.  Initial value is meaningless, since the
    // first beat can have no preceding interpolated beats.
    timeT prevBeatTime = 0;
    for (BeatEventVector::iterator i = beatEventVector.begin();
         i != beatEventVector.end();
         ++i) {
        BeatEvent &beatEvent = *i;
        Event *modelEvent = beatEvent.m_event;
        BeatInterpolator &beatInterpolator =
            beatEvent.m_beatInterpolator;
        for (int j = 0; j < beatEvent.m_numSkippedBeats; ++j) {
            timeT newTime =
                prevBeatTime + beatInterpolator.getBeatRelativeTime(j + 1);
            Event *e = new Event(*modelEvent, newTime);
            s.insert(e);
            m_eventsAdded.push_back(e);
        }
        prevBeatTime = modelEvent->getAbsoluteTime();
    }
}

// Select exactly the beat-defining events, including any extra notes
// this command inserted.
// @author Tom Breton (Tehom)
EventSelection *
SelectAddEvenNotesCommand::getSubsequentSelection()
{
    RG_DEBUG << "SelectAddEvenNotesCommand::getSubsequentSelection" << endl;
    EventSelection *selection = new EventSelection(getSegment());

    RG_DEBUG << (int)m_beatEventVector.size()
             << "elements in m_beatEventVector"
             << endl;
    // Add the beat events we found to the selection
    for (BeatEventVector::iterator i = m_beatEventVector.begin();
         i != m_beatEventVector.end();
         ++i) {
        BeatEvent &beatEvent = *i;
        // Skip ties
        selection->addEvent(beatEvent.m_event, false);
    }

    // Also add any events that we made.
    RG_DEBUG << (int)m_eventsAdded.size()
             << "elements in redoEvents"
             << endl;
    for (EventVector::const_iterator i = m_eventsAdded.begin();
         i != m_eventsAdded.end();
         ++i) {
        Event *e = *i;
        // Skip ties
        selection->addEvent(e, false);
    }
    return selection;
}


}
