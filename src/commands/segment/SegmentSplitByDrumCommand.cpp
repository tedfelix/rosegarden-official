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


#include "SegmentSplitByDrumCommand.h"

#include "base/BaseProperties.h"
#include "base/Sets.h"
#include "misc/AppendLabel.h"
#include "misc/Strings.h"
#include "base/Composition.h"
#include "base/Event.h"
#include "base/NotationTypes.h"
#include "base/NotationQuantizer.h"
#include "base/Segment.h"
#include "base/SegmentNotationHelper.h"

#include <QString>
#include <QtGlobal>

#include <vector>
#include <algorithm>
namespace Rosegarden
{

SegmentSplitByDrumCommand::SegmentSplitByDrumCommand(Segment *segment) :
        NamedCommand(tr("Split by Drum")),
        m_composition(segment->getComposition()),
        m_segment(segment),
        m_newSegment(0),
        m_executed(false)
{}

SegmentSplitByDrumCommand::~SegmentSplitByDrumCommand()
{
    if (m_executed) {
        delete m_segment;
    } else {
        delete m_newSegment; // needs to be container of pointers to segments
    }
}

void
SegmentSplitByDrumCommand::execute()
{
    if (!m_newSegmentA) {

        m_newSegmentA = new Segment;
        m_newSegmentB = new Segment;

        m_newSegmentA->setTrack(m_segment->getTrack());
        m_newSegmentA->setStartTime(m_segment->getStartTime());

        m_newSegmentB->setTrack(m_segment->getTrack());
        m_newSegmentB->setStartTime(m_segment->getStartTime());

        // This value persists between iterations of the loop, for
        // Ranging strategy.
        int splitDrum(m_splitDrum);
            
        for (Segment::iterator i = m_segment->begin();
                m_segment->isBeforeEndMarker(i); ++i) {

            if ((*i)->isa(Note::EventRestType)) continue;
            // just skip indications:
            if ((*i)->isa(Indication::EventType)) continue;

            if ((*i)->isa(Clef::EventType) &&
                    m_clefHandling != LeaveClefs)
                continue;

            if ((*i)->isa(Note::EventType)) {
                splitDrum = getSplitDrumAt(i);
                
                if ((*i)->has(BaseProperties::PITCH) &&
                        (*i)->get
                        <Int>(BaseProperties::PITCH) <
                        splitDrum) {
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

        //!!!   m_newSegmentA->fillWithRests(m_segment->getEndMarkerTime());
        //      m_newSegmentB->fillWithRests(m_segment->getEndMarkerTime());
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
    m_newSegmentA->setLabel(appendLabel(label, qstrtostr(tr("(upper)"))));
    m_newSegmentB->setLabel(appendLabel(label, qstrtostr(tr("(lower)"))));
    m_newSegmentA->setColourIndex(m_segment->getColourIndex());
    m_newSegmentB->setColourIndex(m_segment->getColourIndex());

    m_composition->detachSegment(m_segment);
    m_executed = true;
}

void
SegmentSplitByDrumCommand::unexecute()
{
    m_composition->addSegment(m_segment);
    m_composition->detachSegment(m_newSegmentA);
    m_composition->detachSegment(m_newSegmentB);
    m_executed = false;
}

int
SegmentSplitByDrumCommand::getNewRangingSplitDrum(Segment::iterator prevNote,
                                                    int lastSplitDrum,
                                                    std::vector<int>& c0p)
{
    typedef std::set<int> Drumes;
    typedef std::set<int>::iterator DrumItr;

    const Quantizer *quantizer(m_segment->getComposition()->getNotationQuantizer());

    int myHighest, myLowest;
    int prevHighest = 0, prevLowest = 0;
    bool havePrev = false;
    Drumes pitches;
    pitches.insert(c0p.begin(), c0p.end());

    myLowest = c0p[0];
    myHighest = c0p[c0p.size() - 1];

    if (prevNote != m_segment->end()) {

        havePrev = true;

        Chord c1(*m_segment, prevNote, quantizer);
        std::vector<int> c1p(c1.getDrumes());
        pitches.insert(c1p.begin(), c1p.end());

        prevLowest = c1p[0];
        prevHighest = c1p[c1p.size() - 1];
    }

    if (pitches.size() < 2)
        return lastSplitDrum;

    DrumItr pi = pitches.begin();
    int lowest(*pi);

    pi = pitches.end();
    --pi;
    int highest(*pi);

    if ((pitches.size() == 2 || highest - lowest <= 18) &&
        myHighest > lastSplitDrum &&
        myLowest < lastSplitDrum &&
        prevHighest > lastSplitDrum &&
        prevLowest < lastSplitDrum) {

        if (havePrev) {
            if ((myLowest > prevLowest && myHighest > prevHighest) ||
                (myLowest < prevLowest && myHighest < prevHighest)) {
                int avgDiff = ((myLowest - prevLowest) +
                               (myHighest - prevHighest)) / 2;
                if (avgDiff < -5)
                    avgDiff = -5;
                if (avgDiff > 5)
                    avgDiff = 5;
                return lastSplitDrum + avgDiff;
            }
        }

        return lastSplitDrum;
    }

    int middle = (highest - lowest) / 2 + lowest;

    while (lastSplitDrum > middle && lastSplitDrum > m_splitDrum - 12) {
        if (lastSplitDrum - lowest < 12)
            return lastSplitDrum;
        if (lastSplitDrum <= m_splitDrum - 12)
            return lastSplitDrum;
        --lastSplitDrum;
    }

    while (lastSplitDrum < middle && lastSplitDrum < m_splitDrum + 12) {
        if (highest - lastSplitDrum < 12)
            return lastSplitDrum;
        if (lastSplitDrum >= m_splitDrum + 12)
            return lastSplitDrum;
        ++lastSplitDrum;
    }

    return lastSplitDrum;
}

int
SegmentSplitByDrumCommand::getSplitDrumAt(Segment::iterator i)
{
    // Can handle ConstantDrum immediately.
    if (m_splitStrategy == ConstantDrum) { return m_splitDrum; }

    // when this algorithm appears to be working ok, we should be
    // able to make it much quicker

    const Quantizer *quantizer(m_segment->getComposition()->getNotationQuantizer());

    Chord c0(*m_segment, i, quantizer);
    // Drumes in the chord.
    std::vector<int> c0p(c0.getDrumes());

    // Can handle ChordToneOfInitialDrum early if tone index hasn't
    // been set.
    if ((m_splitStrategy == ChordToneOfInitialDrum) &&
        (m_toneIndex < 0)) {
        // Find tone index.
        typedef std::vector<int>::iterator iterator;
        int toneIndex = 0;
        for (iterator i = c0p.begin(); i != c0p.end(); ++i) {
            if ((*i) < m_splitDrum) { toneIndex++; }
        }
        m_toneIndex = toneIndex;
        // This time split-pitch will just be initial split-pitch, so
        // return that.
        return m_splitDrum;
    }
    
    // Order pitches lowest to highest
    sort(c0p.begin(), c0p.end());

    switch (m_splitStrategy) {
    case LowestTone:
        return c0p[0] + 1; 

        /* NOTREACHED */
    case HighestTone:
        return c0p.back() - 1;

        /* NOTREACHED */
    case ChordToneOfInitialDrum:
        Q_ASSERT(m_toneIndex >= 0);

        // Lower than the lowest tone (a pointless command but
        // shouldn't be an error)
        if (m_toneIndex == 0) { return c0p[0] - 1; }
        // Higher than the highest tone (slightly more reasonable)
        if (m_toneIndex == (int)c0p.size()) { return c0p.back() + 1; }
        // Use a pitch between the adjacent tones (the usual case)
        return (c0p[m_toneIndex - 1] + c0p[m_toneIndex])/2;

        /* NOTREACHED */
    case Ranging:
        m_splitDrum =
            getNewRangingSplitDrum(Segment::iterator(c0.getPreviousNote()),
                                    m_splitDrum,
                                    c0p);
        return m_splitDrum;
        /* NOTREACHED */
        // Shouldn't get here.
    case ConstantDrum:
    default:
        return 0;
    }
}
} // namespace Rosegarden
