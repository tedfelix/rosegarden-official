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

// A locking mechanism
//
//


#include "Lock.h"

namespace Rosegarden
{

Lock::Lock():
    m_writeThread(-1)
{
}

Lock::~Lock()
{
}

bool
Lock::getWriteLock(int thread)
{
    if (m_writeThread == -1)
    {
        m_writeThread = thread;
        return true;
    }

    return false;
}

bool
Lock::releaseWriteLock(int thread)
{
    if (m_writeThread == thread)
    {
        m_writeThread = -1;
        return true;
    }

    return false;
}

bool
Lock::getReadLock(int thread)
{
    // If we're writing then don't allow a read-lock
    //
    if (m_writeThread != -1)
        return false;

    std::vector<int>::iterator it = m_readThreads.begin();
    for (; it != m_readThreads.end(); it++)
    {
        if ((*it) == thread)
            return true;
    }

    m_readThreads.push_back(thread);
    return true;
}

bool
Lock::releaseReadLock(int thread)
{
    std::vector<int>::iterator it = m_readThreads.begin();
    for (; it != m_readThreads.end(); it++)
    {
        if ((*it) == thread)
        {
            m_readThreads.erase(it);
            return true;
        }
    }

    return false;
}

}

