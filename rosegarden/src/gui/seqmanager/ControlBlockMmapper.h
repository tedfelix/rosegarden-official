
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

#ifndef _RG_CONTROLBLOCKMMAPPER_H_
#define _RG_CONTROLBLOCKMMAPPER_H_

#include "base/MidiProgram.h"
#include "base/Track.h"
#include <qstring.h>




namespace Rosegarden
{

class Track;
class RosegardenGUIDoc;
class ControlBlock;


class ControlBlockMmapper
{
public:
    ControlBlockMmapper(RosegardenGUIDoc*);
    ~ControlBlockMmapper();
    
    QString getFileName() { return m_fileName; }
    void updateTrackData(Track*);
    void setTrackDeleted(TrackId);
    void updateMetronomeData(InstrumentId instId);
    void updateMetronomeForPlayback();
    void updateMetronomeForRecord();
    bool updateSoloData(bool solo, TrackId selectedTrack);
    void updateMidiFilters(MidiFilter thruFilter,
                           MidiFilter recordFilter);
    void setDocument(RosegardenGUIDoc*);
    void enableMIDIThruRouting(bool state);

protected:
    void initControlBlock();
    void setFileSize(size_t);
    QString createFileName();

    //--------------- Data members ---------------------------------
    RosegardenGUIDoc* m_doc;
    QString m_fileName;
    int m_fd;
    void* m_mmappedBuffer;
    size_t m_mmappedSize;
    ControlBlock* m_controlBlock;
};


//----------------------------------------



}

#endif
