// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-
/*
  Rosegarden-4
  A sequencer and musical notation editor.

  This program is Copyright 2000-2003
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

#include <qstring.h>

#include "MappedEvent.h"
#include "ControlBlock.h"

#ifndef _MMAPPEDCONTROLBLOCK_H_
#define _MMAPPEDCONTROLBLOCK_H_


using std::cerr;
using std::endl;
using std::cout;
using Rosegarden::MappedEvent;

using Rosegarden::InstrumentId;
using Rosegarden::ControlBlock;

class ControlBlockMmapper
{
public:
    ControlBlockMmapper(QString fileName);
    ~ControlBlockMmapper();
    
    QString getFileName() { return m_fileName; }

    // should be called only when control block changes size,
    // i.e. not very often
    void refresh();

    // delegate ControlBlock's interface
    InstrumentId getInstrumentForTrack(unsigned int trackId) 
        { return m_controlBlock->getInstrumentForTrack(trackId); }

    bool isTrackMuted(unsigned int trackId)
        { return m_controlBlock->isTrackMuted(trackId); }

    InstrumentId getInstrumentForMetronome() 
        { return m_controlBlock->getInstrumentForMetronome(); }

    bool isMetronomeMuted() { return m_controlBlock->isMetronomeMuted(); }

    bool isSolo()  { return m_controlBlock->isSolo(); }

    Rosegarden::TrackId getSelectedTrack()
        { return m_controlBlock->getSelectedTrack(); }

    int getTransposeValue()
    { return m_controlBlock->getTransposeValue(); }
        
protected:

    //--------------- Data members ---------------------------------
    QString m_fileName;
    int m_fd;
    void* m_mmappedBuffer;
    size_t m_mmappedSize;
    ControlBlock* m_controlBlock;
};

#endif // _MMAPPEDCONTROLBLOCK_H_
