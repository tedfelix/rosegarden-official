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

#include "SegmentLinker.h"

#include "Segment.h"
#include "Event.h"
#include "document/CommandHistory.h"
#include "document/Command.h"
#include "BaseProperties.h"
#include "base/SegmentNotationHelper.h"
#include "misc/Debug.h"

#include <algorithm>

namespace Rosegarden
{

SegmentLinker::SegmentLinkerId SegmentLinker::m_count = 0;

SegmentLinker::SegmentLinker()
{
    connect(CommandHistory::getInstance(), &CommandHistory::updateLinkedSegments,
        this, &SegmentLinker::slotUpdateLinkedSegments);

    ++m_count;
    m_id = m_count;
    m_reference = nullptr;
}

SegmentLinker::SegmentLinker(SegmentLinkerId id)
{
    connect(CommandHistory::getInstance(), &CommandHistory::updateLinkedSegments,
        this, &SegmentLinker::slotUpdateLinkedSegments);

    m_id = id;
    m_count = std::max(m_count,m_id+1);
    m_reference = nullptr;
}

SegmentLinker::~SegmentLinker()
{

}

SegmentLinker::LinkedSegmentParamsList::iterator
SegmentLinker::findParamsItrForSegment(Segment *s)
{
    LinkedSegmentParamsList::iterator itr;
    for(itr = m_linkedSegmentParamsList.begin();
        itr!= m_linkedSegmentParamsList.end(); ++itr) {
        if(itr->m_linkedSegment == s) {
            break;
        }
    }

    return itr;
}

void
SegmentLinker::addLinkedSegment(Segment *s)
{
    LinkedSegmentParamsList::iterator itr = findParamsItrForSegment(s);
    if (itr == m_linkedSegmentParamsList.end()) {
        m_linkedSegmentParamsList.push_back(LinkedSegmentParams(s));
        s->setLinker(this);
    }
}

void
SegmentLinker::removeLinkedSegment(Segment *s)
{
    LinkedSegmentParamsList::iterator itr = findParamsItrForSegment(s);
    if (itr != m_linkedSegmentParamsList.end()) {
        m_linkedSegmentParamsList.erase(itr);
        s->setLinker(nullptr);
    }
}

/*static*/ Segment*
SegmentLinker::createLinkedSegment(Segment *s)
{
    handleImpliedCMajor(s);
    Segment *linkedSeg = s->clone();
    if (!s->isLinked()) {
        //here we need to also create a linker
        SegmentLinker *linker = new SegmentLinker();
        linker->addLinkedSegment(s);
        linker->addLinkedSegment(linkedSeg);
    }

    return linkedSeg;
}

/*static*/ void
SegmentLinker::handleImpliedCMajor(Segment *s)
{
    //need to handle implied C Major in segments without a key change event
    //at the start of them

    //rather than keep having to write "handle implied C Major key in segments
    //with no key change event at segment start" code, i'm just going to stick
    //a null-op C Major key at the start (if no key already exists there)

    bool foundKey = false;

    timeT segFrom = s->getStartTime();
    timeT segTo = segFrom + 1;
    Segment::const_iterator itrFrom = s->findTime(segFrom);
    Segment::const_iterator itrTo = s->findTime(segTo);

    for(Segment::const_iterator itr = itrFrom; itr != itrTo; ++itr) {
        if ((*itr)->isa(Rosegarden::Key::EventType)) {
            foundKey = true;
            break;
        }
    }

    if (!foundKey) {
        Rosegarden::Key key;
        SegmentNotationHelper helper(*s);
        helper.insertKey(s->getStartTime(),key);
    }
}

/*static*/ bool
SegmentLinker::unlinkSegment(Segment *s)
{
    bool retVal = false;

    if (s->isLinked()) {
        retVal = true;
        SegmentLinker* linker = s->getLinker();
        linker->removeLinkedSegment(s);

        //in the case that the linker has no more linked segments, delete the
        //segment linker
        if (linker->getNumberOfLinkedSegments() == 0)
        {
            delete linker;
        }
    }

    return retVal;
}

void
SegmentLinker::slotUpdateLinkedSegments(Command *command)
{
    //only the first segment with an invalidated refresh region will be
    //processed. If there are others, their changes will be ignored.
    bool linkedSegmentsUpdated = false;

    LinkedSegmentParamsList::iterator itr;
    for(itr = m_linkedSegmentParamsList.begin();
        itr!= m_linkedSegmentParamsList.end(); ++itr) {

        LinkedSegmentParams &linkedSegParams = *itr;
        Segment *linkedSeg = linkedSegParams.m_linkedSegment;
        uint refreshStatusId = linkedSegParams.m_refreshStatusId;
        SegmentRefreshStatus &rs = linkedSeg->getRefreshStatus(refreshStatusId);

        //have we already done an update?
        if (!linkedSegmentsUpdated) {

            if (command->getUpdateLinks() && rs.needsRefresh()) {
                linkedSegmentChanged(linkedSeg,rs.from(),rs.to());
                linkedSegmentsUpdated = true;
            }
        } else {
            RG_WARNING << "oops, trying to update linked segment set twice!";
        }

        rs.setNeedsRefresh(false);
    }
}

void
SegmentLinker::linkedSegmentChanged(Segment *s, const timeT from,
                                                const timeT to)
{
    //go through the other linked segments which aren't s, and copy the events
    //in the range [from,to] to them, accounting for time and pitch shifts

    const timeT sourceSegStartTime = s->getStartTime();
    const timeT refFrom = from - sourceSegStartTime;
    const timeT refTo = to - sourceSegStartTime;

    // Used to memorize a possible change in lyrics
    bool lyricsChanged = false;

    LinkedSegmentParamsList::iterator itr;
    for(itr = m_linkedSegmentParamsList.begin();
        itr!= m_linkedSegmentParamsList.end(); ++itr) {

        LinkedSegmentParams &linkedSegParams = *itr;
        Segment *linkedSegToUpdate = linkedSegParams.m_linkedSegment;
        uint refreshStatusId = linkedSegParams.m_refreshStatusId;
        SegmentRefreshStatus &rs =
                        linkedSegToUpdate->getRefreshStatus(refreshStatusId);

        if(s == linkedSegToUpdate) {
            continue;
        }

        // Don't send unnecessary resize notifications to observers
        linkedSegToUpdate->lockResizeNotifications();

        timeT segStartTime = linkedSegToUpdate->getStartTime();
        timeT segFrom = segStartTime + refFrom;
        timeT segTo = segStartTime + refTo;
        Segment::iterator itrFrom = linkedSegToUpdate->findTime(segFrom);
        Segment::iterator itrTo = linkedSegToUpdate->findTime(segTo);
        lyricsChanged = eraseNonIgnored(linkedSegToUpdate,
                                        itrFrom, itrTo, lyricsChanged);

        //now go through s from 'from' to 'to', inserting the equivalent
        //event in linkedSegToUpdate
        for(Segment::const_iterator itr = s->findTime(from);
                                    itr != s->findTime(to); ++itr) {
            const Event *e = *itr;

            timeT eventT = (e->getAbsoluteTime() - sourceSegStartTime)
                           + segStartTime;

            timeT eventNotationT = (e->getNotationAbsoluteTime() - sourceSegStartTime)
                                   + segStartTime;

            int semitones =
                    linkedSegToUpdate->getLinkTransposeParams().m_semitones -
                                    s->getLinkTransposeParams().m_semitones;
            int steps = linkedSegToUpdate->getLinkTransposeParams().m_steps -
                                        s->getLinkTransposeParams().m_steps;

            lyricsChanged = insertMappedEvent(linkedSegToUpdate, e, eventT,
                                              eventNotationT, semitones, steps,
                                              lyricsChanged);
        }

        // Fix verses count if lyrics have been modified
        if (lyricsChanged) linkedSegToUpdate->invalidateVerseCount();

        // Now only send one resize notification to observers if needed.
        linkedSegToUpdate->unlockResizeNotifications();

        rs.setNeedsRefresh(false);
    }
}

bool
SegmentLinker::insertMappedEvent(Segment *seg,
                                 const Event *e, timeT t, timeT nt,
                                 int semitones, int steps,
                                 bool lyricsAlreadyInserted)
{
    bool lyricInserted = lyricsAlreadyInserted;

    bool ignore;
    if (e->get<Bool>(BaseProperties::LINKED_SEGMENT_IGNORE_UPDATE, ignore)
        && ignore) {
        return lyricInserted;
    }

    Event *refSegEvent = new Event(*e,
                                   t,
                                   e->getDuration(),
                                   e->getSubOrdering(),
                                   nt,
                                   e->getNotationDuration());

    bool needsInsertion = true;

    //correct for temporal (and pitch shift??) here eventually...
    if (semitones!=0) {
        if (e->isa(Note::EventType)) {
            long oldPitch = 0;
            if (e->get<Int>(BaseProperties::PITCH, oldPitch)) {
                long newPitch = oldPitch + semitones;
                refSegEvent->set<Int>(BaseProperties::PITCH, newPitch);
            }
        } else if (e->isa(Rosegarden::Key::EventType)) {
            Rosegarden::Key trKey = (Rosegarden::Key (*e)).transpose(semitones,
                                                                         steps);
            delete refSegEvent;
            refSegEvent = nullptr;
            SegmentNotationHelper helper(*seg);
            helper.insertKey(t,trKey);
            needsInsertion = false;
        }
    }

    if (needsInsertion) {

        if (! lyricInserted) {
            // Is the inserted event a lyric?
            if (e->isa(Text::EventType)) {
                std::string textType;
                lyricInserted =
                    e->get<String>(Text::TextTypePropertyName, textType)
                    && (textType == Text::Lyric);
                }
        }

        seg->insert(refSegEvent);
    }

    return lyricInserted;
}

bool
SegmentLinker::eraseNonIgnored(Segment *s, Segment::const_iterator itrFrom,
                                           Segment::const_iterator itrTo,
                                           bool lyricsAlreadyErased)
{
    bool lyricErased = lyricsAlreadyErased;

    //only erase items which aren't ignored for link purposes
    Segment::iterator eraseItr;
    for(eraseItr=itrFrom; eraseItr!=s->end() && eraseItr!=itrTo; ) {
        bool ignore = false;
        (*eraseItr)->get<Bool>(BaseProperties::LINKED_SEGMENT_IGNORE_UPDATE,
                                ignore);
        if (!ignore) {

            if (! lyricErased) {
                // Is the erased event a lyric?
                Event *e = *eraseItr;
                if (e->isa(Text::EventType)) {
                    std::string textType;
                    lyricErased =
                        e->get<String>(Text::TextTypePropertyName, textType)
                        && (textType == Text::Lyric);
                }
            }

            s->erase(eraseItr++);
        } else {
            ++eraseItr;
        }
    }
    return lyricErased;
}

void
SegmentLinker::clearRefreshStatuses()
{
    LinkedSegmentParamsList::iterator itr;
    for (itr = m_linkedSegmentParamsList.begin();
        itr!= m_linkedSegmentParamsList.end(); ++itr) {

        LinkedSegmentParams &linkedSegParams = *itr;
        Segment *linkedSegToUpdate = linkedSegParams.m_linkedSegment;
        uint refreshStatusId = linkedSegParams.m_refreshStatusId;
        SegmentRefreshStatus &rs =
                        linkedSegToUpdate->getRefreshStatus(refreshStatusId);
        rs.setNeedsRefresh(false);
    }
}

void
SegmentLinker::refreshSegment(Segment *seg)
{
    timeT startTime = seg->getStartTime();
    eraseNonIgnored(seg, seg->begin(), seg->end(), true);
    // Last parameter set to true to avoid an useless search for lyrics

    //find another segment
    Segment *sourceSeg = nullptr;
    Segment *tempClone = nullptr;

    LinkedSegmentParamsList::iterator itr;
    for (itr = m_linkedSegmentParamsList.begin();
        itr!= m_linkedSegmentParamsList.end(); ++itr) {

        LinkedSegmentParams &linkedSegParams = *itr;
        Segment *other = linkedSegParams.m_linkedSegment;
        if (other != seg) {
            sourceSeg = other;
            break;
        }
    }

    if (!sourceSeg) {
        //make a temporary clone
        tempClone = createLinkedSegment(seg);
        sourceSeg = tempClone;
    }

    timeT sourceSegStartTime = sourceSeg->getStartTime();
    Segment::const_iterator segitr;
    for(segitr=sourceSeg->begin(); segitr!=sourceSeg->end(); ++segitr) {
        const Event *refEvent = *segitr;

        timeT refEventTime = refEvent->getAbsoluteTime() - sourceSegStartTime;
        timeT freshEventTime = refEventTime + startTime;
        timeT refEventNotationTime = refEvent->getNotationAbsoluteTime() - sourceSegStartTime;
        timeT freshEventNotationTime = refEventNotationTime + startTime;

        insertMappedEvent(seg, refEvent, freshEventTime, freshEventNotationTime,
                          seg->getLinkTransposeParams().m_semitones,
                          seg->getLinkTransposeParams().m_steps,
                          true);
        // Last parameter set to true to avoid an useless search for lyrics
    }

    if (tempClone) {
        delete tempClone;
    }
}

int
SegmentLinker::getNumberOfTmpSegments() const
{
    int count = 0;

    LinkedSegmentParamsList::const_iterator it;
    for (it = m_linkedSegmentParamsList.begin();
             it != m_linkedSegmentParamsList.end(); ++it) {
        if ((*it).m_linkedSegment->isTmp()) ++count;
    }

    return count;
}

int
SegmentLinker::getNumberOfOutOfCompSegments() const
{
    int count = 0;

    LinkedSegmentParamsList::const_iterator it;
    for (it = m_linkedSegmentParamsList.begin();
             it != m_linkedSegmentParamsList.end(); ++it) {
        if ((*it).m_linkedSegment->isTmp()) continue;  // Ignore tmp segs
        if (!(*it).m_linkedSegment->getComposition()) count++;
    }

    return count;
}

SegmentLinker::LinkedSegmentParams::LinkedSegmentParams(Segment *s) :
    m_linkedSegment(s),
    m_refreshStatusId(s->getNewRefreshStatusId())
{

}

void
SegmentLinker::setExcludeFromPrinting(bool exclude)
{
    for (LinkedSegmentParamsList::iterator i = m_linkedSegmentParamsList.begin();
            i != m_linkedSegmentParamsList.end(); ++i) {
        // Calling setExcludeFromPrinting() here with linkedSegmentsAlso == true
        // (which is the default) leads to infinite recursion.
        i->m_linkedSegment->setExcludeFromPrinting(exclude, false);
    }
}


}
