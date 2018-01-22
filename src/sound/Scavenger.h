/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_SCAVENGER_H
#define RG_SCAVENGER_H

#include <vector>
#include <list>
#include <sys/time.h>
#include <pthread.h>
#include <iostream>

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
    ~Scavenger();

    /**
     * Call from an RT thread etc., to pass ownership of t to us for
     * later disposal.  Only one thread should be calling this on any
     * given scavenger.
     * 
     * This is only lock-free so long as a slot is available in the
     * object list; otherwise it takes a lock and allocates memory.
     * Scavengers should always be used with an object list size
     * sufficient to ensure that enough slots are always available in
     * normal use.
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

    typedef std::list<T *> ObjectList;
    ObjectList m_excess;
    int m_lastExcess;
    pthread_mutex_t m_excessMutex;
    void pushExcess(T *);
    void clearExcess(int);

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
    m_lastExcess(0),
    m_claimed(0),
    m_scavenged(0)
{
    pthread_mutex_init(&m_excessMutex, NULL);
}

template <typename T>
Scavenger<T>::~Scavenger()
{
    if (m_scavenged < m_claimed) {
        for (size_t i = 0; i < m_objects.size(); ++i) {
            ObjectTimePair &pair = m_objects[i];
            if (pair.first != 0) {
                T *ot = pair.first;
                pair.first = 0;
                delete ot;
                ++m_scavenged;
            }
        }
    }

    clearExcess(0);

    pthread_mutex_destroy(&m_excessMutex);
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

    std::cerr << "WARNING: Scavenger::claim(" << t << "): run out of slots, "
              << "using non-RT-safe method" << std::endl;
    pushExcess(t);
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

    if (sec > m_lastExcess + m_sec) {
        clearExcess(sec);
    }
}

template <typename T>
void
Scavenger<T>::pushExcess(T *t)
{
    pthread_mutex_lock(&m_excessMutex);
    m_excess.push_back(t);
    struct timeval tv;
    (void)gettimeofday(&tv, 0);
    m_lastExcess = tv.tv_sec;
    pthread_mutex_unlock(&m_excessMutex);
}

template <typename T>
void
Scavenger<T>::clearExcess(int sec)
{
    pthread_mutex_lock(&m_excessMutex);
    for (typename ObjectList::iterator i = m_excess.begin();
         i != m_excess.end(); ++i) {
        delete *i;
    }
    m_excess.clear();
    m_lastExcess = sec;
    pthread_mutex_unlock(&m_excessMutex);
}

}

#endif
