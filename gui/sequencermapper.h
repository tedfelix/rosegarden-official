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

#ifndef _GUI_SEQUENCERMAPPER_H_
#define _GUI_SEQUENCERMAPPER_H_

#include <qstring.h>
#include "RealTime.h"

namespace Rosegarden { class MappedEvent; class MappedComposition; }

class SequencerMapper
{
public:
    SequencerMapper(const QString filename);
    ~SequencerMapper();

    Rosegarden::RealTime getPositionPointer() const;
    bool getVisual(Rosegarden::MappedEvent &) const;
    int getRecordedEvents(Rosegarden::MappedComposition &) const;
    
    QString getFileName() const { return m_filename; }

protected:
    void map();
    void unmap();

    int          m_fd;
    size_t       m_mmappedSize;
    void*        m_mmappedBuffer;
    QString      m_filename;
    static const int m_recordingBufferSize;

    //!!! nasty -- move to another class with placement new a la ControlBlock.  plus ew ew, we're even duplicating consts with the same file in sequencer

    Rosegarden::RealTime    *m_positionPtrPtr;
    int                     *m_eventIndexPtr;
    bool                    *m_haveEventPtr;
    Rosegarden::MappedEvent *m_eventPtr;
    int                     *m_recordEventIndexPtr;
    Rosegarden::MappedEvent *m_recordEventBuffer;

};


#endif // _GUI_SEQUENCERMAPPER_H_
