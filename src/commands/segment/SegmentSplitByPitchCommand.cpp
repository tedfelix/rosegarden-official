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


#include "SegmentSplitByPitchCommand.h"

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

SegmentSplitByPitchCommand::SegmentSplitByPitchCommand(Segment *segment,
        int p, SplitStrategy splitStrategy, bool d,
        ClefHandling c) :
        NamedCommand(tr("Split by Pitch")),
        m_composition(segment->getComposition()),
        m_segment(segment),
        m_newSegmentA(0),
        m_newSegmentB(0),
        m_splitPitch(p),
        m_splitStrategy(splitStrategy),
        // Ctor initializes m_toneIndex to an invalid value.  If using
        // ChordToneOfInitialPitch, we'll set it correctly the first
        // time thru.
        m_toneIndex(-1), 
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

        // This value persists between iterations of the loop, for
        // Ranging strategy.
        int splitPitch(m_splitPitch);
            
        for (Segment::iterator i = m_segment->begin();
                m_segment->isBeforeEndMarker(i); ++i) {

            if ((*i)->isa(Note::EventRestType)) continue;
            // just skip indications:
            if ((*i)->isa(Indication::EventType)) continue;

            if ((*i)->isa(Clef::EventType) &&
                    m_clefHandling != LeaveClefs)
                continue;

            if ((*i)->isa(Note::EventType)) {
                splitPitch = getSplitPitchAt(i);
                
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
    m_newSegmentA->setLabel(appendLabel(label, qstrtostr(tr("(upper)"))));
    m_newSegmentB->setLabel(appendLabel(label, qstrtostr(tr("(lower)"))));
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
SegmentSplitByPitchCommand::getNewRangingSplitPitch(Segment::iterator prevNote,
                                                    int lastSplitPitch,
                                                    std::vector<int>& c0p)
{
    typedef std::set<int> Pitches;
    typedef std::set<int>::iterator PitchItr;

    const Quantizer *quantizer(m_segment->getComposition()->getNotationQuantizer());

    int myHighest, myLowest;
    int prevHighest = 0, prevLowest = 0;
    bool havePrev = false;
    Pitches pitches;
    pitches.insert(c0p.begin(), c0p.end());

    myLowest = c0p[0];
    myHighest = c0p[c0p.size() - 1];

    if (prevNote != m_segment->end()) {

        havePrev = true;

        Chord c1(*m_segment, prevNote, quantizer);
        std::vector<int> c1p(c1.getPitches());
        pitches.insert(c1p.begin(), c1p.end());

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

int
SegmentSplitByPitchCommand::getSplitPitchAt(Segment::iterator i)
{
    // Can handle ConstantPitch immediately.
    if (m_splitStrategy == ConstantPitch) { return m_splitPitch; }

    // when this algorithm appears to be working ok, we should be
    // able to make it much quicker

    const Quantizer *quantizer(m_segment->getComposition()->getNotationQuantizer());

    Chord c0(*m_segment, i, quantizer);
    // Pitches in the chord.
    std::vector<int> c0p(c0.getPitches());

    // Can handle ChordToneOfInitialPitch early if tone index hasn't
    // been set.
    if ((m_splitStrategy == ChordToneOfInitialPitch) &&
        (m_toneIndex < 0)) {
        // Find tone index.
        typedef std::vector<int>::iterator iterator;
        int toneIndex = 0;
        for (iterator i = c0p.begin(); i != c0p.end(); ++i) {
            if ((*i) < m_splitPitch) { toneIndex++; }
        }
        m_toneIndex = toneIndex;
        // This time split-pitch will just be initial split-pitch, so
        // return that.
        return m_splitPitch;
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
    case ChordToneOfInitialPitch:
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
        m_splitPitch =
            getNewRangingSplitPitch(Segment::iterator(c0.getPreviousNote()),
                                    m_splitPitch,
                                    c0p);
        return m_splitPitch;
        /* NOTREACHED */
        // Shouldn't get here.
    case ConstantPitch:
    default:
        return 0;
    }
}
} // namespace Rosegarden
