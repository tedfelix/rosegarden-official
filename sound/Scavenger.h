// -*- c-basic-offset: 4 -*-

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

#ifndef _SCAVENGER_H_
#define _SCAVENGER_H_

#include <vector>
#include <sys/time.h>

namespace Rosegarden
{

/**
 * A very simple class that facilitates running things like plugins
 * without locking, by collecting unwanted objects and deleting them
 * after a delay so as to be sure nobody's in the middle of using
 * them.  Requires scavenge() to be called regularly from a non-RT
 * thread.
 *
 * This is currently not at all suitable for large numbers of objects
 * -- it's just a quick hack for use with things like plugins.
 */

template <typename T>
class Scavenger
{
public:
    Scavenger(int sec = 2, int defaultObjectListSize = 200);

    /**
     * Call from an RT thread etc., to pass ownership of t to us.
     * Only one thread should be calling this on any given scavenger.
     */
    void claim(T *t);

    /**
     * Call from a non-RT thread.
     * Only one thread should be calling this on any given scavenger.
     */
    void scavenge();

protected:
    typedef std::pair<T *, int> ObjectTimePair;
    typedef std::vector<ObjectTimePair> ObjectTimeList;
    ObjectTimeList m_objects;
    int m_sec;

    unsigned int m_claimed;
    unsigned int m_scavenged;
};

/**
 * A wrapper to permit arrays to be scavenged.
 */

template <typename T>
class ScavengerArrayWrapper
{
public:
    ScavengerArrayWrapper(T *array) : m_array(array) { }
    ~ScavengerArrayWrapper() { delete[] m_array; }

private:
    T *m_array;
};


template <typename T>
Scavenger<T>::Scavenger(int sec, int defaultObjectListSize) :
    m_objects(ObjectTimeList(defaultObjectListSize)),
    m_sec(sec),
    m_claimed(0),
    m_scavenged(0)
{
}

template <typename T>
void
Scavenger<T>::claim(T *t)
{
    struct timeval tv;
    (void)gettimeofday(&tv, 0);
    int sec = tv.tv_sec;

    for (size_t i = 0; i < m_objects.size(); ++i) {
	ObjectTimePair &pair = m_objects[i];
	if (pair.first == 0) {
	    pair.second = sec;
	    pair.first = t;
	    ++m_claimed;
	    return;
	}
    }

    // Oh no -- run out of slots!  We'd better just delete something
    // and hope.  This is very bad news and could cause real trouble.

    for (size_t i = 0; i < m_objects.size(); ++i) {
	ObjectTimePair &pair = m_objects[i];
	if (pair.first != 0) {
	    T *ot = pair.first;
	    pair.first = 0;
	    pair.second = sec;
	    pair.first = t;
	    ++m_claimed;
	    ++m_scavenged;
	    delete ot;
	}
    }
}

template <typename T>
void
Scavenger<T>::scavenge()
{
    if (m_scavenged >= m_claimed) return;
    
    struct timeval tv;
    (void)gettimeofday(&tv, 0);
    int sec = tv.tv_sec;

    for (size_t i = 0; i < m_objects.size(); ++i) {
	ObjectTimePair &pair = m_objects[i];
	if (pair.first != 0 && pair.second + m_sec < sec) {
	    T *ot = pair.first;
	    pair.first = 0;
	    delete ot;
	    ++m_scavenged;
	}
    }
}

}

#endif
