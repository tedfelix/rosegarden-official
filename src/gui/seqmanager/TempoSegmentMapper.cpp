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


#include "TempoSegmentMapper.h"

#include "base/RealTime.h"
#include "document/RosegardenDocument.h"
#include "misc/Debug.h"
#include "sound/MappedEvent.h"

// #define DEBUG_TEMPO_SEGMENT_MAPPER 1

namespace Rosegarden
{

void TempoSegmentMapper::fillBuffer()
{
    resize(0);
    Composition& comp = m_doc->getComposition();
    bool wroteInitialTempo = false;

    for (int i = 0; i < comp.getTempoChangeCount(); ++i) {

        std::pair<timeT, tempoT> tempoChange = comp.getTempoChange(i);
        std::pair<bool, tempoT> rampTo = comp.getTempoRamping(i, false);
            
        RealTime eventTime = comp.getElapsedRealTime(tempoChange.first);

        // If we haven't written time zero's tempo yet...
        if (!wroteInitialTempo) {
            // check if we have now passed zero.  Since we will write
            // time zero's tempo in any case, our test here excludes
            // time zero.
            if (eventTime > RealTime::zeroTime) {
                // ...write time zero's tempo....
                mapTempoAtZero(comp);
                // ...which we won't do again.
                wroteInitialTempo = true;
                // Now write the tempo change we just found.
                mapATempo(eventTime, tempoChange.second, rampTo.first);
            } 
        } else {
            mapATempo(eventTime, tempoChange.second, rampTo.first);
        }
    }
    // If we never wrote time zero's tempo, do it now.
    if (!wroteInitialTempo) {
        mapTempoAtZero(comp);
    }
}

void
TempoSegmentMapper::
mapTempoAtZero(Composition& comp)
{
    int number = comp.getTempoChangeNumberAt(0);
    bool ramping;
    if (number >= 0) {
        std::pair<bool, tempoT> rampTo = comp.getTempoRamping(number, false);
        ramping = rampTo.first;
    } else {
        ramping = false;
    }
    tempoT initialTempo = comp.getTempoAtTime(0);
    mapATempo(RealTime::zeroTime, initialTempo, ramping);
}

void
TempoSegmentMapper::
mapATempo(RealTime eventTime, tempoT tempo, bool ramping)
{
    MappedEvent e;
    e.setType(MappedEvent::Tempo);
    e.setEventTime(eventTime);
    // Nasty hack -- we use the instrument ID to pass through the
    // raw tempo value, as it has the appropriate range (unlike
    // e.g. tempo1 + tempo2).  These events are not actually used
    // on the sequencer side yet, so this may change to something
    // nicer at some point.  MidiInserter however uses it.
    e.setInstrument(tempo);

    // data1 holds whether we are ramping.  This is only used by the
    // MIDI exporter and it doesn't need to know where we're ramping
    // to.
    e.setData1(ramping ? 1 : 0);
#ifdef DEBUG_TEMPO_SEGMENT_MAPPER
    SEQUENCER_DEBUG
        << "TempoSegmentMapper::mapATempo inserting tempo"
        << tempo << "at"
        << eventTime
        << endl;
#endif

    mapAnEvent(&e);
}

int
TempoSegmentMapper::calculateSize()
{
    return m_doc->getComposition().getTempoChangeCount() + 1;
}

// Tempo changes always "play"
bool
TempoSegmentMapper::
shouldPlay(MappedEvent */*evt*/, RealTime /*startTime*/)
{ return true; }

}
