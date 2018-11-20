/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_METRONOMEMAPPER_H
#define RG_METRONOMEMAPPER_H

#include "base/MidiProgram.h"  // For InstrumentId
#include "base/RealTime.h"
#include "base/TimeT.h"
#include "gui/seqmanager/ChannelManager.h"
#include "gui/seqmanager/MappedEventBuffer.h"

#include <QString>

#include <utility>
#include <vector>

namespace Rosegarden
{


class RosegardenDocument;
class MidiMetronome;


class MetronomeMapper : public MappedEventBuffer
{
public:
    MetronomeMapper(RosegardenDocument *doc);
    virtual ~MetronomeMapper();

    InstrumentId getMetronomeInstrument() const;


    // *** MappedEventBuffer overrides.

    int getSegmentRepeatCount() override;
    // Do channel-setup
    void doInsert(MappedInserterBase &inserter, MappedEvent &evt,
                          RealTime start, bool firstOutput) override;
    void makeReady(MappedInserterBase &inserter, RealTime time) override;
    /// Should the event be played?
    bool shouldPlay(MappedEvent *evt, RealTime startTime) override;
    int calculateSize() override;
    /// Convert m_ticks to events in m_buffer.
    void fillBuffer() override;

private:
    Instrument *m_instrument;

    enum TickType {
        BarTick = 0,
        BeatTick = 1,
        SubBeatTick = 2,
        MidiTimingClockTick = 3  // MIDI Spec, Section 2, Page 30.
    };
    typedef std::pair<timeT, TickType> Tick;
    typedef std::vector<Tick> TickContainer;
    /// The ticks of the metronome.
    TickContainer m_ticks;

    const MidiMetronome *m_metronome;

    ChannelManager m_channelManager;
};


}

#endif
