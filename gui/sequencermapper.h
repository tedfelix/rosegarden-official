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

class SequencerMapper
{
public:
    SequencerMapper(const QString filename);
    ~SequencerMapper();

    bool remap();
    QString getFileName() const { return m_filename; }
    void* getBuffer() { return m_mmappedBuffer; }
    size_t getSize() const { return m_mmappedSize; }

protected:
    void map();
    void unmap();

    int          m_fd;
    size_t       m_mmappedSize;
    void*        m_mmappedBuffer;
    QString      m_filename;

};


#endif // _GUI_SEQUENCERMAPPER_H_
