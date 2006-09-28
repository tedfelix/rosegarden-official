// -*- c-basic-offset: 4 -*-


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

#ifndef _FILE_MMAPER_H_
#define _FILE_MMAPER_H_

#include <string>

class FileMmapper
{
 public:
    FileMmapper(std::string fileName, bool readOnly);
    virtual ~FileMmapper();
    
    virtual void doMmap();
    
 protected:
    virtual void setFileSize(size_t);
    virtual void computeMmapSize() = 0;

    //--------------- Data members ---------------------------------
    std::string m_fileName;
    bool m_readOnly;
    int m_openMode;
    int m_fd;
    void* m_mmappedBuffer;
    size_t m_mmappedSize;

    static const int RW_OPEN_MODE;
    static const int RO_OPEN_MODE;

    static const int RW_MMAP_MODE;
    static const int RO_MMAP_MODE;
    
}


#endif
