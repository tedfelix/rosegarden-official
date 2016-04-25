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

#define RG_NO_DEBUG_PRINT 1
#define RG_MODULE_STRING "[FitToBeatsCommand]"

#include "FitToBeatsCommand.h"
#include "misc/Debug.h"
#include "base/Event.h"
#include "base/CompositionTimeSliceAdapter.h"
#include "base/NotationTypes.h"

namespace Rosegarden
{

// Construct FitToBeatsCommand
// @author Tom Breton (Tehom)
FitToBeatsCommand::FitToBeatsCommand(Segment *grooveSegment)
    :
  NamedCommand(getGlobalName()),
  m_composition(grooveSegment->getComposition()),
  m_executed(false)
{
    initialise(grooveSegment);
}

// Destruct FitToBeatsCommand
// @author Tom Breton (Tehom)
FitToBeatsCommand::~FitToBeatsCommand()
{}

// Change to newTempi, removing all old tempi
// @author Tom Breton (Tehom)
void
FitToBeatsCommand::changeAllTempi(TempoMap newTempi)
{
    const unsigned int Nb = m_composition->getTempoChangeCount();
    for (unsigned int i = 0; i < Nb; i++)
        { m_composition->removeTempoChange(0); }

    for (TempoMap::iterator i = newTempi.begin(); i != newTempi.end(); ++i) {
        m_composition->addTempoAtTime(i->first, i->second);
    }
}

// @remarks Change from using oldSegments to newSegments
// @author Tom Breton (Tehom)
void
FitToBeatsCommand::changeSegments(SegmentMultiSet oldSegments,
                                  SegmentMultiSet newSegments)
{
    for (SegmentMultiSet::iterator i = oldSegments.begin();
         i != oldSegments.end();
         ++i) {
        m_composition->detachSegment(*i);
    }

    for (SegmentMultiSet::iterator i = newSegments.begin();
         i != newSegments.end();
         ++i) {
        m_composition->addSegment(*i);
    }
}

// Initialize FitToBeatsCommand
// @author Tom Breton (Tehom)
void
FitToBeatsCommand::initialise(Segment *s)
{
    m_oldTempi.clear();
    m_newTempi.clear();
    m_oldSegments.clear();
    m_newSegments.clear();

    // Get the real times from the beat segment
    vecRealTime beatRealTimes;
    int success = 
        getBeatRealTimes(s, beatRealTimes);
    if(!success) { return; }

    // Store the current tempos
    getCurrentTempi(*m_composition, m_oldTempi);
    tempoT defaultTempo = m_composition->getCompositionDefaultTempo();

    // A temporary copy of the composition.  It is not intended to be
    // a complete copy, it just provides a place for new segments and
    // tempi to live until we have fully copied events to their new
    // state.
    Composition scratchComposition;
    scratchComposition.clear();
    scratchComposition.setCompositionDefaultTempo(defaultTempo);

    
    // Set tempos in scratchComposition such that each observed beat
    // in beatRealTimes takes one beatTime.
    {
        // Starting time is the same for both.
        timeT firstBeatTime = 
            m_composition->getElapsedTimeForRealTime(beatRealTimes[0]);

        unsigned int numBeats = beatRealTimes.size();

        // Get interval between beats from time signature.
        // Get time signature
        TimeSignature timeSig =
            m_composition->getTimeSignatureAt(firstBeatTime);
        timeT beatTime = timeSig.getBeatDuration();

        // We're going to visit the beats in reverse order, and always
        // remembering the next beat (the next beat time-wise, which
        // the iterator visited last time)
        vecRealTime::const_reverse_iterator i = beatRealTimes.rbegin();

        // Treat the final beat specially
        timeT    finalBeatTime = firstBeatTime + ((numBeats - 1) * beatTime);
        RealTime finalRealTime = beatRealTimes.back();
        scratchComposition.addTempoAtTime(finalBeatTime, defaultTempo, -1);
        // Step past it
        ++i;

        // Set up loop variables
        timeT    nextBeatTime = finalBeatTime;
        RealTime nextRealTime = finalRealTime;
        // nextTempo is unused, it will be used if we make ramped
        // tempi.
        /* tempoT   nextTempo    = defaultTempo; */


        // Treat all the other beats.
        while (i != beatRealTimes.rend()) {
            timeT        timeNow = nextBeatTime - beatTime;
            RealTime realTimeNow = *i;
            RealTime realTimeDelta = nextRealTime - realTimeNow;
            // Calculate what tempoT will get us to the right real
            // time.  For now, we use unramped tempi.
            tempoT rampTo = -1;
            tempoT tempo = Composition::timeRatioToTempo(realTimeDelta,
                                                         beatTime, rampTo);
            scratchComposition.addTempoAtTime(timeNow, tempo, rampTo);

            // Step
            nextBeatTime = timeNow;
            nextRealTime = realTimeNow;
            /* nextTempo    = tempo; */
            ++i;
        }
    }
    // We don't try to copy over tempo changes that are outside the
    // range of the groove segment (before or after).  We don't try to
    // correct for accumulated error.

    // Done setting Tempi

    // Collect tempi
    getCurrentTempi(scratchComposition, m_newTempi);


    // Copy all the events to scratchComposition.  The copies will be
    // at the same realtime but not the same timeT.  Even events in
    // the groove segment get copied.
    SegmentMultiSet &origSegments = m_composition->getSegments();
    for (Composition::iterator i = origSegments.begin();
         i != origSegments.end();
         ++i) {
        Segment * const oldSegment = *i;

        // We'd prefer to just make a segment with no events that's
        // otherwise the same as the old one but we can't.
        Segment *newSegment = oldSegment->clone(false);
        newSegment->clear();

        // Add the segments into appropriate containers.
        // scratchComposition owns the new segments during initialise,
        // but m_newSegments will own them after initialise returns.
        m_oldSegments.insert(oldSegment);
        m_newSegments.insert(newSegment);
        scratchComposition.addSegment(newSegment);

        //Iterate over notes in the old segment.
        const timeT earliestTime = 0;
        for (Segment::iterator j = oldSegment->findTime(earliestTime);
             oldSegment->isBeforeEndMarker(j);
             ++j)  {
            // Get the old-timed event times.
            timeT oldStartTime = (*j)->getAbsoluteTime();
            timeT duration = (*j)->getDuration();

            // Get the real event times.
            RealTime RealStartTime =
                m_composition->getElapsedRealTime(oldStartTime);

            RealTime RealEndTime;
            if (duration == 0) {
                RealEndTime = RealStartTime;
            }
            else {
                timeT oldEndTime = oldStartTime + duration;
                RealEndTime = 
                    m_composition->getElapsedRealTime(oldEndTime);
            }

            // Get the new target times.  Use scratchComposition
            // because its times use the new Tempi.
            timeT newStartTime =
                scratchComposition.getElapsedTimeForRealTime(RealStartTime);
            timeT newDuration;
            if (duration == 0) {
                newDuration = 0;
            }
            else {
                timeT newEndTime =
                    scratchComposition.getElapsedTimeForRealTime(RealEndTime);
                newDuration = newEndTime - newStartTime;
            }

            // Add a parallel event in the new segment.
            newSegment->insert(new Event(**j, newStartTime, newDuration));
        }
    }

    // Detach the segments before scratchComposition goes out of
    // scope.  m_newSegments contains exactly the segments that need
    // to be detached.
    for (SegmentMultiSet::iterator i = m_newSegments.begin();
         i != m_newSegments.end();
         ++i) {
        scratchComposition.weakDetachSegment(*i);
    }

    // We do the actual swapping of old <-> new in (un)execute.
}

// @param A beat segment
// @param Reference to a vector of RealTime, to be filled.
// @returns non-zero on success
// @author Tom Breton (Tehom)
int
FitToBeatsCommand::getBeatRealTimes(Segment *s,
                                    vecRealTime &beatRealTimes)
{
    for (Segment::iterator i = s->begin(); s->isBeforeEndMarker(i); ++i) {
        if ((*i)->isa(Note::EventType)) {
            RealTime Time =
                s->getComposition()->getElapsedRealTime
                ((*i)->getAbsoluteTime());
            beatRealTimes.push_back(Time);
        }
    }

    return (beatRealTimes.size() > 1);
}

// Clone a given tempo by index.
// @remarks Currently discards ramping because TempoT doesn't hold it.
// @author Tom Breton (Tehom)
FitToBeatsCommand::TempoChange
FitToBeatsCommand::getTempoChange(Composition &composition, int i)
{
    return composition.getTempoChange(i);
}

// Get the current tempos of the composition
// @param A composition
// @param Reference to a TempoMap, to be filled.
// @author Tom Breton (Tehom)
void
FitToBeatsCommand::getCurrentTempi(Composition &composition, TempoMap &Tempos)
{
    const int numTempoChanges = composition.getTempoChangeCount();
    for (int i = 0; i < numTempoChanges; ++i) {
        TempoChange tempoChange =
            getTempoChange(composition, i); 
        Tempos[tempoChange.first] = tempoChange.second;
    }
}

// Execute the command
// @author Tom Breton (Tehom)
void
FitToBeatsCommand::execute()
{
    RG_DEBUG << "Executing FitToBeatsCommand";
    changeAllTempi(m_newTempi);
    changeSegments(m_oldSegments, m_newSegments);
    m_executed = true;
}

// Unexecute the command
// @author Tom Breton (Tehom)
void
FitToBeatsCommand::unexecute()
{
    changeAllTempi(m_oldTempi);
    changeSegments(m_newSegments, m_oldSegments);
    m_executed = false;
}

} //End namespace Rosegarden
