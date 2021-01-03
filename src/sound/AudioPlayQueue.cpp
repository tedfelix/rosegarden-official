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

#define RG_MODULE_STRING "[AudioPlayQueue]"

#include "AudioPlayQueue.h"

#include "misc/Debug.h"
#include "PlayableAudioFile.h"
#include "base/Profiler.h"

//#define DEBUG_AUDIO_PLAY_QUEUE 1
//#define FINE_DEBUG_AUDIO_PLAY_QUEUE 1

namespace Rosegarden
{


static inline unsigned int instrumentId2Index(InstrumentId id)
{
    if (id < AudioInstrumentBase)
        return 0;
    else
        return (id - AudioInstrumentBase);
}

bool
AudioPlayQueue::FileTimeCmp::operator()(const PlayableAudioFile &f1,
                                        const PlayableAudioFile &f2) const
{
    return operator()(&f1, &f2);
}

bool
AudioPlayQueue::FileTimeCmp::operator()(const PlayableAudioFile *f1,
                                        const PlayableAudioFile *f2) const
{
    RealTime t1 = f1->getStartTime(), t2 = f2->getStartTime();
    if (t1 < t2)
        return true;
    else if (t2 < t1)
        return false;
    else
        return f1 < f2;
}


AudioPlayQueue::AudioPlayQueue() :
        m_maxBuffers(0)
{
    // nothing to do
}

AudioPlayQueue::~AudioPlayQueue()
{
    RG_DEBUG << "dtor";

    clear();
}

void
AudioPlayQueue::addScheduled(PlayableAudioFile *file)
{
    if (m_files.find(file) != m_files.end()) {
        RG_WARNING << "WARNING: addScheduled(" << file << "): already in queue";
        return ;
    }

    m_files.insert(file);

    RealTime startTime = file->getStartTime();
    RealTime endTime = file->getStartTime() + file->getDuration();

    InstrumentId instrument = file->getInstrument();
    unsigned int index = instrumentId2Index(instrument);

    while ((unsigned int)m_instrumentIndex.size() <= index) {
        m_instrumentIndex.push_back(ReverseFileMap());
    }

#ifdef DEBUG_AUDIO_PLAY_QUEUE
    RG_DEBUG << "[" << this << "]::addScheduled(" << file << "): start " << file->getStartTime() << ", end " << file->getEndTime() << ", slots: ";
#endif

    for (int i = startTime.sec; i <= endTime.sec; ++i) {
        m_index[i].push_back(file);
        m_instrumentIndex[index][i].push_back(file);
        if (!file->isSmallFile()) {
            m_counts[i] += file->getTargetChannels();
            if (m_counts[i] > m_maxBuffers) {
                m_maxBuffers = m_counts[i];
            }
        }
#ifdef DEBUG_AUDIO_PLAY_QUEUE
        RG_DEBUG << "  " << i;
#endif

    }

#ifdef DEBUG_AUDIO_PLAY_QUEUE
    RG_DEBUG << "  (max buffers now " << m_maxBuffers << ")";
#endif
}

void
AudioPlayQueue::addUnscheduled(PlayableAudioFile *file)
{
#ifdef DEBUG_AUDIO_PLAY_QUEUE
    RG_DEBUG << "[" << this << "]::addUnscheduled(" << file << "): start " << file->getStartTime() << ", end " << file->getEndTime() << ", instrument " << file->getInstrument();
#endif

    m_unscheduled.push_back(file);

#ifdef DEBUG_AUDIO_PLAY_QUEUE
    RG_DEBUG << "[" << this << "]::addUnscheduled: now " << m_unscheduled.size() << " unscheduled files";
#endif

}

void
AudioPlayQueue::erase(PlayableAudioFile *file)
{
#ifdef DEBUG_AUDIO_PLAY_QUEUE
    RG_DEBUG << "erase(" << file << "): start " << file->getStartTime() << ", end " << file->getEndTime();
#endif

    FileSet::iterator fi = m_files.find(file);
    if (fi == m_files.end()) {
        for (FileList::iterator fli = m_unscheduled.begin();
                fli != m_unscheduled.end(); ++fli) {
            if (*fli == file) {
                m_unscheduled.erase(fli);
                delete file;
                return ;
            }
        }
        return ;
    }
    m_files.erase(fi);

    InstrumentId instrument = file->getInstrument();
    unsigned int index = instrumentId2Index(instrument);

    for (ReverseFileMap::iterator mi = m_instrumentIndex[index].begin();
            mi != m_instrumentIndex[index].end(); ++mi) {

        for (FileVector::iterator fi = mi->second.begin();
                fi != mi->second.end(); ++fi) {

            if (*fi == file) {
                mi->second.erase(fi);
                if (m_counts[mi->first] > 0)
                    --m_counts[mi->first];
                break;
            }
        }
    }

    for (ReverseFileMap::iterator mi = m_index.begin();
            mi != m_index.end(); ++mi) {

        for (FileVector::iterator fi = mi->second.begin();
                fi != mi->second.end(); ++fi) {

            if (*fi == file) {
                mi->second.erase(fi);
                if (m_counts[mi->first] > 0)
                    --m_counts[mi->first];
                break;
            }
        }
    }

    delete file;
}

void
AudioPlayQueue::clear()
{
#ifdef DEBUG_AUDIO_PLAY_QUEUE
    RG_DEBUG << "clear()";
#endif

    while (m_files.begin() != m_files.end()) {
        delete *m_files.begin();
        m_files.erase(m_files.begin());
    }

    while (m_unscheduled.begin() != m_unscheduled.end()) {
        delete *m_unscheduled.begin();
        m_unscheduled.erase(m_unscheduled.begin());
    }

    m_instrumentIndex.clear();
    m_index.clear();
    m_counts.clear();
    m_maxBuffers = 0;
}

bool
AudioPlayQueue::empty() const
{
    return m_unscheduled.empty() && m_files.empty();
}

size_t
AudioPlayQueue::size() const
{
    return m_unscheduled.size() + m_files.size();
}

void
AudioPlayQueue::getPlayingFiles(const RealTime &sliceStart,
                                const RealTime &sliceDuration,
                                FileSet &playing) const
{
    //    Profiler profiler("AudioPlayQueue::getPlayingFiles");

    // This one needs to be quick.

    playing.clear();

    RealTime sliceEnd = sliceStart + sliceDuration;

    for (int i = sliceStart.sec; i <= sliceEnd.sec; ++i) {

        ReverseFileMap::const_iterator mi(m_index.find(i));
        if (mi == m_index.end())
            continue;

        for (FileVector::const_iterator fi = mi->second.begin();
                fi != mi->second.end(); ++fi) {

            PlayableAudioFile *f = *fi;

            if (f->getStartTime() > sliceEnd ||
                    f->getStartTime() + f->getDuration() <= sliceStart)
                continue;

#ifdef FINE_DEBUG_AUDIO_PLAY_QUEUE
            RG_DEBUG << "getPlayingFiles(): found " << f << " in slot " << i;
#endif

            playing.insert(f);
        }
    }

    for (FileList::const_iterator fli = m_unscheduled.begin();
            fli != m_unscheduled.end(); ++fli) {
        PlayableAudioFile *file = *fli;
        if (file->getStartTime() <= sliceEnd &&
                file->getStartTime() + file->getDuration() > sliceStart) {
            playing.insert(file);
        }
    }

#ifdef FINE_DEBUG_AUDIO_PLAY_QUEUE
    if (playing.size() > 0) {
        RG_DEBUG << "getPlayingFiles(" << sliceStart << "," << sliceDuration << "): total " << playing.size() << " files";
    }
#endif
}

void
AudioPlayQueue::getPlayingFilesForInstrument(const RealTime &sliceStart,
        const RealTime &sliceDuration,
        InstrumentId instrumentId,
        PlayableAudioFile **playing,
        size_t &size) const
{
#ifdef FINE_DEBUG_AUDIO_PLAY_QUEUE
    bool printed = false;
    Profiler profiler("AudioPlayQueue::getPlayingFilesForInstrument", true);
#endif

    // This one needs to be quick.

    size_t written = 0;

    RealTime sliceEnd = sliceStart + sliceDuration;

    unsigned int index = instrumentId2Index(instrumentId);
    if (index >= (unsigned int)m_instrumentIndex.size()) {
        goto unscheduled; // nothing scheduled here
    }

    for (int i = sliceStart.sec; i <= sliceEnd.sec; ++i) {

        ReverseFileMap::const_iterator mi
        (m_instrumentIndex[index].find(i));

        if (mi == m_instrumentIndex[index].end())
            continue;

        for (FileVector::const_iterator fi = mi->second.begin();
                fi != mi->second.end(); ++fi) {

            PlayableAudioFile *f = *fi;

            if (f->getInstrument() != instrumentId)
                continue;

#ifdef FINE_DEBUG_AUDIO_PLAY_QUEUE
            if (!printed) {
                RG_DEBUG << "getPlayingFilesForInstrument(" << sliceStart << ", " << sliceDuration << ", " << instrumentId << ")";
                printed = true;
            }
#endif

            if (f->getStartTime() > sliceEnd ||
                    f->getEndTime() <= sliceStart) {

#ifdef FINE_DEBUG_AUDIO_PLAY_QUEUE
                RG_DEBUG << "  rejected " << f << " in slot " << i;
                if (f->getStartTime() > sliceEnd) {
                    RG_DEBUG << "  (" << f->getStartTime() << " > " << sliceEnd << ")";
                } else {
                    RG_DEBUG << "  (" << f->getEndTime() << " <= " << sliceStart << ")";
                }
#endif

                continue;
            }

#ifdef FINE_DEBUG_AUDIO_PLAY_QUEUE
            RG_DEBUG << "  found " << f << " in slot " << i << " (" << f->getStartTime() << " -> " << f->getEndTime() << ")";
#endif

            size_t j = 0;
            for (j = 0; j < written; ++j) {
                if (playing[j] == f)
                    break;
            }
            if (j < written)
                break; // already have it

            if (written >= size) {
#ifdef FINE_DEBUG_AUDIO_PLAY_QUEUE
                RG_DEBUG << "  No room to write it!";
#endif

                break;
            }

            playing[written++] = f;
        }
    }

unscheduled:

    for (FileList::const_iterator fli = m_unscheduled.begin();
            fli != m_unscheduled.end(); ++fli) {

        PlayableAudioFile *f = *fli;

        if (f->getInstrument() != instrumentId) {
#ifdef FINE_DEBUG_AUDIO_PLAY_QUEUE
            RG_DEBUG << "  rejecting unscheduled " << f << " as wrong instrument (" << f->getInstrument() << " != " << instrumentId << ")";
#endif

            continue;
        }

#ifdef FINE_DEBUG_AUDIO_PLAY_QUEUE
        if (!printed) {
            RG_DEBUG << "getPlayingFilesForInstrument(" << sliceStart << ", " << sliceDuration << ", " << instrumentId << ")";
            printed = true;
        }
#endif

        if (f->getStartTime() <= sliceEnd &&
                f->getStartTime() + f->getDuration() > sliceStart) {

#ifdef FINE_DEBUG_AUDIO_PLAY_QUEUE
            RG_DEBUG << "  found " << f << " in unscheduled list (" << f->getStartTime() << " -> " << f->getEndTime() << ")";
#endif

            if (written >= size)
                break;
            playing[written++] = f;

#ifdef FINE_DEBUG_AUDIO_PLAY_QUEUE

        } else {

            RG_DEBUG << "  rejected " << f << " in unscheduled list";
            if (f->getStartTime() > sliceEnd) {
                RG_DEBUG << "  (" << f->getStartTime() << " > " << sliceEnd << ")";
            } else {
                RG_DEBUG << "  (" << f->getEndTime() << " <= " << sliceStart << ")";
            }
#endif

        }
    }

#ifdef FINE_DEBUG_AUDIO_PLAY_QUEUE
    if (written > 0) {
        RG_DEBUG << "getPlayingFilesForInstrument(): total " << written << " files";
    }
#endif

    size = written;
}

bool
AudioPlayQueue::haveFilesForInstrument(InstrumentId instrumentId) const
{
#ifdef FINE_DEBUG_AUDIO_PLAY_QUEUE
    RG_DEBUG << "haveFilesForInstrument(" << instrumentId << ")...";
#endif

    unsigned int index = instrumentId2Index(instrumentId);

    if (index < (unsigned int)m_instrumentIndex.size() &&
            !m_instrumentIndex[index].empty()) {
#ifdef FINE_DEBUG_AUDIO_PLAY_QUEUE
        RG_DEBUG << "  yes (scheduled)";
#endif

        return true;
    }

    for (FileList::const_iterator fli = m_unscheduled.begin();
            fli != m_unscheduled.end(); ++fli) {
        PlayableAudioFile *file = *fli;
        if (file->getInstrument() == instrumentId) {
#ifdef FINE_DEBUG_AUDIO_PLAY_QUEUE
            RG_DEBUG << "  yes (unscheduled)";
#endif

            return true;
        }
    }

#ifdef FINE_DEBUG_AUDIO_PLAY_QUEUE
    RG_DEBUG << "  no";
#endif

    return false;
}

const AudioPlayQueue::FileSet &
AudioPlayQueue::getAllScheduledFiles() const
{
#ifdef DEBUG_AUDIO_PLAY_QUEUE
    RG_DEBUG << "[" << this << "]::getAllScheduledFiles(): have " << m_files.size() << " files";
#endif

    return m_files;
}

const AudioPlayQueue::FileList &
AudioPlayQueue::getAllUnscheduledFiles() const
{
#ifdef DEBUG_AUDIO_PLAY_QUEUE
    RG_DEBUG << "[" << this << "]::getAllUnscheduledFiles(): have " << m_unscheduled.size() << " files";
#endif

    return m_unscheduled;
}


}

