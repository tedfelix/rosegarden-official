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

#ifndef _GUI_SEQUENCERMAPPER_H_
#define _GUI_SEQUENCERMAPPER_H_

#include "SequencerDataBlock.h"
#include "RealTime.h"

namespace Rosegarden { class MappedEvent; class MappedComposition; }

class SequencerMapper
{
public:
    SequencerMapper(const QString filename);
    ~SequencerMapper();

    Rosegarden::RealTime getPositionPointer() const {
	return m_sequencerDataBlock->getPositionPointer();
    }

    bool getVisual(Rosegarden::MappedEvent &ev) const {
	return m_sequencerDataBlock->getVisual(ev);
    }

    int getRecordedEvents(Rosegarden::MappedComposition &mC) const {
	return m_sequencerDataBlock->getRecordedEvents(mC);
    }

    bool getTrackLevel(Rosegarden::TrackId track,
		       Rosegarden::LevelInfo &info) const {
	return m_sequencerDataBlock->getTrackLevel(track, info);
    }

    bool getInstrumentLevel(Rosegarden::InstrumentId id,
			    Rosegarden::LevelInfo &info) const {
	return m_sequencerDataBlock->getInstrumentLevel(id, info);
    }

    bool getInstrumentLevelForMixer(Rosegarden::InstrumentId id,
			    Rosegarden::LevelInfo &info) const {
	return m_sequencerDataBlock->getInstrumentLevelForMixer(id, info);
    }

    bool getInstrumentRecordLevel(Rosegarden::InstrumentId id,
				  Rosegarden::LevelInfo &info) const {
	return m_sequencerDataBlock->getInstrumentRecordLevel(id, info);
    }

    bool getSubmasterLevel(int submaster,
			   Rosegarden::LevelInfo &info) const {
	return m_sequencerDataBlock->getSubmasterLevel(submaster, info);
    }

    bool getMasterLevel(Rosegarden::LevelInfo &info) const {
	return m_sequencerDataBlock->getMasterLevel(info);
    }

protected:
    void map();
    void unmap();

    int          m_fd;
    size_t       m_mmappedSize;
    void*        m_mmappedBuffer;
    QString      m_filename;
    Rosegarden::SequencerDataBlock *m_sequencerDataBlock;
};


#endif // _GUI_SEQUENCERMAPPER_H_
