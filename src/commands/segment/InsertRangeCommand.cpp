/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "InsertRangeCommand.h"

#include "base/Clipboard.h"
#include "base/Composition.h"
#include "base/Segment.h"
#include "base/SegmentLinker.h"
#include "commands/segment/AudioSegmentSplitCommand.h"
#include "commands/segment/OpenOrCloseRangeCommand.h"
#include "commands/segment/SegmentJoinCommand.h"
#include "commands/segment/SegmentSplitCommand.h"
#include "document/LinkedSegmentsCommand.h"

#include <QObject>
#include <QtGlobal>

namespace Rosegarden
{
class SegmentGroupInsertRangeCommand : public LinkedSegmentsCommand
{
public:
    typedef std::vector<Segment *> SegmentVec;
    SegmentGroupInsertRangeCommand(SegmentVec originalSegments,
                             timeT splitTime, timeT duration,
                             Composition *composition) :
        LinkedSegmentsCommand(tr("Insert Range Helper"),
                              originalSegments, composition),
        m_splitTime(splitTime),
        m_duration(duration)
        {
            Q_ASSERT(duration > 0);
        }
    virtual ~SegmentGroupInsertRangeCommand();

protected:
    virtual void execute();
    virtual void unexecute();
    void     calculateNewSegments(void);
    Segment * splitRejoin(Segment *segment);

private:
    timeT getRangeDuration(void)
    { return m_duration; }

    timeT m_splitTime;
    timeT m_duration;
};

SegmentGroupInsertRangeCommand::~SegmentGroupInsertRangeCommand()
{}

Segment *
SegmentGroupInsertRangeCommand::
splitRejoin(Segment *segment)
{

    /*
      Initially we have:

      ---|---------
       SSSSSSS
    */

    SegmentVec segmentsAC = 
        SegmentSplitCommand::getNewSegments(segment, m_splitTime, true);
    Segment *segmentA = segmentsAC[0];
    Segment *segmentC = segmentsAC[1];


    /*
      Now we have two segments:

      ---|---------
      AAA CCC

    */

    // Move segment C as if inserting the range into composition.
    // This moves its end marker too, etc.
    segmentC->setStartTime(segmentC->getStartTime() + getRangeDuration());

    /*
      Now all of their times are as they will be after
      InsertRangeCommand has executed.

      ---|RANGE|---
      AAA       CCC

    */

    // Join A and C.  
    SegmentVec toBeJoined;
    toBeJoined.reserve(2);
    toBeJoined.push_back(segmentA);
    toBeJoined.push_back(segmentC);

    Segment * segmentFinal = 
        SegmentJoinCommand::makeSegment(toBeJoined);

    // A and C themselves will never be seen.
    delete segmentA;
    delete segmentC;

    /*
      Now we have one segment in its the final version.  It starts
      before the range and therefore won't be moved by
      OpenOrCloseRangeCommand.

      ---|RANGE|---
      FFFFFFFFFFFF

    */

    return segmentFinal;
}

void
SegmentGroupInsertRangeCommand::calculateNewSegments(void)
{
    Q_ASSERT(!m_originalSegments.empty());

    // Pick one original segment to work on.  The rest will just
    // be links to its result.

    // Initialize to a fallback value.
    Segment *segment = m_originalSegments[0];

    // Pick a segment that requires splitting.
    for (SegmentVec::const_iterator i = m_originalSegments.begin();
         i != m_originalSegments.end();
         ++i) {
        timeT endMarkerTime = (*i)->getEndMarkerTime(false);
        if (endMarkerTime > m_splitTime) {
            // This is the best that we can get, so we're done.
            segment = (*i);
            break;
        }
    }

    // We shouldn't have been called if there's not at least one
    // segment to split.
    Q_ASSERT(segment->getStartTime() < m_splitTime);

    // Get the resulting segment.
    Segment * segmentFinal = splitRejoin(segment);


    // For each original segment, make and store a corresponding
    // result segment.  One result segment will be segmentFinal,
    // the others will be created now.
    for (SegmentVec::const_iterator i = m_originalSegments.begin();
         i != m_originalSegments.end();
         ++i) {
        Segment *oldSegment = *i;
        Segment *newSegment =
            (oldSegment == segment) ?
            segmentFinal :
            SegmentLinker::createLinkedSegment(segmentFinal);

        // OpenOrCloseRangeCommand will change all the timing of
        // segments that start after the split, but we have to
        // manually adjust the end markers of segments that start
        // before it, which is true of all the segments we make.

        timeT endMarkerTime = oldSegment->getEndMarkerTime(false);
        if (endMarkerTime > m_splitTime) {
            endMarkerTime += getRangeDuration();
        }
        newSegment->setEndMarkerTime(endMarkerTime);
        copyAuxProperties(oldSegment, newSegment);
            
        m_newSegments.push_back(newSegment);
    }
}

void
SegmentGroupInsertRangeCommand::
execute()
{
    if (m_newSegments.empty()) { calculateNewSegments(); }
    executeAttachDetach();
}

void
SegmentGroupInsertRangeCommand::
unexecute()
{
    unexecuteAttachDetach();
}

InsertRangeCommand::InsertRangeCommand(Composition *composition,
				       timeT startTime, timeT duration) :
    MacroCommand(tr("Insert Range"))
{
    addInsertionCommands(this, composition, startTime, duration);
}

void
InsertRangeCommand::    
addInsertionCommands(MacroCommand *macroCommand,
                     Composition *composition,
                     timeT startTime, timeT duration)
{
    typedef std::vector<Segment *> SegmentVec;
    typedef std::multiset<Segment *,CompareForLinkedGroupSameTime>
        LinkedGroups;
    LinkedGroups linkedGroups;

    for (Composition::iterator i = composition->begin();
         i != composition->end(); ++i) {

        if ((*i)->getStartTime() >= startTime ||
            (*i)->getEndMarkerTime() <= startTime) {
            continue;
        }

        if ((*i)->getType() == Segment::Audio) {
            macroCommand->
                addCommand(new AudioSegmentSplitCommand(*i, startTime));
        } else {
                if ((*i)->getLinker() == 0) {
                    // If not linked, immediately add a command to
                    // handle it.  It can't be stored in linkedGroups
                    // because all unlinked Segments would wrongly
                    // look like members of the same link group.
                    macroCommand->
                        addCommand(new SegmentGroupInsertRangeCommand
                                   (SegmentVec(1, *i), startTime, duration, composition));
                } else {
                    // Otherwise store it.  It will be extracted as a
                    // part of its linked group.
                    linkedGroups.insert(*i);
                }
        }
    }

    // Some linked segments may be missed, eg if some members of the
    // group have their end-markers before the split.  This situation
    // is marginal enough that we don't try to handle it.
    
    // Add commands to handle the linked groups, each as a unit.
    LinkedGroups::iterator i = linkedGroups.begin();
    while (i != linkedGroups.end()) {
        LinkedGroups::iterator endOfGroup = linkedGroups.upper_bound(*i);
        SegmentVec segmentVec(i, endOfGroup);
        macroCommand->
            addCommand(new SegmentGroupInsertRangeCommand
                       (segmentVec, startTime, duration, composition));
        // Now step from the end of the group (ie, one past the last
        // element of this group)
        i = endOfGroup;
    }

    macroCommand->
        addCommand(new OpenOrCloseRangeCommand(composition, startTime,
                                               startTime + duration, true));
}


}
