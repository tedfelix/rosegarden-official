/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2007
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>
 
    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "SegmentSplitByPitchCommand.h"

#include "base/BaseProperties.h"
#include "base/Sets.h"
#include "misc/Strings.h"
#include "base/Composition.h"
#include "base/Event.h"
#include "base/NotationTypes.h"
#include "base/NotationQuantizer.h"
#include "base/Segment.h"
#include "base/SegmentNotationHelper.h"
#include <qstring.h>


namespace Rosegarden
{

SegmentSplitByPitchCommand::SegmentSplitByPitchCommand(Segment *segment,
        int p, bool r, bool d,
        ClefHandling c) :
        KNamedCommand(i18n("Split by Pitch")),
        m_composition(segment->getComposition()),
        m_segment(segment),
        m_newSegmentA(0),
        m_newSegmentB(0),
        m_splitPitch(p),
        m_ranging(r),
        m_dupNonNoteEvents(d),
        m_clefHandling(c),
        m_executed(false)
{}

SegmentSplitByPitchCommand::~SegmentSplitByPitchCommand()
{
    if (m_executed) {
        delete m_segment;
    } else {
        delete m_newSegmentA;
        delete m_newSegmentB;
    }
}

void
SegmentSplitByPitchCommand::execute()
{
    if (!m_newSegmentA) {

        m_newSegmentA = new Segment;
        m_newSegmentB = new Segment;

        m_newSegmentA->setTrack(m_segment->getTrack());
        m_newSegmentA->setStartTime(m_segment->getStartTime());

        m_newSegmentB->setTrack(m_segment->getTrack());
        m_newSegmentB->setStartTime(m_segment->getStartTime());

        int splitPitch(m_splitPitch);

        for (Segment::iterator i = m_segment->begin();
                m_segment->isBeforeEndMarker(i); ++i) {

            if ((*i)->isa(Note::EventRestType))
                continue;
            if ((*i)->isa(Clef::EventType) &&
                    m_clefHandling != LeaveClefs)
                continue;

            if ((*i)->isa(Note::EventType)) {

                if (m_ranging) {
                    splitPitch = getSplitPitchAt(i, splitPitch);
                }

                if ((*i)->has(BaseProperties::PITCH) &&
                        (*i)->get
                        <Int>(BaseProperties::PITCH) <
                        splitPitch) {
                    if (m_newSegmentB->empty()) {
                        m_newSegmentB->fillWithRests((*i)->getAbsoluteTime());
                    }
                    m_newSegmentB->insert(new Event(**i));
                }
                else {
                    if (m_newSegmentA->empty()) {
                        m_newSegmentA->fillWithRests((*i)->getAbsoluteTime());
                    }
                    m_newSegmentA->insert(new Event(**i));
                }

            } else {

                m_newSegmentA->insert(new Event(**i));

                if (m_dupNonNoteEvents) {
                    m_newSegmentB->insert(new Event(**i));
                }
            }
        }

        //!!!	m_newSegmentA->fillWithRests(m_segment->getEndMarkerTime());
        //	m_newSegmentB->fillWithRests(m_segment->getEndMarkerTime());
        m_newSegmentA->normalizeRests(m_segment->getStartTime(),
                                      m_segment->getEndMarkerTime());
        m_newSegmentB->normalizeRests(m_segment->getStartTime(),
                                      m_segment->getEndMarkerTime());
    }

    m_composition->addSegment(m_newSegmentA);
    m_composition->addSegment(m_newSegmentB);

    SegmentNotationHelper helperA(*m_newSegmentA);
    SegmentNotationHelper helperB(*m_newSegmentB);

    if (m_clefHandling == RecalculateClefs) {

        m_newSegmentA->insert
        (helperA.guessClef(m_newSegmentA->begin(),
                           m_newSegmentA->end()).getAsEvent
         (m_newSegmentA->getStartTime()));

        m_newSegmentB->insert
        (helperB.guessClef(m_newSegmentB->begin(),
                           m_newSegmentB->end()).getAsEvent
         (m_newSegmentB->getStartTime()));

    } else if (m_clefHandling == UseTrebleAndBassClefs) {

        m_newSegmentA->insert
        (Clef(Clef::Treble).getAsEvent
         (m_newSegmentA->getStartTime()));

        m_newSegmentB->insert
        (Clef(Clef::Bass).getAsEvent
         (m_newSegmentB->getStartTime()));
    }

    //!!!    m_composition->getNotationQuantizer()->quantize(m_newSegmentA);
    //    m_composition->getNotationQuantizer()->quantize(m_newSegmentB);
    helperA.autoBeam(m_newSegmentA->begin(), m_newSegmentA->end(),
                     BaseProperties::GROUP_TYPE_BEAMED);
    helperB.autoBeam(m_newSegmentB->begin(), m_newSegmentB->end(),
                     BaseProperties::GROUP_TYPE_BEAMED);

    std::string label = m_segment->getLabel();
    m_newSegmentA->setLabel(qstrtostr(i18n("%1 (upper)").arg
                                      (strtoqstr(label))));
    m_newSegmentB->setLabel(qstrtostr(i18n("%1 (lower)").arg
                                      (strtoqstr(label))));
    m_newSegmentA->setColourIndex(m_segment->getColourIndex());
    m_newSegmentB->setColourIndex(m_segment->getColourIndex());

    m_composition->detachSegment(m_segment);
    m_executed = true;
}

void
SegmentSplitByPitchCommand::unexecute()
{
    m_composition->addSegment(m_segment);
    m_composition->detachSegment(m_newSegmentA);
    m_composition->detachSegment(m_newSegmentB);
    m_executed = false;
}

int
SegmentSplitByPitchCommand::getSplitPitchAt(Segment::iterator i,
        int lastSplitPitch)
{
    typedef std::set<int>::iterator PitchItr;
    std::set<int> pitches;

    // when this algorithm appears to be working ok, we should be
    // able to make it much quicker

    const Quantizer *quantizer
    (m_segment->getComposition()->getNotationQuantizer());

    int myHighest, myLowest;
    int prevHighest = 0, prevLowest = 0;
    bool havePrev = false;

    Chord c0(*m_segment, i, quantizer);
    std::vector<int> c0p(c0.getPitches());
    pitches.insert<std::vector<int>::iterator>(c0p.begin(), c0p.end());

    myLowest = c0p[0];
    myHighest = c0p[c0p.size() - 1];

    Segment::iterator j(c0.getPreviousNote());
    if (j != m_segment->end()) {

        havePrev = true;

        Chord c1(*m_segment, j, quantizer);
        std::vector<int> c1p(c1.getPitches());
        pitches.insert<std::vector<int>::iterator>(c1p.begin(), c1p.end());

        prevLowest = c1p[0];
        prevHighest = c1p[c1p.size() - 1];
    }

    if (pitches.size() < 2)
        return lastSplitPitch;

    PitchItr pi = pitches.begin();
    int lowest(*pi);

    pi = pitches.end();
    --pi;
    int highest(*pi);

    if ((pitches.size() == 2 || highest - lowest <= 18) &&
            myHighest > lastSplitPitch &&
            myLowest < lastSplitPitch &&
            prevHighest > lastSplitPitch &&
            prevLowest < lastSplitPitch) {

        if (havePrev) {
            if ((myLowest > prevLowest && myHighest > prevHighest) ||
                    (myLowest < prevLowest && myHighest < prevHighest)) {
                int avgDiff = ((myLowest - prevLowest) +
                               (myHighest - prevHighest)) / 2;
                if (avgDiff < -5)
                    avgDiff = -5;
                if (avgDiff > 5)
                    avgDiff = 5;
                return lastSplitPitch + avgDiff;
            }
        }

        return lastSplitPitch;
    }

    int middle = (highest - lowest) / 2 + lowest;

    while (lastSplitPitch > middle && lastSplitPitch > m_splitPitch - 12) {
        if (lastSplitPitch - lowest < 12)
            return lastSplitPitch;
        if (lastSplitPitch <= m_splitPitch - 12)
            return lastSplitPitch;
        --lastSplitPitch;
    }

    while (lastSplitPitch < middle && lastSplitPitch < m_splitPitch + 12) {
        if (highest - lastSplitPitch < 12)
            return lastSplitPitch;
        if (lastSplitPitch >= m_splitPitch + 12)
            return lastSplitPitch;
        ++lastSplitPitch;
    }

    return lastSplitPitch;
}

}
