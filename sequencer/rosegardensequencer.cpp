// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>

#include <iostream>

#include <klocale.h>
#include <kstandarddirs.h>

#include <dcopclient.h>
#include <qdatetime.h>
#include <qstring.h>
#include <qdir.h>
#include <qbuffer.h>

#include "rosedebug.h"
#include "rosegardensequencer.h"
#include "rosegardendcop.h"
#include "ControlBlock.h"
#include "Sequencer.h"
#include "MappedInstrument.h"

using std::cerr;
using std::endl;
using std::cout;

// Seems not to be properly defined under some gcc 2.95 setups
#ifndef MREMAP_MAYMOVE
# define MREMAP_MAYMOVE        1
#endif

/**
 * An mmap()ed segment
 */
class MmappedSegment
{
public:
    MmappedSegment(const QString filename);
    ~MmappedSegment();

    bool remap();
    QString getFileName() { return m_filename; }
    QByteArray getByteArray() { return m_byteArray; }

    class iterator 
    {
    public:
        iterator() : m_pastEnd(true), m_s(0), m_dataStream(0) {};
        iterator(MmappedSegment* s, bool atEnd = false);

        ~iterator() { delete m_dataStream; }

        iterator& operator=(const iterator&);
        bool operator==(const iterator&);
        bool operator!=(const iterator& it) { return !operator==(it); }

        bool atEnd() { return m_pastEnd; }

        /// go back to beginning of stream
        void reset();
        /// rebuild the stream (called after a remap)
        void resetStream();

        iterator& operator++();
        iterator  operator++(int);
        iterator& operator+=(int);
        iterator& operator-=(int);

        Rosegarden::MappedEvent operator*();

        MmappedSegment* getSegment() { return m_s; }

    protected:
        //--------------- Data members ---------------------------------
        bool m_pastEnd;
        bool m_firstEventRead;

        MmappedSegment* m_s;
        QDataStream *m_dataStream;
        Rosegarden::MappedEvent m_currentEvent;
    };

    iterator begin();
    iterator end(); // TODO : replace this by iterator::pastEnd()

protected:
    void map();
    void unmap();

    //--------------- Data members ---------------------------------
    int m_fd;
    size_t m_mmappedSize;
    char* m_mmappedBuffer;
    QByteArray m_byteArray;
    QString m_filename;
};

MmappedSegment::MmappedSegment(const QString filename)
    : m_fd(-1),
      m_mmappedSize(0),
      m_mmappedBuffer((char*)0),
      m_filename(filename)
{
    SEQUENCER_DEBUG << "mmapping " << filename << endl;

    map();
}
void MmappedSegment::map()
{
    QFileInfo fInfo(m_filename);
    m_mmappedSize = fInfo.size();

    m_fd = ::open(m_filename.latin1(), O_RDWR);

    m_mmappedBuffer = (char*)::mmap(0, m_mmappedSize, PROT_READ, MAP_SHARED, m_fd, 0);

    if (m_mmappedBuffer == (void*)-1) {
        SEQUENCER_DEBUG << QString("mmap failed : (%1) %2\n").arg(errno).arg(strerror(errno));
        throw Rosegarden::Exception("mmap failed");
    }

    SEQUENCER_DEBUG << "MmappedSegment::map() : setRawData("
                    << (void*)m_mmappedBuffer << "," << m_mmappedSize << ")\n";

    m_byteArray.setRawData(m_mmappedBuffer, m_mmappedSize);
}

MmappedSegment::~MmappedSegment()
{
    unmap();
}

void MmappedSegment::unmap()
{
    m_byteArray.resetRawData(m_mmappedBuffer, m_mmappedSize);
    ::munmap(m_mmappedBuffer, m_mmappedSize);
    ::close(m_fd);
}

bool MmappedSegment::remap()
{
    QFileInfo fInfo(m_filename);
    size_t newSize = fInfo.size();

    SEQUENCER_DEBUG << "remap() from " << m_mmappedSize << " to "
                    << newSize << endl;

    if (m_mmappedSize == newSize) {
        SEQUENCER_DEBUG << "remap() : sizes are identical, remap not forced - nothing to do\n";
        return false;
    }

    SEQUENCER_DEBUG << "MmappedSegment::remap() : resetRawData(" << (void*)m_mmappedBuffer
                    << "," << m_mmappedSize << ")\n";

    m_byteArray.resetRawData(m_mmappedBuffer, m_mmappedSize);

#ifdef linux
    m_mmappedBuffer = (char*)::mremap(m_mmappedBuffer, m_mmappedSize, newSize, MREMAP_MAYMOVE);
#else
    ::munmap(m_mmappedBuffer, m_mmappedSize);
    m_mmappedBuffer = (char *)::mmap(0, newSize, PROT_READ, MAP_SHARED, m_fd, 0);
#endif

    if (m_mmappedBuffer == (void*)-1) {
            SEQUENCER_DEBUG << QString("mremap failed : (%1) %2\n").arg(errno).arg(strerror(errno));
            throw Rosegarden::Exception("mremap failed");
    }
    
    m_mmappedSize = newSize;

    m_byteArray.setRawData(m_mmappedBuffer, m_mmappedSize);

    return true;
}

MmappedSegment::iterator MmappedSegment::begin()
{
    return iterator(this);
}

MmappedSegment::iterator MmappedSegment::end()
{
    return iterator(this, true);
}

MmappedSegment::iterator::iterator(MmappedSegment* s, bool atEnd)
    : m_pastEnd(atEnd), m_firstEventRead(false), m_s(s),
      m_dataStream(new QDataStream(m_s->getByteArray(), IO_ReadOnly))
{
    if (atEnd) // actually jump to end of device
        m_dataStream->device()->at(m_dataStream->device()->size());
};

MmappedSegment::iterator& MmappedSegment::iterator::operator=(const iterator& it)
{
    if (&it == this) return *this;

    m_pastEnd = it.m_pastEnd;
    m_s = it.m_s;
    delete m_dataStream;
    m_dataStream = new QDataStream(it.m_s->getByteArray(), IO_ReadOnly);
    m_currentEvent = it.m_currentEvent;

    return *this;
}

MmappedSegment::iterator& MmappedSegment::iterator::operator++()
{
    if (!m_dataStream->atEnd()) {
//         SEQUENCER_DEBUG << "MmappedSegment::iterator::operator++() " << this << " - at "
//                         << m_dataStream->device()->at() << endl;
        do
            (*m_dataStream) >> m_currentEvent;
        while ((m_currentEvent.getType() == 0) && !m_dataStream->atEnd());
        // skip null events - there can be some if the file has been
        // zeroed out after events have been deleted

//         SEQUENCER_DEBUG << "MmappedSegment::iterator::operator++() " << this << " - at "
//                         << m_dataStream->device()->at() << " after event read\n";
    } else {

//         SEQUENCER_DEBUG << "MmappedSegment::iterator::operator*() " << this
//                         << " - reached end of stream\n";
        m_pastEnd = true;
    }

    return *this;
}

MmappedSegment::iterator MmappedSegment::iterator::operator++(int)
{
    iterator r = *this;

    if (!m_dataStream->atEnd()) {
        do
            (*m_dataStream) >> m_currentEvent;
        while (m_currentEvent.getType() == 0 && !m_dataStream->atEnd());

    } else {

        m_pastEnd = true;
    }


    return r;
}

MmappedSegment::iterator& MmappedSegment::iterator::operator+=(int offset)
{
    QIODevice::Offset currentPos = m_dataStream->device()->at();
    m_dataStream->device()->at(currentPos + (offset * Rosegarden::MappedEvent::streamedSize));

    return *this;
}

MmappedSegment::iterator& MmappedSegment::iterator::operator-=(int offset)
{
    QIODevice::Offset currentPos = m_dataStream->device()->at();
    m_dataStream->device()->at(currentPos - (offset * Rosegarden::MappedEvent::streamedSize));

    return *this;
}


bool MmappedSegment::iterator::operator==(const iterator& it)
{
    return ((m_s == it.m_s) && (m_pastEnd == it.m_pastEnd) &&
            (m_dataStream->device()->at() == it.m_dataStream->device()->at()));
}

void MmappedSegment::iterator::reset()
{
    m_dataStream->device()->reset();
    m_firstEventRead = false;
    m_pastEnd = false;
}

void MmappedSegment::iterator::resetStream()
{
    delete m_dataStream;
    m_dataStream = new QDataStream(m_s->getByteArray(), IO_ReadOnly);
    m_firstEventRead = false;
    m_pastEnd = false;
}

Rosegarden::MappedEvent MmappedSegment::iterator::operator*()
{
    if (!m_firstEventRead) { // then read it...
        ++(*this);
        m_firstEventRead = true;
    }

    return m_currentEvent;
}

void RosegardenSequencerApp::dumpFirstSegment()
{
    SEQUENCER_DEBUG << "Dumping 1st segment data :\n";

    unsigned int i = 0;
    
    for (MmappedSegment::iterator it = m_mmappedSegments[0]->begin();
         it != m_mmappedSegments[0]->end();
         ++it) {

        Rosegarden::MappedEvent evt = (*it);
        SEQUENCER_DEBUG << i << " : inst = "  << evt.getInstrument()
                        << " - type = "       << evt.getType()
                        << " - data1 = "      << (unsigned int)evt.getData1()
                        << " - data2 = "      << (unsigned int)evt.getData2()
                        << " - time = "       << evt.getEventTime()
                        << " - duration = "   << evt.getDuration()
                        << " - audio mark = " << evt.getAudioStartMarker()
                        << endl;

        ++i;
    }
    
}

// Ew ew ew - move this to base
kdbgstream&
operator<<(kdbgstream &dbg, const Rosegarden::RealTime &t)
{
    dbg << "sec : " << t.sec << ", usec : " << t.usec;
    return dbg;
}

//--------------------------------------------------

using Rosegarden::InstrumentId;
using Rosegarden::ControlBlock;

class ControlBlockMmapper
{
public:
    ControlBlockMmapper(QString fileName);
    ~ControlBlockMmapper();
    
    QString getFileName() { return m_fileName; }
    void refresh();

    // delegate ControlBlock's interface
    InstrumentId getInstrumentForTrack(unsigned int trackId);
    bool isTrackMuted(unsigned int trackId);

protected:

    //--------------- Data members ---------------------------------
    QString m_fileName;
    int m_fd;
    void* m_mmappedBuffer;
    size_t m_mmappedSize;
    ControlBlock* m_controlBlock;
};

ControlBlockMmapper::ControlBlockMmapper(QString fileName)
    : m_fileName(fileName),
      m_fd(-1),
      m_mmappedBuffer(0),
      m_mmappedSize(ControlBlock::getSize()),
      m_controlBlock(0)
{
    m_fd = ::open(m_fileName.latin1(), O_RDWR);

    if (m_fd < 0) {
        SEQMAN_DEBUG << "ControlBlockMmapper : Couldn't open " << m_fileName
                     << endl;
        throw Rosegarden::Exception(std::string("Couldn't open ") + m_fileName.latin1());
    }

    //
    // mmap() file for reading
    //
    m_mmappedBuffer = (char*)::mmap(0, m_mmappedSize, PROT_READ, MAP_SHARED, m_fd, 0);

    if (m_mmappedBuffer == (void*)-1) {
        SEQUENCER_DEBUG << QString("mmap failed : (%1) %2\n").arg(errno).arg(strerror(errno));
        throw Rosegarden::Exception("mmap failed");
    }

    SEQMAN_DEBUG << "ControlBlockMmapper : mmap size : " << m_mmappedSize
                 << " at " << (void*)m_mmappedBuffer << endl;

    // Create new control block on file
    m_controlBlock = new (m_mmappedBuffer) ControlBlock;
}

ControlBlockMmapper::~ControlBlockMmapper()
{
    ::munmap(m_mmappedBuffer, m_mmappedSize);
    ::close(m_fd);
}


void ControlBlockMmapper::refresh()
{
    ::msync(m_mmappedBuffer, m_mmappedSize, MS_ASYNC);
}

InstrumentId ControlBlockMmapper::getInstrumentForTrack(unsigned int trackNb)
{
    return m_controlBlock->getInstrumentForTrack(trackNb);
}

bool ControlBlockMmapper::isTrackMuted(unsigned int trackNb)
{
    return m_controlBlock->isTrackMuted(trackNb);
}

//----------------------------------------


class MmappedSegmentsMetaIterator
{
public:
    MmappedSegmentsMetaIterator(RosegardenSequencerApp::mmappedsegments&, ControlBlockMmapper*);
    ~MmappedSegmentsMetaIterator();

    /// reset all iterators to beginning
    void reset();
    bool jumpToTime(const Rosegarden::RealTime&);
    bool fillCompositionWithEventsUntil(Rosegarden::MappedComposition*,
                                        const Rosegarden::RealTime&);
    void resetIteratorForSegment(const QString& filename);

    void addSegment(MmappedSegment*);
    void deleteSegment(MmappedSegment*);

protected:
    /// Delete all iterators
    void clear();
    bool moveIteratorToTime(MmappedSegment::iterator&, const Rosegarden::RealTime&);

    //--------------- Data members ---------------------------------

    ControlBlockMmapper* m_controlBlockMmapper;

    Rosegarden::RealTime m_currentTime;
    RosegardenSequencerApp::mmappedsegments& m_segments;

    typedef std::vector<MmappedSegment::iterator*> segmentiterators;
    segmentiterators m_iterators;
};

MmappedSegmentsMetaIterator::MmappedSegmentsMetaIterator(RosegardenSequencerApp::mmappedsegments& segments,
                                                         ControlBlockMmapper* controlBlockMmapper)
    : m_controlBlockMmapper(controlBlockMmapper),
      m_segments(segments)
{
    for(RosegardenSequencerApp::mmappedsegments::iterator i = m_segments.begin();
        i != m_segments.end(); ++i)
        m_iterators.push_back(new MmappedSegment::iterator(i->second));
}

MmappedSegmentsMetaIterator::~MmappedSegmentsMetaIterator()
{
    clear();
}

void MmappedSegmentsMetaIterator::addSegment(MmappedSegment* ms)
{
    MmappedSegment::iterator* iter = new MmappedSegment::iterator(ms);
    moveIteratorToTime(*iter, m_currentTime);
    m_iterators.push_back(iter);
}

void MmappedSegmentsMetaIterator::deleteSegment(MmappedSegment* ms)
{
    for(segmentiterators::iterator i = m_iterators.begin(); i != m_iterators.end(); ++i) {
        if ((*i)->getSegment() == ms) {
            SEQUENCER_DEBUG << "deleteSegment : found segment to delete : "
                            << ms->getFileName() << endl;
            delete (*i);
            m_iterators.erase(i);
            break;
        }
    }
}

void MmappedSegmentsMetaIterator::clear()
{
    for(unsigned int i = 0; i < m_iterators.size(); ++i)
        delete m_iterators[i];

    m_iterators.clear();
}

void MmappedSegmentsMetaIterator::reset()
{
    m_currentTime.sec = m_currentTime.usec = 0;

    for(segmentiterators::iterator i = m_iterators.begin(); i != m_iterators.end(); ++i) {
        (*i)->reset();
    }
    
}

bool MmappedSegmentsMetaIterator::jumpToTime(const Rosegarden::RealTime& startTime)
{
    SEQUENCER_DEBUG << "jumpToTime(" << startTime << ")\n";

    reset();

    bool res = true;

    m_currentTime = startTime;

    if (startTime > Rosegarden::RealTime(0,0)) {
        for(segmentiterators::iterator i = m_iterators.begin(); i != m_iterators.end(); ++i)
            if (!moveIteratorToTime(*(*i), startTime)) res = false;
    }

    return res;
}

bool MmappedSegmentsMetaIterator::moveIteratorToTime(MmappedSegment::iterator& iter,
                                                       const Rosegarden::RealTime& startTime)
{
    MmappedSegment* mmappedSegment = iter.getSegment();

    Rosegarden::MappedEvent e = *iter;

    while ((e.getEventTime() < startTime) && (iter != mmappedSegment->end())) {
        ++iter;
        e = *iter;
    }
    bool res = iter != mmappedSegment->end();

//     if (!res)
//         SEQUENCER_DEBUG << "moveIteratorToTime for segment #" << mmappedSegment->getFileName()
//                         << " to time " << startTime
//                         << " moved iterator to end\n";
    return res;
}

bool MmappedSegmentsMetaIterator::fillCompositionWithEventsUntil(Rosegarden::MappedComposition* c,
                                                                   const Rosegarden::RealTime& endTime)
{
//     SEQUENCER_DEBUG << "fillCompositionWithEventsUntil " << endTime << endl;

    m_currentTime = endTime;

    // keep track of the segments which still have valid events
    std::vector<bool> validSegments;
    for(unsigned int i = 0; i < m_segments.size(); ++i) validSegments.push_back(true);

    bool foundOneEvent = false;

    do {
        foundOneEvent = false;

        for(unsigned int i = 0; i < m_iterators.size(); ++i) {

//             SEQUENCER_DEBUG << "fillCompositionWithEventsUntil : checking segment #"
//                             << i << endl;

            if (!validSegments[i]) {
//                 SEQUENCER_DEBUG << "fillCompositionWithEventsUntil : no more events in segment #"
//                                 << i << endl;
                continue; // skip this segment
            }

            MmappedSegment::iterator* iter = m_iterators[i];

            if (*iter == iter->getSegment()->end()) {
//                 SEQUENCER_DEBUG << "fillCompositionWithEventsUntil : " << endTime
//                                 << " reached end of segment #"
//                                 << i << endl;
                continue;
            }

            Rosegarden::MappedEvent *evt = new Rosegarden::MappedEvent(*(*iter));

            if (evt->getEventTime() < endTime) {
                evt->setInstrument(m_controlBlockMmapper->getInstrumentForTrack(evt->getTrackId()));

                SEQUENCER_DEBUG << "fillCompositionWithEventsUntil : " << endTime
                                << " inserting evt from segment #"
                                << i
                                << " : trackId: " << evt->getTrackId()
                                << " - inst: " << evt->getInstrument()
                                << " - type: " << evt->getType()
                                << " - time: " << evt->getEventTime()
                                << " - duration: " << evt->getDuration()
                                << " - data1: " << (unsigned int)evt->getData1()
                                << " - data2: " << (unsigned int)evt->getData2()
                                << endl;
                if (evt->getType() != 0 && !m_controlBlockMmapper->isTrackMuted(evt->getTrackId())) {
                    c->insert(evt);
                }

                foundOneEvent = true;
                ++(*iter);

            } else {
                validSegments[i] = false; // no more events to get from this segment
//                 SEQUENCER_DEBUG << "fillCompositionWithEventsUntil : no more events to get from segment #"
//                                 << i << endl;
            }

        }

    } while (foundOneEvent);

    return true;
}

void MmappedSegmentsMetaIterator::resetIteratorForSegment(const QString& filename)
{
    for(segmentiterators::iterator i = m_iterators.begin(); i != m_iterators.end(); ++i) {
        MmappedSegment::iterator* iter = *i;

        if (iter->getSegment()->getFileName() == filename) {
//             SEQUENCER_DEBUG << "resetIteratorForSegment(" << filename << ") : found iterator\n";
            iter->resetStream();
            moveIteratorToTime(*iter, m_currentTime);
            break;
        }

    }
}



//----------------------------------------

// The default latency and read-ahead values are actually sent
// down from the GUI every time playback or recording starts
// so the local values are kind of meaningless.
//
//
RosegardenSequencerApp::RosegardenSequencerApp(
        const std::vector<std::string> &jackArgs):
    DCOPObject("RosegardenSequencerIface"),
    m_sequencer(0),
    m_transportStatus(STOPPED),
    m_songPosition(0, 0),
    m_lastFetchSongPosition(0, 0),
    m_fetchLatency(0, 30000),      // default value
    m_playLatency(0, 50000),       // default value
    m_readAhead(0, 40000),         // default value
    //m_audioPlayLatency(0, 0),
    //m_audioRecordLatency(0, 0),
    m_loopStart(0, 0),
    m_loopEnd(0, 0),
    m_studio(new Rosegarden::MappedStudio()),
    m_oldSliceSize(0, 0),
    m_segmentFilesPath(KGlobal::dirs()->resourceDirs("tmp").first()),
    m_metaIterator(0),
    m_controlBlockMmapper(0)
{
    SEQUENCER_DEBUG << "Registering with DCOP server" << endl;

    // Without DCOP we are nothing
    QCString realAppId = kapp->dcopClient()->registerAs(kapp->name(), false);

    if (realAppId.isNull())
    {
        SEQUENCER_DEBUG << "RosegardenSequencer cannot register "
                        << "with DCOP server" << endl;
        close();
    }

    // Initialise the MappedStudio
    //
    initialiseStudio();

    // Creating this object also initialises the Rosegarden aRts or
    // ALSA/JACK interface for both playback and recording. MappedStudio
    // aduio faders are also created.
    //
    m_sequencer = new Rosegarden::Sequencer(m_studio, jackArgs);
    m_studio->setSequencer(m_sequencer);

    if (!m_sequencer)
    {
        SEQUENCER_DEBUG << "RosegardenSequencer object could not be allocated"
                        << endl;
        close();
    }

    // set this here and now so we can accept async midi events
    //
    m_sequencer->record(Rosegarden::ASYNCHRONOUS_MIDI);

    // Setup the slice timer
    //
    m_sliceTimer = new QTimer(this);
//    connect(m_sliceTimer, SIGNAL(timeout()), this, SLOT(slotRevertSliceSize()));

    // Check for new clients every so often
    //
    m_newClientTimer = new QTimer(this);
    connect(m_newClientTimer, SIGNAL(timeout()),
            this, SLOT(slotCheckForNewClients()));

    m_newClientTimer->start(3000); // every 3 seconds
}

RosegardenSequencerApp::~RosegardenSequencerApp()
{
    SEQUENCER_DEBUG << "RosegardenSequencer - shutting down" << endl;
    delete m_sequencer;
    delete m_studio;
    delete m_controlBlockMmapper;
}

void
RosegardenSequencerApp::quit()
{
    close();

    // and break out of the loop next time around
    m_transportStatus = QUIT;
}


// We just "send" to this - no call (removed post 0.1
// to prevent hangs)
//
void
RosegardenSequencerApp::stop()
{
    // set our state at this level to STOPPING (pending any
    // unfinished NOTES)
    m_transportStatus = STOPPING;

    // process pending NOTE OFFs and stop the Sequencer
    m_sequencer->stopPlayback();

    // the Sequencer doesn't need to know these once
    // we've stopped.
    //
    m_songPosition.sec = 0;
    m_songPosition.usec = 0;
    m_lastFetchSongPosition.sec = 0;
    m_lastFetchSongPosition.usec = 0;


    cleanupMmapData();

    // report
    //
    SEQUENCER_DEBUG << "RosegardenSequencerApp::stop() - stopping" << endl;

}

// Get a slice of events from the GUI
//
Rosegarden::MappedComposition*
RosegardenSequencerApp::fetchEvents(const Rosegarden::RealTime &start,
                                    const Rosegarden::RealTime &end,
                                    bool firstFetch)
{
    // Always return nothing if we're stopped
    //
    if ( m_transportStatus == STOPPED || m_transportStatus == STOPPING )
        return 0;

    // If we're looping then we should get as much of the rest of
    // the right hand of the loop as possible and also events from
    // the beginning of the loop.  We can do this in two fetches.
    // Make sure that we delete all returned pointers when we've
    // finished with them.
    //
    //
    if (isLooping() == true && end > m_loopEnd)
    {

        Rosegarden::RealTime loopOverlap = end - m_loopEnd;
        Rosegarden::MappedComposition *endLoop, *beginLoop;

        endLoop = getSlice(start, m_loopEnd, firstFetch);
        beginLoop = getSlice(m_loopStart,
                             m_loopStart + loopOverlap, true);

        // move the start time of the begin section one loop width
        // into the future and ensure that we keep the clocks level
        // until this time has passed
        //
        beginLoop->moveStartTime(m_loopEnd - m_loopStart);

        (*endLoop) = (*endLoop) + (*beginLoop);
        delete beginLoop;
        return endLoop;
    }
    else
        return getSlice(start, end, firstFetch);
}


Rosegarden::MappedComposition*
RosegardenSequencerApp::getSlice(const Rosegarden::RealTime &start,
                                 const Rosegarden::RealTime &end,
                                 bool firstFetch)
{
    Rosegarden::MappedComposition *mC = new Rosegarden::MappedComposition();

    if (firstFetch || (start < m_lastStartTime)) {
        m_metaIterator->jumpToTime(start);
    }

    m_metaIterator->fillCompositionWithEventsUntil(mC, end);

    m_lastStartTime = start;

    return mC;
}


// The first fetch of events from the core/ and initialisation for
// this session of playback.  We fetch up to m_playLatency microseconds/
// seconds ahead at first at then top up once we're within m_fetchLatency
// of the end of the last fetch.
//
bool
RosegardenSequencerApp::startPlaying()
{
    // Fetch up to m_readHead microseconds worth of events
    //
    m_lastFetchSongPosition = m_songPosition + m_readAhead;

    // This will reset the Sequencer's internal clock
    // ready for new playback
    m_sequencer->initialisePlayback(m_songPosition, m_playLatency);

    m_mC.clear();
    m_mC = *fetchEvents(m_songPosition, m_songPosition + m_readAhead, true);

    // process whether we need to or not as this also processes
    // the audio queue for us
    //
    m_sequencer->processEventsOut(m_mC, m_playLatency, false);

    // tell the gui about this slice of events
    notifyVisuals(&m_mC);

    return true;
}

void
RosegardenSequencerApp::notifyVisuals(Rosegarden::MappedComposition *mC)
{
    // Tell the gui that we're processing these events next
    //
    QByteArray data;
    QDataStream arg(data, IO_WriteOnly);
    arg << *mC;

    if (!kapp->dcopClient()->send(ROSEGARDEN_GUI_APP_NAME,
                                  ROSEGARDEN_GUI_IFACE_NAME,
                                 "showVisuals(Rosegarden::MappedComposition)",
                                  data))
    {
        SEQUENCER_DEBUG << "RosegardenSequencer::showVisuals()"
                        << " - can't call RosegardenGUI client"
                        << endl;
    }
}

// Keep playing our fetched events, only top up the queued events
// once we're within m_fetchLatency of the last fetch.  Make sure
// that we're fetching *past* the end of what we've already fetched
// and ensure that we don't duplicate events fetch unnecessarily
// by incrementing m_lastFetchSongPosition before we do anything.
//
//
bool
RosegardenSequencerApp::keepPlaying()
{
    if (m_songPosition > ( m_lastFetchSongPosition - m_fetchLatency))
    {

        // Check to make sure that we haven't got ahead of the GUI
        // and adjust as necessary (drop some "slices" if you like)
        //
        Rosegarden::RealTime sequencerTime = m_sequencer->getSequencerTime();
        Rosegarden::RealTime dropBoundary = m_lastFetchSongPosition
                                            + m_readAhead + m_readAhead;

        if (sequencerTime > dropBoundary &&  // we've overstepped boundary
            !isLooping() &&                  // we're not looping or recording
            m_transportStatus != RECORDING_MIDI &&
            m_transportStatus != RECORDING_AUDIO)
        {
            // Catch up
            m_lastFetchSongPosition = sequencerTime;

            // Comment on droppage
            //
            Rosegarden::RealTime gapTime = sequencerTime - dropBoundary;
            int gapLength = gapTime.sec * 1000000 + gapTime.usec;
            int sliceSize = m_readAhead.sec * 1000000 + m_readAhead.usec;
            unsigned int slices = 
                (gapLength/sliceSize == 0) ? 1 : gapLength/sliceSize;

            QString plural = "";

            if (slices > 1) plural = QString("S");

            SEQUENCER_DEBUG << "RosegardenSequencerApp::keepPlaying() - "
                            << "GUI COULDN'T SERVICE SLICE REQUEST(S)\n" 
                            << "                                        "
                            << "      -- DROPPED "
                            << slices
                            << " SLICE"
                            << plural
                            <<"! --\n";

//             QByteArray data;
//             QDataStream arg(data, IO_WriteOnly);
        
//             arg << slices;
//             if (!kapp->dcopClient()->send(ROSEGARDEN_GUI_APP_NAME,
//                                           ROSEGARDEN_GUI_IFACE_NAME,
//                                           "skippedSlices(unsigned int)",
//                                           data)) 
//             {
//                 SEQUENCER_DEBUG << "RosegardenSequencer::keepPlaying()"
//                      << " - can't send to RosegardenGUI client" << endl;
//             }
        }

        m_mC.clear();
        m_mC = *fetchEvents(m_lastFetchSongPosition,
                            m_lastFetchSongPosition + m_readAhead,
                            false);

        // Again, process whether we need to or not to keep
        // the Sequencer up-to-date with audio events
        //
        m_sequencer->processEventsOut(m_mC, m_playLatency, false);

        // tell the gui about this slice of events
        notifyVisuals(&m_mC);

        m_lastFetchSongPosition = m_lastFetchSongPosition + m_readAhead;
    }

    return true;
}

// Return current Sequencer time in GUI compatible terms
// remembering that our playback is delayed by m_playLatency
// ticks from our current m_songPosition.
//
void
RosegardenSequencerApp::updateClocks()
{
    // Attempt to send MIDI clock 
    //
    m_sequencer->sendMidiClock(m_playLatency);

    // If we're not playing etc. then that's all we need to do

    if (m_transportStatus != PLAYING &&
        m_transportStatus != RECORDING_MIDI &&
        m_transportStatus != RECORDING_AUDIO)
        return;

    QByteArray data, replyData;
    QCString replyType;
    QDataStream arg(data, IO_WriteOnly);

    Rosegarden::RealTime newPosition = m_sequencer->getSequencerTime();

    // Go around the loop if we've reached the end
    //
    if (isLooping() && newPosition > m_loopEnd + m_playLatency)
    {

        // Remove the loop width from the song position and send
        // this position (minus m_playLatency) to the GUI
        /*
        m_songPosition = newPosition - (m_loopEnd - m_loopStart);
        newPosition = m_songPosition - m_playLatency;
        m_lastFetchSongPosition =
                m_lastFetchSongPosition - (m_loopEnd - m_loopStart);
                */

        // forgetting the fancy stuff brings superior results
        //
        newPosition = m_songPosition = m_lastFetchSongPosition = m_loopStart;

        // Reset playback using this jump
        //
        m_sequencer->resetPlayback(m_loopStart, m_playLatency);


    }
    else
    {
        m_songPosition = newPosition;

        if (m_songPosition > m_sequencer->getStartPosition() + m_playLatency)
            newPosition = newPosition - m_playLatency;
        else
            newPosition = m_sequencer->getStartPosition();
    }

    arg << newPosition.sec;
    arg << newPosition.usec;

//    std::cerr << "Calling setPointerPosition(" << newPosition.sec << "," << newPosition.usec << "," << long(clearToSend) << ")" << std::endl;
    
    if (!kapp->dcopClient()->send(ROSEGARDEN_GUI_APP_NAME,
                      ROSEGARDEN_GUI_IFACE_NAME,
                      "setPointerPosition(long int, long int)",
                      data))
    {
        SEQUENCER_DEBUG << "RosegardenSequencer::updateClocks()"
                        << " - can't send to RosegardenGUI client"
                        << endl;

        // Stop the sequencer so we can see if we can try again later
        //
        stop();
    }
}

void
RosegardenSequencerApp::notifySequencerStatus()
{
    QByteArray data, replyData;
    QCString replyType;
    QDataStream arg(data, IO_WriteOnly);

    arg << (int)m_transportStatus;

    if (!kapp->dcopClient()->send(ROSEGARDEN_GUI_APP_NAME,
                                  ROSEGARDEN_GUI_IFACE_NAME,
                                  "notifySequencerStatus(int)",
                                  data)) 
    {
        SEQUENCER_DEBUG << "RosegardenSequencer::notifySequencerStatus()"
                        << " - can't send to RosegardenGUI client"
                        << endl;

        // Stop the sequencer
        //
        stop();
    }
}

// Sets the Sequencer object and this object to the new time 
// from where playback can continue.
//
//
void
RosegardenSequencerApp::jumpTo(long posSec, long posUsec)
{
    SEQUENCER_DEBUG << "RosegardenSequencerApp::jumpTo(" << posSec << ", " << posUsec << ")\n";

    if (posSec < 0 && posUsec < 0)
        return;

    m_songPosition = m_lastFetchSongPosition =
            Rosegarden::RealTime(posSec, posUsec);

    m_sequencer->resetPlayback(m_songPosition,
                               m_playLatency);
    return;
}

// Send the last recorded MIDI block
//
void
RosegardenSequencerApp::processRecordedMidi()
{

    QByteArray data; //, replyData;
    //QCString replyType;
    QDataStream arg(data, IO_WriteOnly);

    arg << m_sequencer->getMappedComposition(m_playLatency);


    if (!kapp->dcopClient()->send(ROSEGARDEN_GUI_APP_NAME,
                                  ROSEGARDEN_GUI_IFACE_NAME,
                                 "processRecordedMidi(Rosegarden::MappedComposition)",
                                  data/*, replyType, replyData, true*/))
    {
        SEQUENCER_DEBUG << "RosegardenSequencer::processRecordedMidi() - " 
                        <<   "can't call RosegardenGUI client" 
                        << endl;

        // Stop the sequencer
        //
        stop();
    }
}


// Send an update
//
void
RosegardenSequencerApp::processRecordedAudio()
{
    QByteArray data; //, replyData;
    //QCString replyType;
    QDataStream arg(data, IO_WriteOnly);

    Rosegarden::RealTime time = m_sequencer->getSequencerTime();

    // Send out current time and last audio level
    //
    arg << time.sec;
    arg << time.usec;

    if (!kapp->dcopClient()->send(ROSEGARDEN_GUI_APP_NAME,
                                  ROSEGARDEN_GUI_IFACE_NAME,
                                 "processRecordedAudio(long int, long int)",
                                  data/*, replyType, replyData, true*/))
    {
        SEQUENCER_DEBUG << "RosegardenSequencer::processRecordedMidi() - " 
                        <<   "can't call RosegardenGUI client" << endl;

        // Stop the sequencer
        //
        stop();
    }
}


// This method is called during STOPPED or PLAYING operations
// to mop up any async (unexpected) incoming MIDI or Audio events
// and forward them to the GUI for display
//
//
void
RosegardenSequencerApp::processAsynchronousEvents()
{
    QByteArray data;
    QDataStream arg(data, IO_WriteOnly);

    arg << m_sequencer->getMappedComposition(m_playLatency);

    if (!kapp->dcopClient()->send(ROSEGARDEN_GUI_APP_NAME,
                                 ROSEGARDEN_GUI_IFACE_NAME,
                                 "processAsynchronousMidi(Rosegarden::MappedComposition)", data))
    {
        SEQUENCER_DEBUG << "RosegardenSequencer::processAsynchronousEvents() - "
                        << "can't call RosegardenGUI client" << endl;

        // Stop the sequencer so we can see if we can try again later
        //
        stop();
    }

    // Process any pending events (Note Offs or Audio) as part of
    // same procedure.
    //
    m_sequencer->processPending(m_playLatency);
}



int
RosegardenSequencerApp::record(const Rosegarden::RealTime &time,
                               const Rosegarden::RealTime &playLatency,
                               const Rosegarden::RealTime &fetchLatency,
                               const Rosegarden::RealTime &readAhead,
                               int recordMode)
{
    TransportStatus localRecordMode = (TransportStatus) recordMode;

    // For audio recording we need to retrieve the input ports
    // we're connected to from the Studio.
    //
    if (localRecordMode == STARTING_TO_RECORD_MIDI)
    {
        SEQUENCER_DEBUG << "RosegardenSequencerApp::record()"
                        << " - starting to record MIDI" << endl;

        // Get the Sequencer to prepare itself for recording - if
        // this fails we stop.
        //
        if(m_sequencer->record(Rosegarden::RECORD_MIDI) == false)
        {
            stop();
            return 0;
        }

    }
    else if (localRecordMode == STARTING_TO_RECORD_AUDIO)
    {
        SEQUENCER_DEBUG << "RosegardenSequencerApp::record()"
                        << " - starting to record Audio" << endl;

        QByteArray data, replyData;
        QCString replyType;
        QDataStream arg(data, IO_WriteOnly);

        if (!kapp->dcopClient()->call(ROSEGARDEN_GUI_APP_NAME,
                                      ROSEGARDEN_GUI_IFACE_NAME,
                                      "createNewAudioFile()",
                                      data, replyType, replyData, true))
        {
            SEQUENCER_DEBUG << "RosegardenSequencer::record()"
                            << " - can't call RosegardenGUI client"
                            << endl;
        }

        QDataStream reply(replyData, IO_ReadOnly);
        QString audioFileName;
        if (replyType == "QString")
        {
            reply >> audioFileName;
        }
        else
        {
            SEQUENCER_DEBUG << "RosegardenSequencer::record() - "
                            << "unrecognised type returned" << endl;
        }

        // set recording filename
        m_sequencer->setRecordingFilename(std::string(audioFileName.data()));

        // set recording
        if (m_sequencer->record(Rosegarden::RECORD_AUDIO) == false)
        {
            SEQUENCER_DEBUG << "couldn't start recording - "
                            << "perhaps audio file path wrong?"
                            << endl;

            stop();
            return 0;
        }
    }
    else
    {
        // unrecognised type - return a problem
        return 0;
    }

    // Now set the local transport status to the record mode
    //
    //
    m_transportStatus = localRecordMode;

    /*

    // Work out the record latency
    Rosegarden::RealTime recordLatency = playLatency;
    if (m_audioRecordLatency > recordLatency)
        recordLatency = m_audioRecordLatency;
    */

    // Ensure that playback is initialised
    //
    m_sequencer->initialisePlayback(m_songPosition, playLatency);

    return play(time, playLatency, fetchLatency, readAhead);
}

// We receive a starting time from the GUI which we use as the
// basis of our first fetch of events from the GUI core.  Assuming
// this works we set our internal state to PLAYING and go ahead
// and play the piece until we get a signal to stop.
// 
// DCOP wants us to use an int as a return type instead of a bool.
//
int
RosegardenSequencerApp::play(const Rosegarden::RealTime &time,
                             const Rosegarden::RealTime &playLatency, 
                             const Rosegarden::RealTime &fetchLatency,
                             const Rosegarden::RealTime &readAhead)
{
    if (m_transportStatus == PLAYING || m_transportStatus == STARTING_TO_PLAY)
        return true;

    // To play from the given song position sets up the internal
    // play state to "STARTING_TO_PLAY" which is then caught in
    // the main event loop
    //
    m_songPosition = time;

    if (m_transportStatus != RECORDING_MIDI &&
        m_transportStatus != RECORDING_AUDIO &&
        m_transportStatus != STARTING_TO_RECORD_MIDI &&
        m_transportStatus != STARTING_TO_RECORD_AUDIO)
    {
        m_transportStatus = STARTING_TO_PLAY;
    }

    // Set up latencies
    //
    m_playLatency = playLatency;
    m_fetchLatency = fetchLatency;
    m_readAhead = readAhead;
    if (m_readAhead == Rosegarden::RealTime(0,0))
        m_readAhead.sec = 1;

    // Ensure that we have time for audio synchronisation
    //
    /*
    if (m_audioPlayLatency > m_playLatency)
        m_playLatency = m_audioPlayLatency;
        */


    cleanupMmapData();
    
    QDir segmentsDir(m_segmentFilesPath, "segment_*");
    for (unsigned int i = 0; i < segmentsDir.count(); ++i) {
        mmapSegment(m_segmentFilesPath + "/" + segmentsDir[i]);
    }

    m_controlBlockMmapper = new ControlBlockMmapper(KGlobal::dirs()->resourceDirs("tmp").first() + "/rosegarden_control_block");

    initMetaIterator();

    // report
    //
    SEQUENCER_DEBUG << "RosegardenSequencerApp::play() - starting to play"
                    << endl;

    // Test bits
//     m_metaIterator = new MmappedSegmentsMetaIterator(m_mmappedSegments);
//     Rosegarden::MappedComposition testCompo;
//     m_metaIterator->fillCompositionWithEventsUntil(&testCompo,
//                                                    Rosegarden::RealTime(2,0));

//     dumpFirstSegment();

    // keep it simple
    return true;
}

MmappedSegment* RosegardenSequencerApp::mmapSegment(const QString& file)
{
    MmappedSegment* m = new MmappedSegment(file);
    m_mmappedSegments[file] = m;
    return m;
}

void RosegardenSequencerApp::initMetaIterator()
{
    delete m_metaIterator;
    m_metaIterator = new MmappedSegmentsMetaIterator(m_mmappedSegments, m_controlBlockMmapper);
}

void RosegardenSequencerApp::cleanupMmapData()
{
    for(mmappedsegments::iterator i = m_mmappedSegments.begin(); i != m_mmappedSegments.end(); ++i)
        delete i->second;

    m_mmappedSegments.clear();

    delete m_metaIterator;
    m_metaIterator = 0;

    delete m_controlBlockMmapper;
    m_controlBlockMmapper = 0;
}

void RosegardenSequencerApp::remapSegment(const QString& filename)
{
    if (m_transportStatus != PLAYING) return;

    SEQUENCER_DEBUG << "MmappedSegment::remapSegment(" << filename << ")\n";

    MmappedSegment* m = m_mmappedSegments[filename];
    if (m->remap() && m_metaIterator)
        m_metaIterator->resetIteratorForSegment(filename);
}

void RosegardenSequencerApp::addSegment(const QString& filename)
{
    if (m_transportStatus != PLAYING) return;

    SEQUENCER_DEBUG << "MmappedSegment::addSegment(" << filename << ")\n";

    MmappedSegment* m = mmapSegment(filename);

    if (m_metaIterator)
        m_metaIterator->addSegment(m);
}

void RosegardenSequencerApp::deleteSegment(const QString& filename)
{
    if (m_transportStatus != PLAYING) return;

    SEQUENCER_DEBUG << "MmappedSegment::deleteSegment(" << filename << ")\n";

    MmappedSegment* m = m_mmappedSegments[filename];

    if (m_metaIterator)
        m_metaIterator->deleteSegment(m);

    delete m;
}

void RosegardenSequencerApp::closeAllSegments()
{
    SEQUENCER_DEBUG << "MmappedSegment::closeAllSegments()\n";

    for(mmappedsegments::iterator i = m_mmappedSegments.begin();
        i != m_mmappedSegments.end(); ++i) {
        if (m_metaIterator)
            m_metaIterator->deleteSegment(i->second);

        delete i->second;
    }

    m_mmappedSegments.clear();
    
}

void RosegardenSequencerApp::remapControlBlock()
{
    if (m_transportStatus != PLAYING) return;

    SEQUENCER_DEBUG << "MmappedSegment::remapControlBlock()\n";

    m_controlBlockMmapper->refresh();
}

// DCOP Wrapper for play(Rosegarden::RealTime,
//                       Rosegarden::RealTime,
//                       Rosegarden::RealTime)
//
//
int
RosegardenSequencerApp::play(long timeSec,
                             long timeUsec,
                             long playLatencySec,
                             long playLatencyUSec,
                             long fetchLatencySec,
                             long fetchLatencyUSec,
                             long readAheadSec,
                             long readAheadUSec)

{
    return play(Rosegarden::RealTime(timeSec, timeUsec),
                Rosegarden::RealTime(playLatencySec, playLatencyUSec),
                Rosegarden::RealTime(fetchLatencySec, fetchLatencyUSec),
                Rosegarden::RealTime(readAheadSec, readAheadUSec));
}



// Wrapper for record(Rosegarden::RealTime,
//                    Rosegarden::RealTime,
//                    Rosegarden::RealTime,
//                    recordMode);
//
//
int
RosegardenSequencerApp::record(long timeSec,
                               long timeUSec,
                               long playLatencySec,
                               long playLatencyUSec,
                               long fetchLatencySec,
                               long fetchLatencyUSec,
                               long readAheadSec,
                               long readAheadUSec,
                               int recordMode)

{
    return record(Rosegarden::RealTime(timeSec, timeUSec),
                  Rosegarden::RealTime(playLatencySec, playLatencyUSec),
                  Rosegarden::RealTime(fetchLatencySec, fetchLatencyUSec),
                  Rosegarden::RealTime(readAheadSec, readAheadUSec),
                  recordMode);
}


void
RosegardenSequencerApp::setLoop(const Rosegarden::RealTime &loopStart,
                                const Rosegarden::RealTime &loopEnd)
{
    m_loopStart = loopStart;
    m_loopEnd = loopEnd;

    m_sequencer->setLoop(loopStart, loopEnd);
}


void
RosegardenSequencerApp::setLoop(long loopStartSec,
                                long loopStartUSec,
                                long loopEndSec,
                                long loopEndUSec)
{
    setLoop(Rosegarden::RealTime(loopStartSec, loopStartUSec),
            Rosegarden::RealTime(loopEndSec, loopEndUSec));
}


// Return the status of the sound systems (audio and MIDI)
//
unsigned int
RosegardenSequencerApp::getSoundDriverStatus()
{
    return m_sequencer->getSoundDriverStatus();
}


// Add an audio file to the sequencer
int
RosegardenSequencerApp::addAudioFile(const QString &fileName, int id)
{
    return((int)m_sequencer->addAudioFile(fileName.utf8().data(), id));
}

int
RosegardenSequencerApp::removeAudioFile(int id)
{
    return((int)m_sequencer->removeAudioFile(id));
}

void
RosegardenSequencerApp::clearAllAudioFiles()
{
    m_sequencer->clearAudioFiles();
}

void
RosegardenSequencerApp::setMappedInstrument(int type, unsigned char channel,
                                            unsigned int id)
{
    InstrumentId mID = (InstrumentId)id;
    Rosegarden::Instrument::InstrumentType mType = 
        (Rosegarden::Instrument::InstrumentType)type;
    Rosegarden::MidiByte mChannel = (Rosegarden::MidiByte)channel;

    m_sequencer->setMappedInstrument(
            new Rosegarden::MappedInstrument (mType, mChannel, mID));

}

// Process a MappedComposition sent from Sequencer with
// immediate effect
//
void
RosegardenSequencerApp::processSequencerSlice(Rosegarden::MappedComposition mC)
{
    // Use the "now" API
    //
    m_sequencer->processEventsOut(mC, Rosegarden::RealTime(0, 0), true);
}

void
RosegardenSequencerApp::processMappedEvent(unsigned int id,
                                           int type,
                                           unsigned char pitch,
                                           unsigned char velocity,
                                           long absTimeSec,
                                           long absTimeUsec,
                                           long durationSec,
                                           long durationUsec,
                                           long audioStartMarkerSec,
                                           long audioStartMarkerUSec)
{
    Rosegarden::MappedEvent *mE =
        new Rosegarden::MappedEvent(
            (InstrumentId)id,
            (Rosegarden::MappedEvent::MappedEventType)type,
            (Rosegarden::MidiByte)pitch,
            (Rosegarden::MidiByte)velocity,
            Rosegarden::RealTime(absTimeSec, absTimeUsec),
            Rosegarden::RealTime(durationSec, durationUsec),
            Rosegarden::RealTime(audioStartMarkerSec, audioStartMarkerUSec));

    Rosegarden::MappedComposition mC;

    SEQUENCER_DEBUG << "processMappedEvent() - sending out single event"
                    << endl;

    /*
    std::cout << "ID = " << mE->getInstrument() << std::endl;
    std::cout << "TYPE = " << mE->getType() << std::endl;
    std::cout << "D1 = " << (int)mE->getData1() << std::endl;
    std::cout << "D2 = " << (int)mE->getData2() << std::endl;
    */

    mC.insert(mE);

    m_sequencer->processEventsOut(mC, Rosegarden::RealTime(0, 0), true);
}

void
RosegardenSequencerApp::processMappedEvent(Rosegarden::MappedEvent mE)
{
    Rosegarden::MappedComposition mC;
    mC.insert(new Rosegarden::MappedEvent(mE));
    m_sequencer->processEventsOut(mC, Rosegarden::RealTime(0, 0), true);
}

// Get the MappedDevice (DCOP wrapped vector of MappedInstruments)
//
Rosegarden::MappedDevice
RosegardenSequencerApp::getMappedDevice(unsigned int id)
{
    return m_sequencer->getMappedDevice(id);
}

unsigned int
RosegardenSequencerApp::getDevices()
{
    return m_sequencer->getDevices();
}

int
RosegardenSequencerApp::canReconnect(int type)
{
    return m_sequencer->canReconnect(type);
}

unsigned int
RosegardenSequencerApp::addDevice(int type, unsigned int direction)
{
    return m_sequencer->addDevice(type, direction);
}

void
RosegardenSequencerApp::removeDevice(unsigned int deviceId)
{
    m_sequencer->removeDevice(deviceId);
}

unsigned int
RosegardenSequencerApp::getConnections(int type, unsigned int direction)
{
    return m_sequencer->getConnections(type, direction);
}

QString
RosegardenSequencerApp::getConnection(int type, unsigned int direction,
				      unsigned int connectionNo)
{
    return m_sequencer->getConnection(type, direction, connectionNo);
}

void
RosegardenSequencerApp::setConnection(unsigned int deviceId,
				      QString connection)
{
    m_sequencer->setConnection(deviceId, connection);
}

void
RosegardenSequencerApp::sequencerAlive()
{
    if (!kapp->dcopClient()->
        isApplicationRegistered(QCString(ROSEGARDEN_GUI_APP_NAME)))
    {
        SEQUENCER_DEBUG << "RosegardenSequencerApp::sequencerAlive() - "
                        << "waiting for GUI to register" << endl;
        return;
    }

    QByteArray data;

    if (!kapp->dcopClient()->send(ROSEGARDEN_GUI_APP_NAME,
                                  ROSEGARDEN_GUI_IFACE_NAME,
                                 "alive()",
                                  data))
    {
        SEQUENCER_DEBUG << "RosegardenSequencer::alive()"
                        << " - can't call RosegardenGUI client"
                        << endl;
    }

    SEQUENCER_DEBUG << "RosegardenSequencerApp::sequencerAlive() - "
                    << "trying to tell GUI that we're alive" << endl;
}

void
RosegardenSequencerApp::setAudioMonitoring(long value)
{
    bool bValue = (bool)value;
    std::vector<unsigned int> inputPorts;

    if (bValue &&
            m_sequencer->getRecordStatus() == Rosegarden::ASYNCHRONOUS_MIDI)
    {
        m_sequencer->record(Rosegarden::ASYNCHRONOUS_AUDIO);
        SEQUENCER_DEBUG << "RosegardenSequencerApp::setAudioMonitoring - "
                        << "monitoring audio input" << endl;
        return;
    }

    if (bValue == false &&
            m_sequencer->getRecordStatus() == Rosegarden::ASYNCHRONOUS_AUDIO)
    {
        SEQUENCER_DEBUG << "RosegardenSequencerApp::setAudioMonitoring - "
                        << "monitoring MIDI input" << endl;
        m_sequencer->record(Rosegarden::ASYNCHRONOUS_MIDI);
    }
    
}

void
RosegardenSequencerApp::setAudioMonitoringInstrument(unsigned int id)
{
    m_sequencer->setAudioMonitoringInstrument(id);
}


Rosegarden::MappedRealTime
RosegardenSequencerApp::getAudioPlayLatency()
{
    return Rosegarden::MappedRealTime(m_sequencer->getAudioPlayLateny());
}

Rosegarden::MappedRealTime
RosegardenSequencerApp::getAudioRecordLatency()
{
    return Rosegarden::MappedRealTime(m_sequencer->getAudioRecordLateny());
}

// Initialise the virtual studio with a few audio faders and
// create a plugin manager.  For the moment this is pretty
// arbitrary but eventually we'll drive this from the gui
// and rg file "Studio" entries.
//
void
RosegardenSequencerApp::initialiseStudio()
{
    // clear down the studio before we start adding anything
    //
    m_studio->clear();

    // Create a plugin manager
    //
    Rosegarden::MappedAudioPluginManager *pM =
      dynamic_cast<Rosegarden::MappedAudioPluginManager*>(
        m_studio->createObject(
            Rosegarden::MappedObject::AudioPluginManager, true)); // read-only

    if (pM)
        SEQUENCER_DEBUG << "created plugin manager" << endl;

#ifdef HAVE_LADSPA 
    pM->getenvLADSPAPath();
#endif

    // This creates new MappedPlugin objects under the studio
    //
    pM->discoverPlugins(m_studio);
}


void
RosegardenSequencerApp::setMappedProperty(int id,
                                          const QString &property,
                                          float value)
{
    /*
    SEQUENCER_DEBUG << "setProperty: id = " << id
                    << " : property = \"" << property << "\"" << endl;
                    */

    Rosegarden::MappedObject *object = m_studio->getObject(id);

    if (object)
        object->setProperty(property, value);

    /*
    Rosegarden::MappedAudioFader *fader = 
        dynamic_cast<Rosegarden::MappedAudioFader*>(object);

    if (fader)
    {
        if (property == Rosegarden::MappedAudioFader::FaderLevel)
            fader->setLevel(value);
    }
    */
}

void
RosegardenSequencerApp::setMappedProperty(int id, const QString &property,
        Rosegarden::MappedObjectValueList value)
{
    SEQUENCER_DEBUG << "setProperty: id = " << id
                    << " : property list size = \"" << value.size()
                    << "\"" << endl;

    Rosegarden::MappedObject *object = m_studio->getObject(id);

    if (object)
        object->setProperty(property, value);
}


int
RosegardenSequencerApp::getMappedObjectId(int type)
{
    int value = -1;

    Rosegarden::MappedObject *object =
        m_studio->getObjectOfType(
                Rosegarden::MappedObject::MappedObjectType(type));

    if (object)
    {
        value = int(object->getId());
    }

    return value;
}


std::vector<QString>
RosegardenSequencerApp::getPropertyList(int id,
                                        const QString &property)
{
    std::vector<QString> list;

    Rosegarden::MappedObject *object =
        m_studio->getObject(id);

    if (object)
    {
        list = object->getPropertyList(property);
    }

    SEQUENCER_DEBUG << "getPropertyList - return " << list.size()
                    << " items" << endl;

    return list;
}

unsigned int
RosegardenSequencerApp::getSampleRate() const
{
    if (m_sequencer)
        return m_sequencer->getSampleRate();

    return 0;
}

// Creates an object of a type
//
int 
RosegardenSequencerApp::createMappedObject(int type)
{
    Rosegarden::MappedObject *object =
              m_studio->createObject(
                      Rosegarden::MappedObject::MappedObjectType(type), false);

    if (object)
    {
        SEQUENCER_DEBUG << "createMappedObject - type = "
                        << type << ", object id = "
                        << object->getId() << endl;
        return object->getId();
    }

    return 0;
}

// Destroy an object
//
int 
RosegardenSequencerApp::destroyMappedObject(int id)
{
#ifdef HAVE_LADSPA
    Rosegarden::MappedLADSPAPlugin *plugin =
        dynamic_cast<Rosegarden::MappedLADSPAPlugin*>(m_studio->getObject(id));

    if (plugin)
    {
        std::cout << "RosegardenSequencerApp::destroyMappedObject - "
                  << "removing plugin instance" << std::endl;
        m_sequencer->removePluginInstance(plugin->getInstrument(),
                                          plugin->getPosition());
    }
#endif

    return(int(m_studio->destroyObject(Rosegarden::MappedObjectId(id))));
}


void
RosegardenSequencerApp::clearStudio()
{
    SEQUENCER_DEBUG << "clearStudio()" << endl;
    m_sequencer->removePluginInstances();
    m_studio->clearTemporaries();
} 

void
RosegardenSequencerApp::setMappedPort(int pluginId,
                                      unsigned long portId,
                                      float value)
{
    Rosegarden::MappedObject *object =
        m_studio->getObject(pluginId);

#ifdef HAVE_LADSPA

    Rosegarden::MappedLADSPAPlugin *plugin = 
        dynamic_cast<Rosegarden::MappedLADSPAPlugin*>(object);

    if (plugin)
    {
        plugin->setPort(portId, value);
    }


#endif // HAVE_LADSPA

}

void
RosegardenSequencerApp::slotCheckForNewClients()
{
    // Don't do this check if any of these conditions hold
    //
    if (m_transportStatus == PLAYING ||
        m_transportStatus == RECORDING_MIDI ||
        m_transportStatus == RECORDING_AUDIO)
        return;

    if (m_sequencer->checkForNewClients())
    {
        SEQUENCER_DEBUG << "client list changed" << endl;
    }
}



void
RosegardenSequencerApp::setSliceSize(long timeSec, long timeUSec)
{
    int msecs = (timeSec * 1000) + (timeUSec / 1000);
    SEQUENCER_DEBUG << "set slice size = " << msecs << "ms" << endl;

    Rosegarden::RealTime newReadAhead(timeSec, timeUSec);

    m_readAhead = newReadAhead;

    /*
    if (newReadAhead > m_readAhead)
    else // shrinking slice, we have to refetch sooner
    {
        // for the moment we just keep it simple
        m_readAhead = newReadAhead;
    }
    */
}

// Set temporary slice size.  The timer reverts slice size half way
// through this slice.
//
// void
// RosegardenSequencerApp::setTemporarySliceSize(long timeSec, long timeUSec)
// {
//     Rosegarden::RealTime newReadAhead(timeSec, timeUSec);
//     int msecs = (timeSec * 1000) + (timeUSec / 1000);

//     if (m_sliceTimer->isActive())
//     {
//         m_readAhead = newReadAhead;
//         m_sliceTimer->changeInterval(msecs / 2);
//         SEQUENCER_DEBUG << "set temporary slice size = "
//                         << msecs << "ms" << endl;
//     }
//     else
//     {
//         m_oldSliceSize = m_readAhead;
//         m_readAhead = newReadAhead;
//         m_sliceTimer->start(msecs / 2, true);
//         SEQUENCER_DEBUG << "set temporary slice size = "
//                         << msecs << "ms" << endl;
//     }
// }

// void
// RosegardenSequencerApp::slotRevertSliceSize()
// {
//     SEQUENCER_DEBUG << "revert temporary slice size" << endl;
//     m_readAhead = m_oldSliceSize;
//     m_oldSliceSize = Rosegarden::RealTime(0, 0);
// }

// Set the MIDI Clock period in microseconds
//
void
RosegardenSequencerApp::setQuarterNoteLength(long timeSec, long timeUSec)
{
    long usecs =
        long((double(timeSec) * 1000000.0 + double(timeUSec)) / 24.0);

    //SEQUENCER_DEBUG << "sending MIDI clock every " << usecs << " usecs" << endl;
    m_sequencer->setMIDIClockInterval(usecs);
}

QString
RosegardenSequencerApp::getStatusLog()
{
    return m_sequencer->getStatusLog();
}

