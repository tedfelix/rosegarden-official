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

#ifndef _AUDIO_PLAY_QUEUE_H_
#define _AUDIO_PLAY_QUEUE_H_

#include "RealTime.h"
#include "Instrument.h"

#include <set>
#include <vector>
#include <map>
#include <list>

namespace Rosegarden
{

class PlayableAudioFile;

/**
 * An ordered list of PlayableAudioFiles that does not aim to be quick
 * to add to or remove files from, but that aims to quickly answer the
 * question of which files are playing within a given time slice.
 *
 * Note that there is no locking between audio file add/remove and
 * lookup.  Add/remove should only be carried out when it is known
 * that no threads will be performing lookup.
 */

class AudioPlayQueue
{
public:
    AudioPlayQueue();
    virtual ~AudioPlayQueue();

    struct FileTimeCmp {
	bool operator()(const PlayableAudioFile &, const PlayableAudioFile &) const;
	bool operator()(const PlayableAudioFile *, const PlayableAudioFile *) const;
    };
    typedef std::set<PlayableAudioFile *, FileTimeCmp> FileSet;

    /**
     * Add a file to the queue.  AudioPlayQueue takes ownership of the
     * file and will delete it when removed.
     */
    void addScheduled(PlayableAudioFile *file);

    /**
     * Add a file to the unscheduled list.  AudioPlayQueue takes
     * ownership of the file and will delete it when removed.
     * Unscheduled files will be returned along with schuled ones for
     * normal lookups, but everything will be less efficient when
     * there are unscheduled files on the queue.  This is intended
     * for asynchronous (preview) playback.
     */
    void addUnscheduled(PlayableAudioFile *file);

    /**
     * Remove a scheduled or unscheduled file from the queue and
     * delete it.
     */
    void erase(PlayableAudioFile *file);

    /**
     * Remove all files and delete them.
     */
    void clear();

    /**
     * Look up the files playing during a given slice and return them
     * in the passed FileSet.  The pointers returned are still owned
     * by me and the caller should not delete them.
     */ 
    void getPlayingFiles(const RealTime &sliceStart,
			 const RealTime &sliceDuration,
			 FileSet &) const;

    /**
     * Look up the files playing during a given slice on a given
     * instrument and return them in the passed FileSet.  The pointers
     * returned are still owned by me and the caller should not delete
     * them.
     */
    void getPlayingFilesForInstrument(const RealTime &sliceStart,
				      const RealTime &sliceDuration,
				      InstrumentId instrumentId,
				      FileSet &) const;

    /**
     * Return a (shared reference to an) ordered set of all files on
     * the scheduled queue.
     */
    const FileSet &getAllScheduledFiles() const;

    /**
     * Get an approximate (but always pessimistic) estimate of the
     * number of ring buffers required for the current queue -- that
     * is, the maximum possible number of audio channels playing at
     * once from non-small-file-cached-files.
     */
    size_t getMaxBuffersRequired() const { return m_maxBuffers; }

private:
    FileSet m_files;

    typedef std::vector<PlayableAudioFile *> FileVector;
    typedef std::map<int, FileVector> ReverseFileMap;
    ReverseFileMap m_index;

    typedef std::vector<ReverseFileMap> InstrumentReverseFileMap;
    InstrumentReverseFileMap m_instrumentIndex;

    typedef std::list<PlayableAudioFile *> FileList;
    FileList m_unscheduled;

    typedef std::map<int, size_t> FileCountMap;
    FileCountMap m_counts;

    size_t m_maxBuffers;
};


}

#endif

