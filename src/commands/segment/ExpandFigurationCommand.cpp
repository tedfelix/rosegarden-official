/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2011 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "ExpandFigurationCommand.h"
#include "misc/Debug.h"
#include "base/BaseProperties.h"
#include "base/Event.h"
#include "base/Exception.h"
#include "base/NotationQuantizer.h"
#include "base/NotationRules.h"
#include "base/NotationTypes.h"
#include "base/Property.h"
#include "base/Selection.h"
#include <algorithm>

namespace Rosegarden
{

/***** Internal types *****/

/// @typedef The types we retrieve from particular properties
/// @author Tom Breton (Tehom)
typedef PropertyDefn<Int>::basic_type pitchT;
typedef PropertyDefn<Int>::basic_type velocityT;


;
/// Class to describe a relative event
/// @class RelativeEvent
/// @author Tom Breton (Tehom)
class RelativeEvent
{
public:
    RelativeEvent(int i, Event *e, timeT startTime)
        : m_score(-1000000),
          m_index(i),
          m_bareEvent(e),
          m_relativeTime(e->getAbsoluteTime() - startTime)
    {};

    virtual Event   *getAsEvent(timeT baseTime, const Key key,
                                eventVector notes)=0;
    int getScore(void) { return m_score; };
    timeT            getRelativeTime() { return m_relativeTime; };
protected:
    int              m_score;
    int              m_index;
    Event            *m_bareEvent;
    timeT            m_relativeTime;
};

/// Class to describe a relative note
/// @class RelativeNote
/// @author Tom Breton (Tehom)
class RelativeNote : public RelativeEvent
{
public:
    RelativeNote(int i, Event *e, timeT startTime) :
        RelativeEvent(i, e, startTime)
    {};
    virtual Event   *getAsEvent(timeT baseTime, const Key key,
                                eventVector notes);
    virtual pitchT getResultPitch(const Key key, const Pitch & basePitch)=0;
};

/// Class to describe a chromatically relative note
/// @class RelativeNote
/// @author Tom Breton (Tehom)
class ChromaticRelativeNote : public RelativeNote
{
public:
    ChromaticRelativeNote(int i, Event *e, timeT startTime,
                          const Pitch &basePitch) :
        RelativeNote(i, e, startTime),
        m_interval(e->get<Int>(BaseProperties::PITCH) -
                   basePitch.getPerformancePitch()) {

        // The larger the interval, the worse the score.
        const int absInterval = abs(m_interval);
        const int penalty = (absInterval % 12) + (absInterval / 12);
        m_score = - penalty;
    };
    static pitchT addChromaticInterval(const Pitch & basePitch, int interval);
    virtual pitchT getResultPitch(const Key key, const Pitch & basePitch);
private:
    int  m_interval;
};

/// Class to describe a diatonically relative note
/// @class RelativeNote
/// @author Tom Breton (Tehom)
class DiatonicRelativeNote : public RelativeNote
{
public:
    DiatonicRelativeNote(int i, Event *e, timeT startTime,
                         const Key key, const Pitch basePitch);
    static pitchT addDiatonicInterval(const Key key,
                                      const Pitch & basePitch,
                                      int interval);
    virtual pitchT getResultPitch(const Key key, const Pitch & basePitch);
private:
    int  m_interval;
};

/// @typedef Containers for RelativeEvent
/// @author Tom Breton (Tehom)
typedef std::vector<RelativeEvent *> RelativeEventVec;

/// @class A chord together with its start and end times.
/// @author Tom Breton (Tehom)
class TimedEventVector
{
public:
    eventVector chord;
    timeT       start;
    timeT       end;
};
/// @typedef Several TimedEventVectors in the same bar (or other group)
/// @author Tom Breton (Tehom)
typedef std::vector<TimedEventVector> TimeSegmentedEventVector;

/***** End of internal types *****/

/***** Helper functions *****/
    
/// True if Event a has higher pitch than Event b.  
/// @param a and b both must be notes.
/// @author Tom Breton (Tehom)
int higherPitch(Event *a, Event* b)
{
    if (!a->has(BaseProperties::PITCH) ||
        !a->has(BaseProperties::PITCH)) {
        throw Exception("Shouldn't get a note that has no pitch.");
    }

    return
        a->get<Int>(BaseProperties::PITCH) >
        b->get<Int>(BaseProperties::PITCH);
}

/***** Methods for DiatonicRelativeNote *****/    

/// Make a DiatonicRelativeNote for Event e
/// @param i indexes some tone in the parameter chord.
/// @param basePitch is the pitch of that tone
/// @param startTime is the time the figuration started
/// @param key is the current key
/// @author Tom Breton (Tehom)
DiatonicRelativeNote::DiatonicRelativeNote(int i,
                                           Event *e,
                                           timeT startTime,
                                           const Key key,
                                           const Pitch basePitch) :
    RelativeNote(i, e, startTime)
{
    const int octaveBase = 0;
    const Pitch thisPitch(*e);
    int relativeNoteInScale =
        thisPitch.getNoteInScale(key) -
        basePitch.getNoteInScale(key);
    int relativeOctave =
        thisPitch.getOctave(octaveBase) -
        basePitch.getOctave(octaveBase);

    relativeNoteInScale += 7 * 10;
    relativeOctave += (relativeNoteInScale / 7) - 10;
    relativeNoteInScale %= 7;
    
    m_interval = relativeNoteInScale + (7 * relativeOctave);
    // Big penalty for accidentals in key, because diatonics aren't
    // meant to handle them.
    const int accidentalPenalty =
        (thisPitch.getDisplayAccidental(key) ==
         Accidentals::NoAccidental) ? 0 : 1000;

    const int penalty =
        abs(relativeNoteInScale) +
        abs(relativeOctave) +
        accidentalPenalty;
    m_score =  -penalty;
};

/// Return the pitch of basePitch raised by a diatonic interval.
/// @param key is the key to govern diatonicity
/// @param basePitch is a Pitch
/// @param interval can be positive or negative
/// @author Tom Breton (Tehom)
pitchT
DiatonicRelativeNote::addDiatonicInterval(const Key key,
                                          const Pitch & basePitch,
                                          int interval)
{
    // This code does a lot of calculation that almost belongs to
    // Pitch, but can't easily be done thru the Pitch interface.
    const pitchT pitch = basePitch.getPerformancePitch();
    const int pitchInC = Key::DefaultKey.transposeFrom(pitch, key);
    const int noteInOctave = pitchInC % 12;
    const int oldOctave = pitchInC / 12;
    const int oldStepInScale =
        (key.isMinor()) ?
        steps_Cminor_harmonic[noteInOctave] :
        steps_Cmajor[noteInOctave];
    
    // Add the interval
    int newStepInScale = oldStepInScale + interval;

    // Ensure newStepInScale is within (0,6), remembering any
    // adjustment for octave.
    // Make sure it's positive so that operations / and % round
    // consistently.
    newStepInScale += 7 * 10;
    const int octavesGained = ((newStepInScale / 7) - 10);
    newStepInScale %= 7;

    const int newNoteInScale = 
        (key.isMinor()) ?
        scale_Cminor_harmonic[newStepInScale] :
        scale_Cmajor[newStepInScale];
    const int newOctave = oldOctave + octavesGained;

    const int newPitchInC = newNoteInScale + (newOctave * 12);
    return key.transposeFrom(newPitchInC, Key::DefaultKey);
}

/// Return pitch relative to basePitch
/// @param key is the key to govern diatonicity
/// @param e must be a note.
/// @author Tom Breton (Tehom)
pitchT
DiatonicRelativeNote::getResultPitch(const Key key, const Pitch & basePitch)
{
    return addDiatonicInterval(key, basePitch, m_interval);
}
/***** Methods for ChromaticRelativeNote *****/

/// Return the pitch of e, raised by a chromatic interval.
/// @param e must be a note.
/// @param interval can be positive or negative
/// @author Tom Breton (Tehom)
pitchT
ChromaticRelativeNote::addChromaticInterval(const Pitch & basePitch,
                                            int interval)
{
    return basePitch.getPerformancePitch() + interval;
}

/// Return the pitch relative to e
/// @param key is ignored
/// @param e must be a note.
/// @author Tom Breton (Tehom)
pitchT
ChromaticRelativeNote::getResultPitch(const Key, const Pitch & basePitch)
{
    return addChromaticInterval(basePitch, m_interval);
}

/***** Methods for RelativeNote *****/    

/// Return a new note event relative to baseNote.
/// @param baseTime is the baseline time.  
/// @param baseNote must be a note.  Its internal time is ignored.
/// @param key is the key to govern diatonicity
/// @author Tom Breton (Tehom)
Event *
RelativeNote::getAsEvent(timeT baseTime, const Key key, eventVector notes)
{
    // Figure out time
    timeT newStartTime =
        m_relativeTime + baseTime;

    // Figure out pitch
    Event *baseNote = notes[m_index];
    const Pitch basePitch(*baseNote);
    pitchT pitch =
        getResultPitch(key, basePitch);

    // Figure out velocity
    velocityT velocity = 
        baseNote->get<Int>(BaseProperties::VELOCITY) +
        (m_bareEvent)->get<Int>(BaseProperties::VELOCITY) -
        100;
    // Clamp to legal MIDI velocities
    if (velocity < 0)
        { velocity = 0; }
    if (velocity > 127)
        { velocity = 127; }
    
    // Create a corresponding note
    Event *newNote = new Event(*(m_bareEvent), newStartTime);
    newNote->set<Int>(BaseProperties::PITCH, pitch, true);
    newNote->set<Int>(BaseProperties::VELOCITY, velocity, true);
    return newNote;
}

/***** Ctor/dtor of ExpandFigurationCommand *****/

/// Construct ExpandFigurationCommand
/// @author Tom Breton (Tehom)
ExpandFigurationCommand::ExpandFigurationCommand(SegmentSelection selection)
  :
  NamedCommand(getGlobalName()),
  m_executed(false)
{
  m_composition = (*selection.begin())->getComposition();
  m_newSegments.clear();
  initialise(selection);
}

/// Destruct ExpandFigurationCommand
/// @author Tom Breton (Tehom)
ExpandFigurationCommand::~ExpandFigurationCommand()
{}

/***** Methods for ExpandFigurationCommand *****/    
    
/// Return a vector of notes that occur at the given time in segment s.
/// @author Tom Breton (Tehom)
eventVector
ExpandFigurationCommand::getBlockChord(Segment *s, timeT time)
{
    eventVector blockChord;
    blockChord.clear();

    // Look at each event that starts at time
    Segment::const_iterator start, end;
    s->getTimeSlice(time, start, end);
    for (Segment::const_iterator i = start; i != end; ++i) {
        // Capture note events to make a chord
        if ((*i)->isa(Note::EventType)) {
            blockChord.push_back(*i);
        }
    }

    // Sort pitches from high to low (Any ordering would work as long
    // as it's consistent)
    sort(blockChord.begin(), blockChord.end(), higherPitch);
    return blockChord;
}

/// Return a legal time no earlier than lookAtTime to expand the
/// figuration.
/// @param s is the segment holding the block chords.
/// @author Tom Breton (Tehom)
timeT
Figuration::getOKFigTime(Composition *composition, timeT lookAtTime)
{
    // !!! If bar is partial, we should move to the next full bar.

    return composition->getBarStartForTime(lookAtTime);
}

/// Return a legal time strictly later than lookAtTime to expand the
/// figuration.
/// @param s is the segment holding the block chords.
/// @param lookAtTime is the time at which we last expanded figuration
/// @author Tom Breton (Tehom)
timeT
Figuration::getNextFigTime(Composition *composition, timeT lookAtTime)
{
    return composition->getBarEndForTime(lookAtTime); 
}


// There may be different ways to realize a note event.  For instance,
// assuming the parameter chord is a C major triad in root position, a
// D natural might be seen as relatively a major second above the
// first note or relatively a major second below the second note.  So
// in the future we will collect the various possibilities, and at the
// end we score them and pick the best.  RelationSeenMap is meant to
// support that, but right now the code to use it isn't written.

/// Return a map from the relations to booleans, all initially false.
/// Intended for when we explore multiple instances of a figuration.
/// @author Tom Breton (Tehom)
ExpandFigurationCommand::RelationSeenMap
ExpandFigurationCommand::getFreshSeenMap(UnsolvedFiguration &figuration)
{
    RelationSeenMap hypotheses;
    // For each event,
    for (UnsolvedFiguration::iterator i = figuration.begin();
         i != figuration.end();
         ++i) {
        const UnsolvedNote &possibleRelations = *i;
        // for each of its possible relations
        for (UnsolvedNote::iterator j =
                 possibleRelations.begin();
             j != possibleRelations.end();
             ++j) {
            // Add to set of hypotheses
            hypotheses.insert(RelationSeen(*j,false));
        }
    }
    return hypotheses;
}

/// Return a set of possible generators for Event e
/// @author Tom Breton (Tehom)
ExpandFigurationCommand::UnsolvedNote
ExpandFigurationCommand::getPossibleRelations(Event *e,
                                              const eventVector parameterChord,
                                              const Key key,
                                              timeT startTime)
{
    UnsolvedNote possibleRelations;
    possibleRelations.clear();

    for (unsigned int i = 0; i < parameterChord.size(); ++i) {
        const Pitch basePitch(*parameterChord[i]);
        for (int j = 0; j <= 1; ++j) {
            RelativeNote * relation;
            switch (j) {
            case 0:
                relation =
                    new DiatonicRelativeNote(i, e, startTime,
                                             key, basePitch);
                break;
            case 1:
                relation =
                    new ChromaticRelativeNote(i, e, startTime,
                                              basePitch);
                break;
            default:
                throw Exception("Got a relation type we don't recognize.");
                break;
            }
            possibleRelations.insert(relation);
        }
    }

    return possibleRelations;
}

/// Return a figuration object if segment s has one.
/// @author Tom Breton (Tehom)
Figuration
ExpandFigurationCommand::getFiguration(Segment *s)
{
    RG_DEBUG << "Looking for figuration in segment " << s->getLabel()
             << endl;

    UnsolvedFiguration  notesToAccountFor;
    eventVector         parameterChord;
    RelativeEventVec    events;
    events.clear();
    for (Segment::iterator i = s->begin();
         i != s->end();
         ++i) {

        if ((*i)->isa(Indication::EventType)) {
            Indication indication(**i);
            const std::string type = indication.getIndicationType();
            if (type == Indication::ParameterChord)
                {
                    // The chord here will parameterize the next
                    // figuration.
                        
                    RG_DEBUG << "Got a parameter chord"
                             << endl;
                    timeT now = (*i)->getAbsoluteTime();
                    parameterChord = getBlockChord(s, now);
                } else if (type == Indication::Figuration) {

                // There is a figuration here.  It's relative to
                // the preceding parameter chord.
                RG_DEBUG << "Got a figuration"
                         << endl;
            
                // If there's no parameter chord, we can't do anything
                // sensible here.
                if (parameterChord.size() == 0) {
                    RG_DEBUG << "Figuration had no parameter chord"
                             << endl;
                    continue;
                }

                // Collect all events that start during it.
                timeT duration  = (*i)->getDuration();
                timeT startTime = (*i)->getAbsoluteTime();
                timeT endTime   = startTime + duration;

                // If the indication takes zero time, we don't collect
                // anything, because we don't collect events that
                // start exactly at the end.
                if (startTime == endTime) { continue; }

                // We allow any events that start within the interval,
                // but not exactly at the end of it.  So we don't
                // re-use iterator i, because it might already be past
                // relevant events.  And since we allow events whose
                // durations extend past the end of the indication, we
                // don't check for duration.
                Segment::iterator start = s->findTime(startTime);
                Segment::iterator end   = s->findTime(endTime);

                // When we consult key, it will always be the current
                // key at this time, so get key here.
                const Key key =
                    s->getKeyAtTime(startTime);
                
                for (Segment::iterator j = start;
                     j != end;
                     ++j) {
                    Event *e = (*j);
                    // !!! For now, just skip non-notes.  Later
                    // they'll have their own relativeEvent type
                    if (!e->isa(Note::EventType))
                        { continue; }
                    RG_DEBUG << "Figuration note"
                             << endl;
                
                    UnsolvedNote
                        possibleRelations =
                        getPossibleRelations(e, parameterChord,
                                             key, startTime);

                    notesToAccountFor.insert(possibleRelations);
                }

                // We want to not inspect any of these events again, so
                // continue the big loop from the next event.
                i = end;

                // !!!  We do not yet treat multiple specifications of
                // the same figuration.  For the second+ time, we would
                // only insert hypotheses that already exist.  We would
                // clear `seen' first and at the end, low-score any
                // hypothesis never seen in the second (etc) figuration.
                parameterChord.clear();
                // !!!  We do not yet treat multiple figurations.  So
                // immediately we figure out the winners, make a
                // figuration, and return it.

                for (UnsolvedFiguration::iterator j =
                         notesToAccountFor.begin();
                     j != notesToAccountFor.end();
                     ++j) {
                    UnsolvedNote hs = (*j);
                    RelativeEvent * bestRelation = 0;
                    int bestScore = -1000000;
                    for (UnsolvedNote::iterator k = hs.begin();
                         k != hs.end();
                         ++k) {
                        RelativeEvent * relation = *k;
                        int score = relation->getScore();
                        if (score > bestScore) {
                            bestRelation = relation;
                            bestScore = score;
                        }
                    }
                    if (bestRelation) {
                        RG_DEBUG << "Found a preferred relation"
                                 << endl;
                        events.push_back(bestRelation);
                    }
                }
                Figuration figurationAll = { events, duration, };
                return figurationAll;
            }
        }
    }

    Figuration figurationAll = { events, 0, };
    return figurationAll;
}

/// Initialize ExpandFigurationCommand
/// @author Tom Breton (Tehom)
void
ExpandFigurationCommand::initialise(SegmentSelection selection)
{
    Figuration figuration;
    // figurationSegment will be the first segment in which we find
    // figuration.
    Segment *figurationSegment = 0;

    RG_DEBUG << "Initializing ExpandFigurationCommand" << endl;
    for (SegmentSelection::iterator i = selection.begin();
         i != selection.end();
         ++i) {
        figuration = getFiguration(*i);
        if (figuration.m_events.size() > 0) {
            figurationSegment = *i;
            break;
        }
    }
    // If we didn't find a real figuration, there's nothing to do.
    if (figuration.m_events.size() == 0) { return; }

    
    // Expand figuration in each segment in selection except the
    // figuration segment itself.
    for (SegmentSelection::iterator i = selection.begin();
         i != selection.end();
         ++i) {
        Segment *s = (*i);
        // Don't try to expand the figuration segment.
        if (s == figurationSegment) { continue; }
        if ((s)->getType() != Segment::Internal) {
            // Audio segments shouldn't get here because
            // RosegardenMainWindow::slotExpandFiguration checks for
            // them
            throw Exception("Shouldn't get an audio segment in ExpandFigurationCommand.");
        }
        
        // Make a target segment
        Segment *target = s->clone(false);
        target->clear();
        target->fillWithRests(s->getEndTime());
        m_newSegments.insert(target);
        

        /** Add notes to target segment **/

        // We step figurationStartTime along the composition, finding
        // valid places to expand it.  We have two loop variables,
        // figurationStartTime and timePastFiguration.

        // didExpansion is true just if we expanded last iteration.
        // It helps us find valid times to expand.
        bool didExpansion = false;
        for (timeT
                 figurationStartTime =
                 figuration.getOKFigTime(m_composition, s->getStartTime()),
                 timePastFiguration =
                         figuration.getEndTime(figurationStartTime);
             // Test to continue
             figurationStartTime < s->getEndTime();
             // Step both loop variables.
             // If we did expansion, we can't expand again till after
             // timePastFiguration, otherwise we can try again at the
             // next legal time.
              figurationStartTime =
                 (didExpansion ?
                  figuration.getOKFigTime(m_composition, timePastFiguration) :
                  figuration.getNextFigTime(m_composition, figurationStartTime)),
              timePastFiguration =
                      figuration.getEndTime(figurationStartTime)) {

            didExpansion = false;

            // !!! Check that we can match time signature, otherwise
            // continue to next.

            /* Get one or more block chords */

            TimeSegmentedEventVector multiBlockChords;

            // Get the chord at this time.
            eventVector blockChord =
                getBlockChord(s, figurationStartTime);

            // If we have no match for the number of tones, continue
            // to the next possible time.
            // !!! For now, assume the right number of tones is 3.
            const unsigned int nbChordNotes = 3;
            if (blockChord.size() != nbChordNotes) { continue; }

            // Variables for the chord search loop and slightly past
            // it.  We keep track of when we looked for a chord, to
            // control search timing, and when we last found one, to
            // give us chord duration.
            timeT timeLookedChord = figurationStartTime;
            timeT timeGotChord    = figurationStartTime;

            // Iterate over events within the figuration duration, to
            // find more block chords in case they changed.
            Segment::iterator EventsPastFiguration = 
                s->findTime(timePastFiguration);
            for (Segment::iterator e = s->findTime(figurationStartTime);
                 e != EventsPastFiguration;
                 ++e) {
                // Skip events that are part of a chord we already
                // know about.
                if ((*e)->getAbsoluteTime() <= timeLookedChord)
                    { continue; }
                // Skip non-notes because they don't imply there's a
                // chord here.
                if (!(*e)->isa(Note::EventType))
                    { continue; }
                const timeT timeNow = (*e)->getAbsoluteTime();
                eventVector newBlockChord =
                    getBlockChord(s, timeNow);
                // If new chord is compatible with the old one, store
                // the *old* one, which now knows its complete
                // duration, and consider the new one the incomplete
                // one.
                if (newBlockChord.size() == nbChordNotes) {
                    // Store the old chord
                    TimedEventVector timedChord = {
                        blockChord,
                        timeGotChord,
                        timeNow,
                    };
                    multiBlockChords.push_back(timedChord);

                    // Switch state data to be about our new chord.
                    blockChord = newBlockChord;
                    timeGotChord = timeNow;
                    }
                else
                    // Clean up.  eventVector doesn't own the memory
                    // it points to, so there's nothing to do.
                    {
                        RG_DEBUG << "Got a chord with " <<
                            newBlockChord.size() <<
                            " notes instead of " <<
                            nbChordNotes <<
                            endl;
                    }
                // Whether we got a usable chord or not, we looked at
                // this time.
                timeLookedChord = timeNow;
            }

            // Store the final chord and its timing.  Since we started
            // with a chord and we always keep one incomplete one,
            // there's guaranteed to be a last chord.
            TimedEventVector timedChord = {
                blockChord,
                timeGotChord,
                timePastFiguration,
            };
            multiBlockChords.push_back(timedChord);
            
            // Get the key, for diatonic relations.
            const Key key =
                s->getKeyAtTime(figurationStartTime);
            
            // Write the respective notes into target
            RelativeEventVec & events = figuration.m_events;
            for (RelativeEventVec::iterator k = events.begin();
                 k != events.end();
                 ++k) {
                const timeT eventTime = figurationStartTime +
                    (*k)->getRelativeTime(); 
                const eventVector *pBlockChord;
                // Get the relevant blockChord.
                for (TimeSegmentedEventVector::iterator j =
                         multiBlockChords.begin();
                     j != multiBlockChords.end();
                     ++j) {
                        if ((eventTime >= j->start) &&
                            (eventTime < j->end))
                            {
                                pBlockChord = &j->chord;
                                break;
                            }
                    }
                if (!pBlockChord) { continue; }
                
                Event *newNote =
                    (*k)->getAsEvent(figurationStartTime,
                                     key,
                                     *pBlockChord);
                target->insert(newNote);
            }
            didExpansion = true;
        }
        m_composition->weakAddSegment(target);
        target->normalizeRests(s->getStartTime(),s->getEndTime());

        // Notation-quantize, otherwise the result is hard to read.
        NotationQuantizer nq;
        nq.quantize(target);

        // We do the actual placing of segments in (un)execute, so we
        // remove target from the composition.
        m_composition->weakDetachSegment(target);
    }
    
}

/// Execute the command
/// @author Tom Breton (Tehom)
void
ExpandFigurationCommand::execute()
{
    RG_DEBUG << "Executing ExpandFigurationCommand" << endl;
    m_composition->addAllSegments(m_newSegments);
    m_executed = true;
}

/// Unexecute the command
/// @author Tom Breton (Tehom)
void
ExpandFigurationCommand::unexecute()
{
    m_composition->detachAllSegments(m_newSegments);
    m_executed = false;
}

} // End namespace Rosegarden
