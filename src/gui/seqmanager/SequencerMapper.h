
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2008
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

#ifndef _RG_SEQUENCERMAPPER_H_
#define _RG_SEQUENCERMAPPER_H_

#include "base/MidiProgram.h"
#include "base/RealTime.h"
#include "base/Track.h"
#include "sound/SequencerDataBlock.h"
#include <qstring.h>


class LevelInfo;


namespace Rosegarden
{

class MappedEvent;
class MappedComposition;


class SequencerMapper
{
public:
    SequencerMapper(const QString filename);
    ~SequencerMapper();

    RealTime getPositionPointer() const {
        return m_sequencerDataBlock->getPositionPointer();
    }

    bool getVisual(MappedEvent &ev) const {
        return m_sequencerDataBlock->getVisual(ev);
    }

    int getRecordedEvents(MappedComposition &mC) const {
        return m_sequencerDataBlock->getRecordedEvents(mC);
    }

    bool getTrackLevel(TrackId track,
                       LevelInfo &info) const {
        return m_sequencerDataBlock->getTrackLevel(track, info);
    }

    bool getInstrumentLevel(InstrumentId id,
                            LevelInfo &info) const {
        return m_sequencerDataBlock->getInstrumentLevel(id, info);
    }

    bool getInstrumentLevelForMixer(InstrumentId id,
                            LevelInfo &info) const {
        return m_sequencerDataBlock->getInstrumentLevelForMixer(id, info);
    }

    bool getInstrumentRecordLevel(InstrumentId id,
                                  LevelInfo &info) const {
        return m_sequencerDataBlock->getInstrumentRecordLevel(id, info);
    }

    bool getInstrumentRecordLevelForMixer(InstrumentId id,
                                          LevelInfo &info) const {
        return m_sequencerDataBlock->getInstrumentRecordLevelForMixer(id, info);
    }

    bool getSubmasterLevel(int submaster,
                           LevelInfo &info) const {
        return m_sequencerDataBlock->getSubmasterLevel(submaster, info);
    }

    bool getMasterLevel(LevelInfo &info) const {
        return m_sequencerDataBlock->getMasterLevel(info);
    }

protected:
    void map();
    void unmap();

    int          m_fd;
    size_t       m_mmappedSize;
    void*        m_mmappedBuffer;
    QString      m_filename;
    SequencerDataBlock *m_sequencerDataBlock;
};



}

#endif
