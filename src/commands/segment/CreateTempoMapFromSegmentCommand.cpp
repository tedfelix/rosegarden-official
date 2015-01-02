/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[CreateTempoMapFromSegmentCommand]"

#include "CreateTempoMapFromSegmentCommand.h"

#include "misc/Debug.h"
#include "base/Composition.h"
#include "base/NotationTypes.h"
#include "base/RealTime.h"
#include "base/Segment.h"


namespace Rosegarden
{

CreateTempoMapFromSegmentCommand::CreateTempoMapFromSegmentCommand(Segment *groove) :
        NamedCommand(tr("Set Tempos from Beat Segment")),
        m_composition(groove->getComposition())
{
    initialise(groove);
}

CreateTempoMapFromSegmentCommand::~CreateTempoMapFromSegmentCommand()
{
    // nothing
}

void
CreateTempoMapFromSegmentCommand::execute()
{
    for (TempoMap::iterator i = m_oldTempi.begin(); i != m_oldTempi.end(); ++i) {
        int n = m_composition->getTempoChangeNumberAt(i->first);
        if (n < m_composition->getTempoChangeCount()) {
            m_composition->removeTempoChange(n);
        }
    }

    for (TempoMap::iterator i = m_newTempi.begin(); i != m_newTempi.end(); ++i) {
        m_composition->addTempoAtTime(i->first, i->second);
    }
}

void
CreateTempoMapFromSegmentCommand::unexecute()
{
    for (TempoMap::iterator i = m_newTempi.begin(); i != m_newTempi.end(); ++i) {
        int n = m_composition->getTempoChangeNumberAt(i->first);
        if (n < m_composition->getTempoChangeCount()) {
            m_composition->removeTempoChange(n);
        }
    }

    for (TempoMap::iterator i = m_oldTempi.begin(); i != m_oldTempi.end(); ++i) {
        m_composition->addTempoAtTime(i->first, i->second);
    }
}

void
CreateTempoMapFromSegmentCommand::initialise(Segment *s)
{
    m_oldTempi.clear();
    m_newTempi.clear();

    //!!! need an additional option: per-chord, per-beat, per-bar.
    // Let's work per-beat for the moment.  Even for this, we should
    // probably use TimeSignature.getDivisions()

    std::vector<timeT> beatTimeTs;
    std::vector<RealTime> beatRealTimes;

    int startBar = m_composition->getBarNumber(s->getStartTime());
    int barNo = startBar;
    int beat = 0;

    for (Segment::iterator i = s->begin(); s->isBeforeEndMarker(i); ++i) {
        if ((*i)->isa(Note::EventType)) {

            bool isNew;
            TimeSignature sig =
                m_composition->getTimeSignatureInBar(barNo, isNew);

            beatTimeTs.push_back(m_composition->getBarStart(barNo) +
                                 beat * sig.getBeatDuration());

            if (++beat >= sig.getBeatsPerBar()) {
                ++barNo;
                beat = 0;
            }

            beatRealTimes.push_back(s->getComposition()->getElapsedRealTime
                                    ((*i)->getAbsoluteTime()));
        }
    }

    if (beatTimeTs.size() < 2)
        return ;

    tempoT prevTempo = 0;

    // set up m_oldTempi and prevTempo

    for (int i = m_composition->getTempoChangeNumberAt(*beatTimeTs.begin() - 1) + 1;
            i <= m_composition->getTempoChangeNumberAt(*beatTimeTs.end() - 1); ++i) {

        std::pair<timeT, tempoT> tempoChange =
            m_composition->getTempoChange(i);
        m_oldTempi[tempoChange.first] = tempoChange.second;
        if (prevTempo == 0)
            prevTempo = tempoChange.second;
    }

    RG_DEBUG << "starting tempo: " << prevTempo << endl;

    timeT quarter = Note(Note::Crotchet).getDuration();

    for (size_t beat = 1; beat < beatTimeTs.size(); ++beat) {

        timeT beatTime = beatTimeTs[beat] - beatTimeTs[beat - 1];
        RealTime beatRealTime = beatRealTimes[beat] - beatRealTimes[beat - 1];

        // Calculate tempo to nearest qpm.
        // This is 60 / {quarter note duration in seconds}
        // = 60 / ( {beat in seconds} * {quarter in ticks} / { beat in ticks} )
        // = ( 60 * {beat in ticks} ) / ( {beat in seconds} * {quarter in ticks} )
        // Precision is deliberately limited to qpm to avoid silly values.

        double beatSec = double(beatRealTime.sec) +
                         double(beatRealTime.usec() / 1000000.0);
        double qpm = (60.0 * beatTime) / (beatSec * quarter);
        tempoT tempo = Composition::getTempoForQpm(int(qpm + 0.001));

        RG_DEBUG << "prev beat: " << beatTimeTs[beat] << ", prev beat real time " << beatRealTimes[beat] << endl;
        RG_DEBUG << "time " << beatTime << ", rt " << beatRealTime << ", beatSec " << beatSec << ", tempo " << tempo << endl;

        if (tempo != prevTempo) {
            m_newTempi[beatTimeTs[beat - 1]] = tempo;
            prevTempo = tempo;
        }
    }

}

}
