
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _MMAPPEDCONTROLBLOCK_H_
#define _MMAPPEDCONTROLBLOCK_H_

#include <QString>

#include "sound/MappedEvent.h"
#include "sound/ControlBlock.h"

namespace Rosegarden
{

class MmappedControlBlock
{
public:
    MmappedControlBlock(QString fileName);
    ~MmappedControlBlock();
    
    QString getFileName() { return m_fileName; }

    // delegate ControlBlock's interface
    InstrumentId getInstrumentForTrack(unsigned int trackId) 
        { return m_controlBlock->getInstrumentForTrack(trackId); }
        
    InstrumentId getInstrumentForEvent(unsigned int dev, 
                                       unsigned int chan)
        { return m_controlBlock->getInstrumentForEvent(dev, chan); }

    bool isTrackMuted(unsigned int trackId)
        { return m_controlBlock->isTrackMuted(trackId); }
        
    bool isTrackArmed(unsigned int trackId)
        { return m_controlBlock->isTrackArmed(trackId); }

    InstrumentId getInstrumentForMetronome() 
        { return m_controlBlock->getInstrumentForMetronome(); }

    bool isMetronomeMuted() { return m_controlBlock->isMetronomeMuted(); }

    bool isSolo()  { return m_controlBlock->isSolo(); }

    bool isMidiRoutingEnabled() 
        { return m_controlBlock->isMidiRoutingEnabled(); }
        
    TrackId getSelectedTrack()
        { return m_controlBlock->getSelectedTrack(); }

    MidiFilter getThruFilter()
        { return m_controlBlock->getThruFilter(); }

    MidiFilter getRecordFilter()
        { return m_controlBlock->getRecordFilter(); }

    // for transfer to SequencerMmapper
    ControlBlock *getControlBlock()
        { return m_controlBlock; }

protected:

    //--------------- Data members ---------------------------------
    QString m_fileName;
    int m_fd;
    void* m_mmappedBuffer;
    size_t m_mmappedSize;
    ControlBlock* m_controlBlock;
};

}

#endif // _MMAPPEDCONTROLBLOCK_H_
