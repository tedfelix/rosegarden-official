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

class SequencerMmapper
{
public:
    SequencerMmapper();
    ~SequencerMmapper();
    
    QString getFileName() { return m_fileName; }
    void refresh();
    void updatePositionPointer(Rosegarden::RealTime time);

protected:

    void init();
    void setFileSize(size_t);
    QString createFileName();

    //--------------- Data members ---------------------------------
    //
    QString               m_fileName;
    bool                  m_needsRefresh;
    int                   m_fd;
    void*                 m_mmappedBuffer;
    size_t                m_mmappedSize;

    // Our position to map
    //
    Rosegarden::RealTime  m_position;
};


#endif // _SEQUENCERMAPPER_H_
