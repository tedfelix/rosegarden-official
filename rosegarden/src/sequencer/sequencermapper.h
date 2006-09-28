// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-
/*
  Rosegarden-4
  A sequencer and musical notation editor.

  This program is Copyright 2000-2006
  Guillaume Laurent   <glaurent@telegraph-road.org>,
  Chris Cannam        <cannam@all-day-breakfast.com>,
  Richard Bown        <bownie@bownie.com>

  The moral right of the authors to claim authorship of this work
  has been asserted.

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of the
  License, or (at your option) any later version.  See the file
  COPYING included with this distribution for more information.
*/

#ifndef _SEQUENCERMAPPER_H_
#define _SEQUENCERMAPPER_H_

#include "SequencerDataBlock.h"
#include "RealTime.h"

namespace Rosegarden { class MappedEvent; class MappedComposition; }

class SequencerMmapper
{
public:
    SequencerMmapper();
    ~SequencerMmapper();
    
    void updatePositionPointer(RealTime time) {
	m_sequencerDataBlock->setPositionPointer(time);
    }

    void updateVisual(MappedEvent *ev) {
	m_sequencerDataBlock->setVisual(ev);
    }

    void updateRecordingBuffer(MappedComposition *mC) {
	m_sequencerDataBlock->addRecordedEvents(mC);
    }

    void setTrackLevel(TrackId track, const LevelInfo &info) {
	m_sequencerDataBlock->setTrackLevel(track, info);
    }

    void setInstrumentLevel(InstrumentId id,
			    const LevelInfo &info) {
	m_sequencerDataBlock->setInstrumentLevel(id, info);
    }

    void setInstrumentRecordLevel(InstrumentId id,
				  const LevelInfo &info) {
	m_sequencerDataBlock->setInstrumentRecordLevel(id, info);
    }

    void setSubmasterLevel(int submaster,
			   const LevelInfo &info) {
	m_sequencerDataBlock->setSubmasterLevel(submaster, info);
    }

    void setMasterLevel(const LevelInfo &info) {
	m_sequencerDataBlock->setMasterLevel(info);
    }

    SequencerDataBlock *getSequencerDataBlock() {
	return m_sequencerDataBlock;
    }
    void setControlBlock(ControlBlock *cb) {
	m_sequencerDataBlock->setControlBlock(cb);
    }

protected:
    void init();
    void setFileSize(size_t);
    QString createFileName();

    //--------------- Data members ---------------------------------
    //
    QString               m_fileName;
    int                   m_fd;
    void*                 m_mmappedBuffer;
    size_t                m_mmappedSize;
    SequencerDataBlock *m_sequencerDataBlock;
};


#endif // _SEQUENCERMAPPER_H_
