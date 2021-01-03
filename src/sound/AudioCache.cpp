/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.
    See the AUTHORS file for more details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[AudioCache]"

#include "AudioCache.h"
#include "misc/Debug.h"

//#define DEBUG_AUDIO_CACHE 1

namespace Rosegarden
{

AudioCache::~AudioCache()
{
    clear();
}

bool
AudioCache::has(void *index)
{
    return m_cache.find(index) != m_cache.end();
}

float **
AudioCache::getData(void *index, size_t &channels, size_t &frames)
{
    if (m_cache.find(index) == m_cache.end())
        return nullptr;
    CacheRec *rec = m_cache[index];
    channels = rec->channels;
    frames = rec->nframes;
    return rec->data;
}

void
AudioCache::addData(void *index, size_t channels, size_t nframes, float **data)
{
#ifdef DEBUG_AUDIO_CACHE
    RG_DEBUG << "AudioCache::addData(" << index << ")";
#endif

    if (m_cache.find(index) != m_cache.end()) {
        RG_WARNING << "WARNING: AudioCache::addData(" << index << ", "
        << channels << ", " << nframes
        << ": already cached";
        return ;
    }

    m_cache[index] = new CacheRec(data, channels, nframes);
}

void
AudioCache::incrementReference(void *index)
{
    if (m_cache.find(index) == m_cache.end()) {
        RG_WARNING << "WARNING: AudioCache::incrementReference(" << index
        << "): not found";
        return ;
    }
    ++m_cache[index]->refCount;

#ifdef DEBUG_AUDIO_CACHE
    RG_DEBUG << "AudioCache::incrementReference(" << index << ") [to " << (m_cache[index]->refCount) << "]";
#endif
}

void
AudioCache::decrementReference(void *index)
{
    std::map<void *, CacheRec *>::iterator i = m_cache.find(index);

    if (i == m_cache.end()) {
        RG_WARNING << "WARNING: AudioCache::decrementReference(" << index
        << "): not found";
        return ;
    }
    if (i->second->refCount <= 1) {
        delete i->second;
        m_cache.erase(i);
#ifdef DEBUG_AUDIO_CACHE
        RG_DEBUG << "AudioCache::decrementReference(" << index << ") [deleting]";
#endif

    } else {
        --i->second->refCount;
#ifdef DEBUG_AUDIO_CACHE
        RG_DEBUG << "AudioCache::decrementReference(" << index << ") [to " << (m_cache[index]->refCount) << "]";
#endif

    }
}

void
AudioCache::clear()
{
#ifdef DEBUG_AUDIO_CACHE
    RG_DEBUG << "AudioCache::clear()";
#endif

    for (std::map<void *, CacheRec *>::iterator i = m_cache.begin();
            i != m_cache.end(); ++i) {
        if (i->second->refCount > 0) {
            RG_WARNING << "WARNING: AudioCache::clear: deleting cached data with refCount " << i->second->refCount;
        }
    }
    m_cache.clear();
}

AudioCache::CacheRec::~CacheRec()
{
    for (size_t j = 0; j < channels; ++j)
        delete[] data[j];
    delete[] data;
}

}


