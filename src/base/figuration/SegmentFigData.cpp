/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2012 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[SegmentFigData]"

#include "SegmentFigData.h"

#include "base/BasicQuantizer.h"
#include "base/Composition.h"
#include "base/NotationTypes.h"
#include "base/Segment.h"
#include "base/figuration/ChordSegment.h"
#include "base/figuration/FigChord.h"
#include "base/figuration/FigurationSourceMap.h"
#include "base/figuration/GeneratedRegion.h"
#include "base/figuration/RelativeEvent.h"
#include "base/figuration/SegmentID.h"
#include "commands/edit/EventInsertionCommand.h"
#include "document/BasicCommand.h"
#include "document/DocumentGet.h"
#include "misc/Debug.h"

#include <QObject>
#include <QtGlobal>

#include <vector>


namespace Rosegarden
{

/*** Helper class ReplaceRegionCommand ***/

    
class ReplaceRegionCommand : public BasicCommand
{
public:
    ReplaceRegionCommand(Segment &segment, Segment *redoEvents) :
        BasicCommand(QObject::tr("Replace segment contents"),
                     segment, redoEvents)
        
        {}
    void modifySegment(void) {}
};


/*** Helper class TargetSegment ***/

class TargetSegment : public std::vector<Event*>
{
public:
    TargetSegment(Segment* s);
    void update(ChordSegmentMap& chordSources,
                FigurationSourceMap& figurationSources,
                MacroCommand* command);

private:
    static Segment *newEventHolder(Segment *s, Composition *c);
    static void doneEventHolder(Segment *s, Composition *c,
                                Segment *target, MacroCommand* command);

    Segment* m_s;
};

/*** Helper class TargetSet ***/

class TargetSet : public std::vector<TargetSegment>
{
public:
    void update(ChordSegmentMap& chordSources,
                FigurationSourceMap& figurationSources,
                MacroCommand* command) {

        for (iterator i = begin(); i != end(); ++i) {
            i->update(chordSources, figurationSources, command);
        }
    }
        
    void addTargets(Segment* s)
    { push_back(TargetSegment(s)); }
};

TargetSegment::
TargetSegment(Segment* s)
    : m_s(s)
{
    timeT lastEnd = s->getStartTime();
    for (Segment::iterator i = s->begin();
         i != s->end();
         ++i) {
        Event* e = *i;
        timeT start = e->getAbsoluteTime();
        if (e->isa(GeneratedRegion::EventType) && (start >= lastEnd)) {
            push_back(e);
            lastEnd = start + e->getDuration();
        }
    }
}

// Caller takes ownership of return value
Segment *
TargetSegment::
newEventHolder(Segment *s, Composition *comp)
{
    // Make a fresh segment that we will give to ReplaceRegionCommand.
    Segment* eventHolder = new Segment(Segment::Internal, s->getStartTime());
    eventHolder->setQuantizeLevel(s->getQuantizer()->getUnit());
        
    // Temporarily attach it to composition so that expand can
    // work.
    comp->weakAddSegment(eventHolder);
    return eventHolder;
}

// Takes ownership of `target'.  Safe even if target is NULL.
void
TargetSegment::
doneEventHolder(Segment *s, Composition *comp,
                Segment *target, MacroCommand* command)
{
    if (!target) { return; }
    comp->weakDetachSegment(target);
    // Empty targets can cause serious problems, because the resulting
    // ReplaceRegionCommand would delete everything at time zero.
    if (target->empty()) {
        delete target;
        return;
    }
    Command *c = new ReplaceRegionCommand(*s, target);
    command->addCommand(c);
}

void
TargetSegment::
update(ChordSegmentMap& chordSources,
       FigurationSourceMap& figurationSources,
       MacroCommand* command)
{
    Q_CHECK_PTR(command);
    Composition *comp = DocumentGet::getComposition();
    Segment* s = m_s;
    // Initialize `target' as NULL.  We allocate it just-in-time
    // because we use and release it in some iterations.
    Segment* target = 0;

    // For telling whether there's space between one region and the
    // next.  Since we get the regions in forward order, this is
    // simple.  The initial value doesn't matter much because the
    // relevant branch does nothing when `target' is NULL.
    timeT lastEnd = 0;
    
    // Expand for each target
    for (iterator i = begin(); i != end(); ++i) {
        Event* e = (*i);
        Q_ASSERT(e->isa(GeneratedRegion::EventType));
        GeneratedRegion generatedRegion(*e);
              
        /*** Find the respective sources ***/
          
        int figurationSourceIndex =
            generatedRegion.getFigurationSourceID();
        FigurationSourceMap::iterator figurationIter =
            figurationSources.find(figurationSourceIndex);
        if (figurationIter == figurationSources.end())
            { continue; }
        SourcedFiguration& figuration = *figurationIter;

        int chordSourceIndex =
            generatedRegion.getChordSourceID();
        ChordSegmentMap::iterator chordSourceIter =
            chordSources.find(chordSourceIndex);
        if (chordSourceIter == chordSources.end())
            { continue; }
        ChordSegment& chordSource = chordSourceIter->second;
        
        // Find it by time.
        timeT startRegion = e->getAbsoluteTime();
        timeT endRegion = startRegion + e->getDuration();

        // If we've just now gone past a contiguous region, save what
        // we got and start another.
        if (lastEnd < startRegion) {
            doneEventHolder(s, comp, target, command);
            target = 0;
        }

        // Make sure we have something to write to.
        if (!target) { target = newEventHolder(s, comp); }

        /*** Write new contents ***/
        timeT newTime =
            SegmentFigData::expand(figuration, chordSource, target,
                                   startRegion);
        // If we expanded nothing, the rest of this block is
        // inapplicable so start a new iteration.
        if (newTime == startRegion)
            { continue; }

        // Now update lastEnd for the next iteration.
        lastEnd = endRegion;
        
        // Copy selected other contents of that region
        Segment::iterator end = s->findTime(endRegion);
        for (Segment::iterator j = s->findTime(startRegion); j != end; ++j) {
            if (SegmentFigData::eventShouldPass(*j))
                { target->insert(new Event(**j)); }
        }
    }

    // Finally, save the last contiguous region.  Safe even if we did
    // zero iterations and `target' is NULL.
    doneEventHolder(s, comp, target, command);
}

bool
SegmentFigData::
eventShouldPass(Event *e)
{
    if (e->isa(Key::EventType)) { return true; }
    if (e->isa(Clef::EventType)) { return true; }
    // Typically volume controllers should pass thru while expression
    // controllers shouldn't.  As yet, we make no provision for
    // setting this more generally.
    if (e->isa(Controller::EventType) && 
        (e->has(Controller::NUMBER) &&
         e->get <Int>(Controller::NUMBER) == 7))
        { return true; }
    return false;
}

/*** Helper class DelimitedChord and ChordSequence ***/

/// @class DelimitedChord
/// A chord together with its start and end times.
/// @author Tom Breton (Tehom)
class DelimitedChord
{
public:
    DelimitedChord(FigChord *chord, timeT m_start)
        : m_chord(chord),
          m_start(m_start)
          {}

    void        setEndTime(timeT end) { m_end = end; }
    FigChord *m_chord;
    timeT     m_start;
    timeT     m_end;
};

/// @typedef ChordSequence
/// Several DelimitedChords in the same bar (or other group)
/// @author Tom Breton (Tehom)
typedef std::vector<DelimitedChord> ChordSequence;

/*** SegmentFigData itself ***/

int
SegmentFigData::
m_maxID = -1;

SegmentFigData::SegmentFigData(Segment* s) :
    m_type(Uninvolved),
    m_needsTag(false),
    m_id(-1)
{
    // Audio segments can't do this.
    if ((s)->getType() != Segment::Internal) {
        m_type = Unavailable;
        return;
    }
          
    // Loop thru events until one tells us what this segment is.
    for (Segment::iterator i = s->begin(); i != s->end(); ++i) {
        const std::string &type = (*i)->getType();

        /** Consider the various relevant types of event **/

        // The segment may be directly identified by an ID

        // !!! Possible optimization: If it's not at segment start
        // time, remove this event and give m_needsTag as true.
        if (type == SegmentID::EventType) {
            SegmentID segmentID(**i);
            const std::string subtype = segmentID.getType();

            if (subtype == SegmentID::ChordSource) {
                m_needsTag = false;
                m_id = segmentID.getID();
                m_type = ChordSource;
                return;
            } else if (subtype == SegmentID::FigurationSource) {
                m_needsTag = false;
                m_id = segmentID.getID();
                m_type = FigurationSource;
                return;
            }
            else if (subtype == SegmentID::Target) {
                m_needsTag = false;
                m_type = Target;
                return;
            }
        }

        // The presence of figuration indications implies this
        // segment is a figuration source.
        else if (type == Indication::EventType) {
            const std::string subtype = Indication(**i).getIndicationType();

            // A parameter chord or figuration implies this segment is a
            // figuration source.
            if ((subtype == Indication::FigParameterChord) ||
                (subtype == Indication::Figuration)) {
                m_needsTag = true;
                m_type = FigurationSource;
                return;
            }
        }

        // The presence of a generated region implies this segment
        // is a figuration target.
        else if (type == GeneratedRegion::EventType) {
            m_needsTag = true;
            m_type = Target;
        }

        continue;
    }

    // We got to the end without finding anything figuration-relevant,
    // so leave the defaults in place.
    return;
}


// After this returns, all source and target segments have IDs at
// their starting time and the two maxID static members have the
// value of the maximum current ID.
SegmentFigData::SegmentFigDataMap
SegmentFigData::
getInvolvedSegments(bool onlyIfNeedTag, MacroCommand* command)
{
    typedef SegmentFigDataMap::value_type ValueType;
    SegmentFigDataMap   storedScans;

    segmentcontainer ss = Segment::getCompositionSegments();

    // Loop thru all segments with scanSegment.
    for (segmentcontainer::iterator i = ss.begin();
	 i != ss.end();
	 ++i) {
        SegmentFigData segmentData(*i);
      
        // Set the relevant ID max, if any, to the maximum of its
        // previous value and this id.
        switch (segmentData.m_type) {
        case ChordSource:
        case FigurationSource:
        case Target:
            m_maxID = std::max(m_maxID, segmentData.m_id);
            // Store it, except if `onlyIfNeedTag' is set, then store
            // it only if it needs a tag
            if (!onlyIfNeedTag || segmentData.m_needsTag)
                { storedScans.insert(ValueType(*i, segmentData)); }
            break;

        case Uninvolved:  // Uninvolved don't have any indications.
        case Unavailable: // Audio segments can't.
        default:
            break;
        }
    }

    for (SegmentFigDataMap::iterator i = storedScans.begin();
	 i != storedScans.end();
	 ++i)
        { (*i).second.addTagIfNeeded((*i).first, command); }

    return storedScans;
}


void
SegmentFigData::
addTagIfNeeded(Segment *s, MacroCommand* command)
{
    if (!m_needsTag) { return; }

    // Add the respective event at start time.
    std::string type;
    switch (m_type) {
    case ChordSource:
        type = SegmentID::ChordSource;
        break;
    case FigurationSource:
        type = SegmentID::FigurationSource;
        break;

    case Target:
        type = SegmentID::Target;
        break;
    case Uninvolved:
    case Unavailable:
    default:
        // This shouldn't be reached.
        return;
    }
    if (!IsValidId(m_id)) {
        m_id = getUnusedSegmentID();
    }

    addTag(s, command, type, m_id);
    m_needsTag = false;
}

SegmentFigData &
SegmentFigData::
findOrAdd(SegmentFigDataMap &map, Segment *s)
{
  SegmentFigDataMap::iterator it = map.find(s);
  if (it != map.end()) { return it->second; }
  // If it wasn't there, add it now.

  typedef SegmentFigDataMap::value_type ValueType;
  it = map.insert(ValueType(s,SegmentFigData(s))).first;
  return it->second;
}


void
SegmentFigData::
addTag(Segment* s, MacroCommand* command, std::string type, int id)
{
  timeT t = s->getStartTime();
  SegmentID segmentID(type, id);
  Event * e = segmentID.getAsEvent(t);
  command->addCommand(new EventInsertionCommand(*s, e));
}

void
SegmentFigData::
updateComposition(MacroCommand* command)
{
    SegmentFigDataMap segs =
        getInvolvedSegments(false, command);

    TargetSet targetSet;
    ChordSegmentMap chordSources;
    FigurationSourceMap figurationSources;

    for (SegmentFigDataMap::iterator i = segs.begin();
         i != segs.end();
         ++i) {
        Segment *s = i->first;
        SegmentFigData &data = i->second;

        Q_ASSERT(!data.m_needsTag);
      
        // Set the relevant ID max, if any, to the maximum of its
        // previous value and this id.
        switch (data.m_type) {
        case ChordSource:
            chordSources.addSource(s, data.m_id);
            break;

        case FigurationSource:
            figurationSources.addSource(s, data.m_id);
            break;

        case Target:
            targetSet.addTargets(s);
            break;
        case Uninvolved:
        case Unavailable:
        default:
            break;
        }
    }

    targetSet.update(chordSources, figurationSources, command);
}

timeT
SegmentFigData::expand(SourcedFiguration& sourcedFiguration,
                       ChordSegment chordSource,
                       Segment*    target,
                       timeT       startTime)
{
    // Target had better be in a composition at least temporarily.
    Q_ASSERT(target->getComposition());

    /*** Get preliminary data ***/

    // Current time signature
    TimeSignature timeSig =
        target->getComposition()->getTimeSignatureAt(startTime);

    FindFigChords finder(chordSource.m_s, startTime);

    FigChord *blockChord = finder.getChordNow(startTime + 1);
    if (!blockChord) { return startTime; }

    /** Find a figuration that matches. **/

    // Try to get figuration
    Figuration *figuration =
        FigurationSourceMap::
        findMatch(sourcedFiguration.second,
                  timeSig.getNumerator(),
                  timeSig.getDenominator(),
                  blockChord->size());

    // If we didn't get any match, bail out.
    if (!figuration) {
        delete blockChord;
        return startTime;
    }

    /** Create and initialize the loop variables **/


    const unsigned int nbChordNotes = figuration->m_NbParameters;

    timeT timePastFiguration =
        figuration->getEndTime(startTime);
    // The chord whose time is yet to be set.
    DelimitedChord * waitingChord;

    ChordSequence chordSequence;

    // Add the first chord.  We don't know its end time yet.
    chordSequence.push_back(DelimitedChord(blockChord, startTime));
    waitingChord = &chordSequence.back();

    for(;;++finder) {
        FigChord *newBlockChord = finder.getChordNow(timePastFiguration);
        if (!newBlockChord) { break; }
        const timeT timeNow = finder.timeNow();
        // The rest as before.
        if ((newBlockChord->size() == nbChordNotes) ||
            ((nbChordNotes == 0) && !newBlockChord->empty())) {
            // Set the end time of the old chord
            waitingChord->setEndTime(timeNow);
            // Make a new chord, incomplete.
            chordSequence.push_back(DelimitedChord(newBlockChord, timeNow));
            // Now this is the chord we're waiting for the end
            // of.
            waitingChord = &chordSequence.back();
        } else {
            RG_DEBUG << "Got a chord with"
                     << newBlockChord->size()
                     << "notes instead of"
                     << nbChordNotes
                     << endl;
            delete newBlockChord;
            return startTime;
        }
    }

    // Store the final chord's end time
    waitingChord->setEndTime(timePastFiguration);

    /*** Now we have all the source chords. ***/

    
    // Get the key, for diatonic relations.
    const Key key =
        chordSource.m_s->getKeyAtTime(startTime);
            
    // Write an indication for the whole thing.
    {
        GeneratedRegion
            generatedRegion(chordSource.m_ID,
                            sourcedFiguration.first,
                            figuration->m_duration);
        Event * e = generatedRegion.getAsEvent(startTime);
        (void)target->insert(e);
    }
    // Write the respective notes into target
    RelativeEventVec & events = figuration->m_events;
    for (RelativeEventVec::iterator k = events.begin();
         k != events.end();
         ++k) {
        const timeT eventTime =
            (*k)->getAbsoluteTime(startTime);
        const FigChord *pBlockChord = 0;

        // Get the relevant blockChord.
        for (ChordSequence::iterator j =
                 chordSequence.begin();
             j != chordSequence.end();
             ++j) {
            if ((eventTime >= j->m_start) &&
                (eventTime < j->m_end))
                {
                    pBlockChord = j->m_chord;
                    break;
                }
        }
        // Here we skip no-source regions
        if (!pBlockChord || pBlockChord->empty()) { continue; }
                
        Event *newNote =
            (*k)->getAsEvent(startTime,
                             key,
                             pBlockChord);
        target->insert(newNote);
    }

    // We're done with chordSequence.
    for (ChordSequence::iterator j =
             chordSequence.begin();
         j != chordSequence.end();
         ++j)
        { delete j->m_chord; }
    chordSequence.clear();

    /** Now just make it nice **/
    
    // Normalize the rests
    target->normalizeRests(startTime,timePastFiguration);

    // Notation-quantize, otherwise the result is hard to read.
    target->getQuantizer()->quantize(target,
                                     target->findTime(startTime),
                                     target->findTime(timePastFiguration));
    
    return timePastFiguration;
}

}
