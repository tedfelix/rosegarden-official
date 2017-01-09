/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.
    See the AUTHORS file for more details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

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
    std::cerr << "AudioPlayQueue::~AudioPlayQueue()" << std::endl;
    clear();
}

void
AudioPlayQueue::addScheduled(PlayableAudioFile *file)
{
    if (m_files.find(file) != m_files.end()) {
        std::cerr << "WARNING: AudioPlayQueue::addScheduled("
        << file << "): already in queue" << std::endl;
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
    std::cerr << "AudioPlayQueue[" << this << "]::addScheduled(" << file << "): start " << file->getStartTime() << ", end " << file->getEndTime() << ", slots: " << std::endl;
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
        std::cerr << i << " ";
#endif

    }

#ifdef DEBUG_AUDIO_PLAY_QUEUE
    std::cerr << std::endl << "(max buffers now "
    << m_maxBuffers << ")" << std::endl;
#endif
}

void
AudioPlayQueue::addUnscheduled(PlayableAudioFile *file)
{
#ifdef DEBUG_AUDIO_PLAY_QUEUE
    std::cerr << "AudioPlayQueue[" << this << "]::addUnscheduled(" << file << "): start " << file->getStartTime() << ", end " << file->getEndTime() << ", instrument " << file->getInstrument() << std::endl;
#endif

    m_unscheduled.push_back(file);

#ifdef DEBUG_AUDIO_PLAY_QUEUE

    std::cerr << "AudioPlayQueue[" << this << "]::addUnscheduled: now " << m_unscheduled.size() << " unscheduled files" << std::endl;
#endif

}

void
AudioPlayQueue::erase(PlayableAudioFile *file)
{
#ifdef DEBUG_AUDIO_PLAY_QUEUE
    std::cerr << "AudioPlayQueue::erase(" << file << "): start " << file->getStartTime() << ", end " << file->getEndTime() << std::endl;
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
    std::cerr << "AudioPlayQueue::clear()" << std::endl;
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

            std::cerr << "... found " << f << " in slot " << i << std::endl;
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
        std::cerr << "AudioPlayQueue::getPlayingFiles(" << sliceStart << ","
        << sliceDuration << "): total "
        << playing.size() << " files" << std::endl;
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
                std::cerr << "AudioPlayQueue::getPlayingFilesForInstrument(" << sliceStart
                << ", " << sliceDuration << ", " << instrumentId << ")"
                << std::endl;
                printed = true;
            }
#endif

            if (f->getStartTime() > sliceEnd ||
                    f->getEndTime() <= sliceStart) {

#ifdef FINE_DEBUG_AUDIO_PLAY_QUEUE
                std::cerr << "... rejected " << f << " in slot " << i << std::endl;
                if (f->getStartTime() > sliceEnd) {
                    std::cerr << "(" << f->getStartTime() << " > " << sliceEnd
                    << ")" << std::endl;
                } else {
                    std::cerr << "(" << f->getEndTime() << " <= " << sliceStart
                    << ")" << std::endl;
                }
#endif

                continue;
            }

#ifdef FINE_DEBUG_AUDIO_PLAY_QUEUE
            std::cerr << "... found " << f << " in slot " << i << " ("
            << f->getStartTime() << " -> " << f->getEndTime()
            << ")" << std::endl;
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
                std::cerr << "No room to write it!" << std::endl;
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
            std::cerr << "rejecting unscheduled " << f << " as wrong instrument ("
            << f->getInstrument() << " != " << instrumentId << ")" << std::endl;
#endif

            continue;
        }

#ifdef FINE_DEBUG_AUDIO_PLAY_QUEUE
        if (!printed) {
            std::cerr << "AudioPlayQueue::getPlayingFilesForInstrument(" << sliceStart
            << ", " << sliceDuration << ", " << instrumentId << ")"
            << std::endl;
            printed = true;
        }
#endif

        if (f->getStartTime() <= sliceEnd &&
                f->getStartTime() + f->getDuration() > sliceStart) {

#ifdef FINE_DEBUG_AUDIO_PLAY_QUEUE
            std::cerr << "... found " << f << " in unscheduled list ("
            << f->getStartTime() << " -> " << f->getEndTime()
            << ")" << std::endl;
#endif

            if (written >= size)
                break;
            playing[written++] = f;

#ifdef FINE_DEBUG_AUDIO_PLAY_QUEUE

        } else {

            std::cerr << "... rejected " << f << " in unscheduled list" << std::endl;
            if (f->getStartTime() > sliceEnd) {
                std::cerr << "(" << f->getStartTime() << " > " << sliceEnd
                << ")" << std::endl;
            } else {
                std::cerr << "(" << f->getEndTime() << " <= " << sliceStart
                << ")" << std::endl;
            }
#endif

        }
    }

#ifdef FINE_DEBUG_AUDIO_PLAY_QUEUE
    if (written > 0) {
        std::cerr << "AudioPlayQueue::getPlayingFilesForInstrument: total "
        << written << " files" << std::endl;
    }
#endif

    size = written;
}

bool
AudioPlayQueue::haveFilesForInstrument(InstrumentId instrumentId) const
{
#ifdef FINE_DEBUG_AUDIO_PLAY_QUEUE
    std::cerr << "AudioPlayQueue::haveFilesForInstrument(" << instrumentId << ")...";
#endif

    unsigned int index = instrumentId2Index(instrumentId);

    if (index < (unsigned int)m_instrumentIndex.size() &&
            !m_instrumentIndex[index].empty()) {
#ifdef FINE_DEBUG_AUDIO_PLAY_QUEUE
        std::cerr << " yes (scheduled)" << std::endl;
#endif

        return true;
    }

    for (FileList::const_iterator fli = m_unscheduled.begin();
            fli != m_unscheduled.end(); ++fli) {
        PlayableAudioFile *file = *fli;
        if (file->getInstrument() == instrumentId) {
#ifdef FINE_DEBUG_AUDIO_PLAY_QUEUE
            std::cerr << " yes (unscheduled)" << std::endl;
#endif

            return true;
        }
    }

#ifdef FINE_DEBUG_AUDIO_PLAY_QUEUE
    std::cerr << " no" << std::endl;
#endif

    return false;
}

const AudioPlayQueue::FileSet &
AudioPlayQueue::getAllScheduledFiles() const
{
#ifdef DEBUG_AUDIO_PLAY_QUEUE
    std::cerr << "AudioPlayQueue[" << this << "]::getAllScheduledFiles: have " << m_files.size() << " files" << std::endl;
#endif

    return m_files;
}

const AudioPlayQueue::FileList &
AudioPlayQueue::getAllUnscheduledFiles() const
{
#ifdef DEBUG_AUDIO_PLAY_QUEUE
    std::cerr << "AudioPlayQueue[" << this << "]::getAllUnscheduledFiles: have " << m_unscheduled.size() << " files" << std::endl;
#endif

    return m_unscheduled;
}


}

