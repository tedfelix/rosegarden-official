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

#define RG_MODULE_STRING "[SegmentAutoSplitCommand]"

#include "SegmentAutoSplitCommand.h"

#include "base/Event.h"
#include "misc/Debug.h"
#include "misc/AppendLabel.h"
#include "misc/Strings.h"
#include "base/Composition.h"
#include "base/NotationTypes.h"
#include "base/Segment.h"
#include <QString>


namespace Rosegarden
{

struct AutoSplitPoint
{
    timeT time;
    timeT lastSoundTime;
    Clef clef;
    Rosegarden::Key key;
    AutoSplitPoint(timeT t, timeT lst, Clef c, Rosegarden::Key k) :
	time(t), lastSoundTime(lst), clef(c), key(k) { }
};

SegmentAutoSplitCommand::SegmentAutoSplitCommand(Segment *segment) :
        NamedCommand(getGlobalName()),
        m_segment(segment),
        m_composition(segment->getComposition()),
        m_detached(false)
{}

SegmentAutoSplitCommand::~SegmentAutoSplitCommand()
{
    if (m_detached) {
        delete m_segment;
    } else {
        for (size_t i = 0; i < m_newSegments.size(); ++i) {
            delete m_newSegments[i];
        }
    }
}

void
SegmentAutoSplitCommand::execute()
{
    if (m_newSegments.empty()) {

        std::vector<AutoSplitPoint> splitPoints;

        Clef clef;
        Key key;
        timeT segmentStart = m_segment->getStartTime();
        timeT lastSoundTime = segmentStart;
        timeT lastSplitTime = segmentStart - 1;

        for (Segment::iterator i = m_segment->begin();
                m_segment->isBeforeEndMarker(i); ++i) {

            timeT myTime = (*i)->getAbsoluteTime();
            int barNo = m_composition->getBarNumber(myTime);

            if ((*i)->isa(Clef::EventType)) {
                clef = Clef(**i);
            } else if ((*i)->isa(Key::EventType)) {
                key = Key(**i);
            }

            if (myTime <= lastSplitTime)
                continue;

            bool newTimeSig = false;
            TimeSignature tsig =
                m_composition->getTimeSignatureInBar(barNo, newTimeSig);

            if (newTimeSig) {

                // If there's a new time sig in this bar and we haven't
                // already made a split in this bar, make one

                if (splitPoints.empty() ||
                        m_composition->getBarNumber
                        (splitPoints[splitPoints.size() - 1].time) < barNo) {

                    splitPoints.push_back(AutoSplitPoint(myTime, lastSoundTime,
                                                         clef, key));
                    lastSoundTime = lastSplitTime = myTime;
                }

            } else if ((*i)->isa(Note::EventRestType)) {

                // Otherwise never start a subsegment on a rest

                continue;

            } else {

                // When we meet a non-rest event, start a new split
                // if an entire bar has passed since the last non-rest event

                int lastSoundBarNo = m_composition->getBarNumber(lastSoundTime);

                if (lastSoundBarNo < barNo - 1 ||
                        (lastSoundBarNo == barNo - 1 &&
                         m_composition->getBarStartForTime(lastSoundTime) ==
                         lastSoundTime &&
                         lastSoundTime > segmentStart)) {

                    splitPoints.push_back
                    (AutoSplitPoint
                     (m_composition->getBarStartForTime(myTime), lastSoundTime,
                      clef, key));
                    lastSplitTime = myTime;
                }
            }

            lastSoundTime = std::max(lastSoundTime, myTime + (*i)->getDuration());
        }

        for (size_t split = 0; split <= splitPoints.size(); ++split) {

            Segment *newSegment = new Segment();
            newSegment->setTrack(m_segment->getTrack());
            std::string label = m_segment->getLabel();
            newSegment->setLabel(appendLabel(label, qstrtostr(tr("(part %1)").arg(split + 1))));
            newSegment->setColourIndex(m_segment->getColourIndex());

            timeT startTime = segmentStart;
            if (split > 0) {

                RG_DEBUG << "Auto-split point " << split - 1 << ": time "
                << splitPoints[split - 1].time << ", lastSoundTime "
                << splitPoints[split - 1].lastSoundTime << endl;

                startTime = splitPoints[split - 1].time;
                newSegment->insert(splitPoints[split - 1].clef.getAsEvent(startTime));
                newSegment->insert(splitPoints[split - 1].key.getAsEvent(startTime));
            }

            Segment::iterator i = m_segment->findTime(startTime);

            // A segment has to contain at least one note to be a worthy
            // candidate for adding back into the composition
            bool haveNotes = false;

            while (m_segment->isBeforeEndMarker(i)) {
                timeT t = (*i)->getAbsoluteTime();
                if (split < splitPoints.size() &&
                        t >= splitPoints[split].lastSoundTime)
                    break;
                if ((*i)->isa(Note::EventType))
                    haveNotes = true;
                newSegment->insert(new Event(**i));
                ++i;
            }

            if (haveNotes)
                m_newSegments.push_back(newSegment);
            else
                delete newSegment;
        }
    }

    m_composition->detachSegment(m_segment);
    for (size_t i = 0; i < m_newSegments.size(); ++i) {
        m_composition->addSegment(m_newSegments[i]);
    }
    m_detached = true;
}

void
SegmentAutoSplitCommand::unexecute()
{
    for (size_t i = 0; i < m_newSegments.size(); ++i) {
        m_composition->detachSegment(m_newSegments[i]);
    }
    m_composition->addSegment(m_segment);
    m_detached = false;
}

}
