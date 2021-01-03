/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "DeleteRangeCommand.h"

#include "base/Composition.h"
#include "base/Segment.h"
#include "base/SegmentLinker.h"
#include "base/Selection.h"
#include "commands/segment/AudioSegmentSplitCommand.h"
#include "commands/segment/EraseSegmentsStartingInRangeCommand.h"
#include "commands/segment/OpenOrCloseRangeCommand.h"
#include "commands/segment/SegmentJoinCommand.h"
#include "commands/segment/SegmentSplitCommand.h"
#include "document/LinkedSegmentsCommand.h"

#include <QtGlobal>

namespace Rosegarden
{

class SegmentGroupDeleteRangeCommand : public LinkedSegmentsCommand
{
public:
    typedef std::vector<Segment *> SegmentVec;
    SegmentGroupDeleteRangeCommand(SegmentVec originalSegments,
                             timeT firstSplitTime, timeT secondSplitTime,
                             Composition *composition) :
        LinkedSegmentsCommand(tr("Delete Range Helper"),
                              originalSegments, composition),
        m_firstSplitTime(firstSplitTime),
        m_secondSplitTime(secondSplitTime)
        {
            Q_ASSERT(firstSplitTime < secondSplitTime);
        }
    ~SegmentGroupDeleteRangeCommand() override;

protected:
    void execute() override;
    void unexecute() override;
    Segment *splitTwiceRejoin(Segment *segment);
    Segment *splitAtFirst(Segment *segment);
    Segment *splitAtSecond(Segment *segment);
    void     calculateNewSegments();

private:
    timeT getRangeDuration()
    { return m_secondSplitTime - m_firstSplitTime; }

    timeT m_firstSplitTime;
    timeT m_secondSplitTime;
};

SegmentGroupDeleteRangeCommand::~SegmentGroupDeleteRangeCommand()
{

}

Segment *
SegmentGroupDeleteRangeCommand::
splitAtFirst(Segment *segment)
{
    /*
      Initially we have:

      ---|RANGE|---
        SSSSS
    */

    SegmentVec segmentsAB = 
        SegmentSplitCommand::getNewSegments(segment, m_firstSplitTime, true);

    Segment *segmentA = segmentsAB[0];
    Segment *segmentB = segmentsAB[1];

    /*
      Now we have two segments:

      ---|RANGE|---
      AAA BBB

    */

    // Delete Segment B.
    delete segmentB;

    /*
      Now we have one segment in its final version.  It starts before
      the range and therefore won't be deleted by
      EraseSegmentsStartingInRangeCommand.

      ---|RANGE|---
      AAA 

    */

    return segmentA;
}

Segment *
SegmentGroupDeleteRangeCommand::
splitAtSecond(Segment *segment)
{
    /*
      Initially we have:

      ---|RANGE|---
            SSSSSSS
    */

    SegmentVec segmentsBC = 
        SegmentSplitCommand::getNewSegments(segment, m_secondSplitTime, true);
    Segment *segmentB = segmentsBC[0];
    Segment *segmentC = segmentsBC[1];

    /*
      Now we have two segments:

      ---|RANGE|---
            BBB CCC

    */

    // Delete Segment B.
    delete segmentB;

    /*
      Now we have one segment.  We don't move it in time, because it
      would be deleted by EraseSegmentsStartingInRangeCommand.
      OpenOrCloseRangeCommand will move it, so we don't have to.

      ---|RANGE|---
                CCC

    */

    return segmentC;
}

Segment *
SegmentGroupDeleteRangeCommand::
splitTwiceRejoin(Segment *segment)
{

    /*
      Initially we have:

      ---|RANGE|---
        SSSSSSSSSSS
    */

    SegmentVec segmentsAX = 
        SegmentSplitCommand::getNewSegments(segment,
                                            m_firstSplitTime, true);
    Segment *segmentA = segmentsAX[0];

    SegmentVec segmentsBC = 
        SegmentSplitCommand::getNewSegments(segmentsAX[1],
                                            m_secondSplitTime, true);
    Segment *segmentB = segmentsBC[0];
    Segment *segmentC = segmentsBC[1];

    /*
      Now we have three segments:

      ---|RANGE|---
      AAA BBBBB CCC

    */

    // Delete Segment B.
    delete segmentB;

    // Move segment C as if removing the range from composition.  This
    // moves its end marker too, etc.
    segmentC->setStartTime(segmentC->getStartTime() - getRangeDuration());

    /*
      Now we have two segments and all of their times are as they will
      be after DeleteRangeCommand has executed.

      ---|---------
      AAA CCC....

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
      before the range and therefore won't be deleted by
      EraseSegmentsStartingInRangeCommand or moved by
      OpenOrCloseRangeCommand.

      ---|---------
      FFFFFF.....

    */

    return segmentFinal;
}
void
SegmentGroupDeleteRangeCommand::
calculateNewSegments()
{
    Q_ASSERT(!m_originalSegments.empty());

    // Pick one original segment to work on.  The rest will just
    // be links to its result.

    bool splitBySecond = false;
    // Initialize to a fallback value.
    Segment *segment = m_originalSegments[0];

    // Pick the segment that requires the most splitting.
    for (SegmentVec::const_iterator i = m_originalSegments.begin();
         i != m_originalSegments.end();
         ++i) {
        timeT endMarkerTime = (*i)->getEndMarkerTime(false);
        if (endMarkerTime > m_secondSplitTime) {
            // This is the best that we can get, so we're done.
            segment = (*i);
            splitBySecond = true;
            break;
        } else if (endMarkerTime > m_firstSplitTime) {
            // This may be better than the default so store it.
            segment = (*i);
        }
    }
        
    bool splitByFirst =  (segment->getStartTime() < m_firstSplitTime);

    // There are no new segments, we are just detaching segments
    // wholly within the range.  (This case isn't used yet)
    if (!splitByFirst && !splitBySecond) { return; }

    // Get the resulting segment in the appropriate way.
    Segment * segmentFinal =
        (splitBySecond && splitByFirst) ? splitTwiceRejoin(segment) :
        splitByFirst ? splitAtFirst(segment) :
        splitAtSecond(segment);

    // Whether OpenOrCloseRangeCommand will adjust these segments'
    // timing.
    const bool timingWillBeAdjusted = splitBySecond && !splitByFirst;

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

        timeT endMarkerTime = oldSegment->getEndMarkerTime(false);

        // OpenOrCloseRangeCommand will change all the timing of
        // segments that start after the split, but we have to
        // manually adjust the end markers of segments that start
        // before it.
        if (!timingWillBeAdjusted) {
            if (endMarkerTime >= m_secondSplitTime) {
                endMarkerTime -= getRangeDuration();
            } else if (endMarkerTime >= m_firstSplitTime)
                { endMarkerTime = m_firstSplitTime; }
        }

        newSegment->setEndMarkerTime(endMarkerTime);
        copyAuxProperties(oldSegment, newSegment);
        
        m_newSegments.push_back(newSegment);
    }
}
    
void
SegmentGroupDeleteRangeCommand::
execute()
{
    if (m_newSegments.empty()) { calculateNewSegments(); }
    executeAttachDetach();
}

void
SegmentGroupDeleteRangeCommand::
unexecute()
{
    unexecuteAttachDetach();
}

DeleteRangeCommand::DeleteRangeCommand(Composition *composition,
                                       timeT t0, timeT t1) :
        MacroCommand(tr("Delete Range"))
{
    // First add commands to split the segments up.  Make a note of
    // segments that will need rejoining with their neighbours
    // afterwards.

    // Audio segments first
    for (int e = 0; e < 2; ++e) {

        // Split all segments at the range end first, then the range
        // begin afterwards.  This is because the split commands create
        // new segments for the right part and leave the left parts in
        // the original segments, so that we can use the same segment
        // pointer to do the left split as we did for the right

        timeT t = t1;
        if (e == 1)
            t = t0;

        for (Composition::iterator i = composition->begin();
                i != composition->end(); ++i) {

            if ((*i)->getType() == Segment::Audio) {

                if ((*i)->getStartTime() >= t || (*i)->getEndMarkerTime() <= t) {
                    continue;
                }

                addCommand(new AudioSegmentSplitCommand(*i, t));
            }
        }
    }

    // then non audio segments
    typedef std::vector<Segment *> SegmentVec;
    LinkedGroups linkedGroups;
    for (Composition::iterator i = composition->begin();
            i != composition->end(); ++i) {

        if ((*i)->getType() != Segment::Audio) {

            // How many time to split the segment ?
            int count = 0;
            if (t0 > (*i)->getStartTime() && t0 < (*i)->getEndMarkerTime()) {
                count++;
            }
            if (t1 > (*i)->getStartTime() && t1 < (*i)->getEndMarkerTime()) {
                count++;
            }

            if (count > 0) {
                if ((*i)->getLinker() == nullptr) {
                    // If not linked, immediately add a command to
                    // handle it.  It can't be stored in linkedGroups
                    // because all unlinked Segments would wrongly
                    // look like members of the same link group.
                    addCommand(new SegmentGroupDeleteRangeCommand
                               (SegmentVec(1, *i), t0, t1, composition));
                } else {
                    // Otherwise store it.  It will be extracted as a
                    // part of its linked group.
                    linkedGroups.insert(*i);
                }
            }
            // If there are no splits, do nothing for this segment.
        }
    }

    // Add commands to handle the linked groups, each as a unit.
    LinkedGroups::iterator i = linkedGroups.begin();
    while (i != linkedGroups.end()) {
        LinkedGroups::iterator endOfGroup = linkedGroups.upper_bound(*i);
        SegmentVec segmentVec(i, endOfGroup);
        addCommand(new SegmentGroupDeleteRangeCommand
                   (segmentVec, t0, t1, composition));
        // Now step from the end of the group (ie, one past the last
        // element of this group)
        i = endOfGroup;
    }

    // Add commands to do the rest of the work

    // !!! Now this does almost nothing, just needed for small
    // segments entirely within the deleted range.
    addCommand(new EraseSegmentsStartingInRangeCommand(composition, t0, t1));

    addCommand(new OpenOrCloseRangeCommand(composition, t0, t1, false));
}

DeleteRangeCommand::~DeleteRangeCommand()
{
}

}
