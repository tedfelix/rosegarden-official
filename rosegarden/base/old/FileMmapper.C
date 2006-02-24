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


#include "FileMmapper.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>


FileMmapper::FileMmapper(std::string fileName, bool readOnly)
    : m_fileName(fileName),
      m_readOnly(readOnly),
      m_fd(-1),
      m_mmappedBuffer(0),
      m_mmappedSize(0)
{
    if (!readOnly)
        ::unlink(m_fileName.c_str());
    
    if (!openFile()) {
        throw Rosegarden::Exception("Couldn't open " + m_fileName);
    }
    
    setFileSize(m_mmappedSize);

    if (!mmapFile()) {
        throw Rosegarden::Exception("mmap failed" + strerror(errno));
    }
    
}

FileMmapper::~FileMmapper()
{
    unmap()
}

void FileMmapper::setFileSize(size_t)
{
}

bool FileMmapper::openFile()
{
    if (m_readOnly) {
        m_fd = ::open(m_fileName.c_str(), RO_OPEN_MODE);
    } else {
        m_fd = ::open(m_fileName.c_str(), RW_OPEN_MODE,
                      S_IRUSR|S_IWUSR);
    }
    
    return m_fd >= 0;
}

bool FileMmapper::mmapFile()
{
    if(m_readOnly) {
        m_mmappedBuffer = ::mmap(0, m_mmappedSize,
                                 RO_MMAP_MODE,
                                 MAP_SHARED, m_fd, 0);

    } else {
        
        //
        // mmap() file for writing
        //
        m_mmappedBuffer = ::mmap(0, m_mmappedSize,
                                 RW_MMAP_MODE,
                                 MAP_SHARED, m_fd, 0);
    }
    
    return m_mmappedBuffer != (void*)-1;
}

FileMmapper::unmap()
{
    ::munmap(m_mmappedBuffer, m_mmappedSize);
    ::close(m_fd);
    if (!readOnly)
        ::unlink(m_fileName.c_str());
}

static const int FileMmapper::RW_OPEN_MODE = O_RDWR|O_CREAT|O_TRUNC;
static const int FileMmapper::RO_OPEN_MODE = O_RDWR; // Why not O_RDONLY ?
// I think I tried that but it wouldn't work.

static const int FileMmapper::RW_MMAP_MODE = PROT_READ|PROT_WRITE;
static const int FileMmapper::RO_MMAP_MODE = PROT_READ;
