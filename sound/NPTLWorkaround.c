/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2004
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


/*
  
   This file exists solely to work around NPTL pthread.h, which
   declares the PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP macro using ISO
   C99 syntax that is not valid C++.

*/

#define _GNU_SOURCE 1
#include <pthread.h>

void initRecursiveMutex(pthread_mutex_t *mutex)
{
    pthread_mutex_t reference = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
    *mutex = reference;
}


/*

   That will be all.

*/

