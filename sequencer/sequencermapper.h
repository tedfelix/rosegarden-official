// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-
/*
  Rosegarden-4
  A sequencer and musical notation editor.

  This program is Copyright 2000-2005
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
    
    void updatePositionPointer(Rosegarden::RealTime time) {
	m_sequencerDataBlock->setPositionPointer(time);
    }

    void updateVisual(Rosegarden::MappedEvent *ev) {
	m_sequencerDataBlock->setVisual(ev);
    }

    void updateRecordingBuffer(Rosegarden::MappedComposition *mC) {
	m_sequencerDataBlock->addRecordedEvents(mC);
    }

    void setTrackLevel(Rosegarden::TrackId track, const Rosegarden::LevelInfo &info) {
	m_sequencerDataBlock->setTrackLevel(track, info);
    }

    void setInstrumentLevel(Rosegarden::InstrumentId id,
			    const Rosegarden::LevelInfo &info) {
	m_sequencerDataBlock->setInstrumentLevel(id, info);
    }

    void setInstrumentRecordLevel(Rosegarden::InstrumentId id,
				  const Rosegarden::LevelInfo &info) {
	m_sequencerDataBlock->setInstrumentRecordLevel(id, info);
    }

    void setSubmasterLevel(int submaster,
			   const Rosegarden::LevelInfo &info) {
	m_sequencerDataBlock->setSubmasterLevel(submaster, info);
    }

    void setMasterLevel(const Rosegarden::LevelInfo &info) {
	m_sequencerDataBlock->setMasterLevel(info);
    }

    Rosegarden::SequencerDataBlock *getSequencerDataBlock() {
	return m_sequencerDataBlock;
    }
    void setControlBlock(Rosegarden::ControlBlock *cb) {
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
    Rosegarden::SequencerDataBlock *m_sequencerDataBlock;
};


#endif // _SEQUENCERMAPPER_H_
