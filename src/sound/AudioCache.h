/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_AUDIO_CACHE_H
#define RG_AUDIO_CACHE_H

#include <map>
#include <stddef.h>

namespace Rosegarden
{

/**
 * A simple cache for smallish bits of audio data, indexed by some
 * opaque pointer type.  (The PlayableAudioFile uses this with an
 * AudioFile* index type, for example.)  With reference counting.
 */

class AudioCache
{
public:
    AudioCache() { }
    virtual ~AudioCache();

    /**
     * Look some audio data up in the cache and report whether it
     * exists.
     */
    bool has(void *index);

    /**
     * Look some audio data up in the cache and return it if it
     * exists, returning the number of channels in channels, the frame
     * count in frames, and one pointer per channel to samples in the
     * return value.  Return 0 if the data is not in cache.  Does not
     * affect the reference count.  Ownership of the returned data
     * remains with the cache object.  You should not call this unless
     * you have already called incrementReference (or addData) to
     * register your interest.
     */
    float **getData(void *index, size_t &channels, size_t &frames);

    /**
     * Add a piece of data to the cache, and increment the reference
     * count for that data (to 1).  Ownership of the data is passed
     * to the cache, which will delete it with delete[] when done.
     */
    void addData(void *index, size_t channels, size_t nframes, float **data);

    /**
     * Increment the reference count for a given piece of data.
     */
    void incrementReference(void *index);

    /**
     * Decrement the reference count for a given piece of data,
     * and delete the data from the cache if the reference count has
     * reached zero.
     */
    void decrementReference(void *index);

protected:
    void clear();

    struct CacheRec {
        CacheRec() : data(0), channels(0), nframes(0), refCount(0) { }
        CacheRec(float **d, size_t c, size_t n) :
            data(d), channels(c), nframes(n), refCount(1) { }
        ~CacheRec();
        float **data;
        size_t channels;
        size_t nframes;
        int refCount;
    };

    std::map<void *, CacheRec *> m_cache;
};

}

#endif
