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

#ifndef _SEQUENCERMAPPER_H_
#define _SEQUENCERMAPPER_H_

#include <qstring.h>
#include "RealTime.h"

namespace Rosegarden { class MappedEvent; class MappedComposition; }

class SequencerMmapper
{
public:
    SequencerMmapper();
    ~SequencerMmapper();
    
    void updatePositionPointer(Rosegarden::RealTime time);
    void updateVisual(Rosegarden::MappedEvent *ev);
    void updateRecordingBuffer(Rosegarden::MappedComposition *mC);

    QString getFileName() { return m_fileName; }

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
    static const int      m_recordingBufferSize;

    //!!! nasty -- move to another class with placement new a la ControlBlock
    Rosegarden::RealTime    *m_positionPtrPtr;
    int                     *m_eventIndexPtr;
    bool                    *m_haveEventPtr;
    Rosegarden::MappedEvent *m_eventPtr;
    int                     *m_recordEventIndexPtr;
    Rosegarden::MappedEvent *m_recordEventBuffer;
};


#endif // _SEQUENCERMAPPER_H_
