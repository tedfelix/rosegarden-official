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

#ifndef RG_METRONOMEMAPPER_H
#define RG_METRONOMEMAPPER_H

#include "base/Event.h"
#include "base/MidiProgram.h"
#include "base/RealTime.h"
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
    friend class SegmentMapperFactory;

 public:
    virtual ~MetronomeMapper();

    InstrumentId getMetronomeInstrument();

    // overrides from SegmentMapper
    virtual int getSegmentRepeatCount();

    // Do channel-setup
    virtual void doInsert(MappedInserterBase &inserter, MappedEvent &evt,
                         RealTime start, bool firstOutput);

    virtual void makeReady(MappedInserterBase &inserter, RealTime time);

    
    // Return whether the event should be played.
    virtual bool shouldPlay(MappedEvent *evt, RealTime startTime);
 protected:
    MetronomeMapper(RosegardenDocument *doc);

    virtual int calculateSize();

    void sortTicks();

    // override from SegmentMapper
    virtual void fillBuffer();

    // Whether the metronome is muted regarding this event.
    bool mutedEtc(MappedEvent *evt);
    
    //--------------- Data members ---------------------------------
    typedef std::pair<timeT, int> Tick;
    typedef std::vector<Tick> TickContainer;
    friend bool operator<(Tick, Tick);

    TickContainer m_ticks;
    bool m_deleteMetronome;
    const MidiMetronome* m_metronome;
    RealTime m_tickDuration;
    EternalChannelManager m_channelManager;
};

}

#endif
