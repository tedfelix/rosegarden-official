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

#include <vector>

#ifndef _LOCK_H_
#define _LOCK_H_

// Lock classes for synchornising over multiple threads.
//

namespace Rosegarden
{

class Lock
{
public:
    Lock();
    ~Lock();

    bool getWriteLock(int thread);
    bool releaseWriteLock(int thread);

    bool getReadLock(int thread);
    bool releaseReadLock(int thread);

protected:

    int              m_writeThread;
    std::vector<int> m_readThreads;

};

}

#endif // _LOCK_H_
 
