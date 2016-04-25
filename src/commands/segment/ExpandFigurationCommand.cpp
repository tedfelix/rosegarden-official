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
#define RG_MODULE_STRING "[ExpandFigurationCommand]"

#include "ExpandFigurationCommand.h"
#include "base/Composition.h"
#include "base/Event.h"
#include "base/Selection.h"
#include "base/figuration/ChordSegment.h"
#include "base/figuration/SegmentFigData.h"
#include "base/figuration/SegmentID.h"
#include "commands/segment/SegmentInsertCommand.h"
#include "misc/Debug.h"

#include <QtGlobal>

namespace Rosegarden
{

/// Construct ExpandFigurationCommand
/// @author Tom Breton (Tehom)
ExpandFigurationCommand::ExpandFigurationCommand(SegmentSelection selection)
  :
  MacroCommand(getGlobalName()),
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

/// Return a legal time for expansion no earlier than raw
/// @author Tom Breton (Tehom)
timeT
ExpandFigurationCommand::
rawStartTimeToExact(timeT raw)
{
    // If raw was exactly on a bar line, we'd get the next bar line,
    // so step it back by one.  This will never result in a time
    // earlier than raw.
    return m_composition->getBarEndForTime(raw - 1);
}


/// Initialize ExpandFigurationCommand
/// @author Tom Breton (Tehom)
void
ExpandFigurationCommand::initialise(SegmentSelection selection)
{
    FigurationVector figs;
    int figSourceID = -1;
    bool gotFigSource = false;

    RG_DEBUG << "Initializing ExpandFigurationCommand";

    // Update, because if we need new IDs they mustn't duplicate old
    // IDs, so we must be up to date on what IDs are there.
    SegmentFigData::SegmentFigDataMap segMap = 
        SegmentFigData::getInvolvedSegments(true, this);
    for (SegmentSelection::iterator i = selection.begin();
         i != selection.end();
         ++i) {
        SegmentFigData& segmentData =
            SegmentFigData::findOrAdd(segMap, *i);
        
        // If it's used here, it's not uninvolved, so unless it's a
        // figuration, it's a chord source.
        if (segmentData.isa(SegmentFigData::Uninvolved)) {
            segmentData.convertType(SegmentFigData::ChordSource);
        }
        segmentData.addTagIfNeeded(*i, this);
        if (gotFigSource ||
            !segmentData.isa(SegmentFigData::FigurationSource))
            { continue; }

        figSourceID = segmentData.getID();
        figs = FigurationSourceMap::getFigurations(*i);
        if (!figs.empty())
            { gotFigSource = true; }
    }
    // If we didn't find a real figuration, there's nothing to do.
    if (!gotFigSource) { return; }

    SourcedFiguration sourcedfigs(figSourceID, figs);
    
    // Expand figuration in each segment in selection except the
    // figuration segment itself.
    for (SegmentSelection::iterator i = selection.begin();
         i != selection.end();
         ++i) {
        Segment *s = (*i);
        SegmentFigData::SegmentFigDataMap::iterator it = segMap.find(*i);
        Q_ASSERT_X(it != segMap.end(),
                   "ExpandFigurationCommand::initialise",
                   "didn't find the segment");
        SegmentFigData& segmentData = it->second;
        
        if (!segmentData.isa(SegmentFigData::ChordSource))
            { continue; }

        // Make a target segment
        Segment *target = s->clone(false);
        // Temporarily attach it to composition so that expand can
        // work.
        m_composition->weakAddSegment(target);

        target->clear();
        target->fillWithRests(s->getEndTime());
        SegmentFigData::addTag(target, this, SegmentID::Target);
        m_newSegments.insert(target);

        /** Add notes to target segment **/
        for (Segment::iterator e = s->begin();
             e != s->end();
             ++e) {
            // Non-notes they don't imply there's a chord here.
            // We add them to target segment in case they are
            // clefs or key changes.
            if ((*e)->isa(SegmentID::EventType)) {
                continue;
            }
            if (!(*e)->isa(Note::EventType)) {
                target->insert(new Event(**e));
            }
        }


        // rawStartTime is the apparent start time before we take bars
        // into account.  We step it along the composition, finding
        // valid places to expand.  Specifically, on bar lines not
        // already part of an expansion.
        timeT rawStartTime = s->getStartTime();
        while (1) {
            timeT figurationStartTime = rawStartTimeToExact(rawStartTime);

            if (rawStartTime >= s->getEndTime())
                { break; }

            timeT timePastFiguration =
                SegmentFigData::expand(sourcedfigs,
                                       ChordSegment(s, segmentData.getID()),
                                       target, figurationStartTime);

            // If we didn't expand, ensure we don't try endlessly at
            // the same place.
            if (timePastFiguration == figurationStartTime) {
                ++timePastFiguration;
            }
            rawStartTime = timePastFiguration;
        }

        // Detach from composition, because SegmentInsertCommand does
        // the actual placing
        m_composition->weakDetachSegment(target);

        
        Command *c =
            new SegmentInsertCommand(m_composition, target, s->getTrack());
        addCommand(c);
    }

}

} // End namespace Rosegarden
