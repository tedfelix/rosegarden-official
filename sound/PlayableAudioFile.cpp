// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2006
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

#include "PlayableAudioFile.h"
#include <cassert>

namespace Rosegarden
{

//#define DEBUG_RING_BUFFER_POOL 1
//#define DEBUG_PLAYABLE 1
//#define DEBUG_PLAYABLE_READ 1
//#define DEBUG_RECORDABLE 1

class RingBufferPool
{
public:
    typedef float sample_t;

    RingBufferPool(size_t bufferSize);
    virtual ~RingBufferPool();

    /**
     * Set the default size for buffers.  Buffers currently allocated
     * will not be resized until they are returned.
     */
    void setBufferSize(size_t n);

    size_t getBufferSize() const { return m_bufferSize; }

    /**
     * Discard or create buffers as necessary so as to have n buffers
     * in the pool.  This will not discard any buffers that are
     * currently allocated, so if more than n are allocated, more than
     * n will remain.
     */
    void setPoolSize(size_t n);

    size_t getPoolSize() const { return m_buffers.size(); }

    /**
     * Return true if n buffers available, false otherwise.
     */
    bool getBuffers(size_t n, RingBuffer<sample_t> **buffers);

    /**
     * Return a buffer to the pool.
     */
    void returnBuffer(RingBuffer<sample_t> *buffer);

protected:
    // Want to avoid memory allocation when marking a buffer
    // unallocated or allocated, so we use a single container for all

    typedef std::pair<RingBuffer<sample_t> *, bool> AllocPair;
    typedef std::vector<AllocPair> AllocList;
    AllocList m_buffers;

    size_t m_bufferSize;

    pthread_mutex_t m_lock;
};


RingBufferPool::RingBufferPool(size_t bufferSize) :
    m_bufferSize(bufferSize)
{
    pthread_mutex_t initialisingMutex = PTHREAD_MUTEX_INITIALIZER;
    memcpy(&m_lock, &initialisingMutex, sizeof(pthread_mutex_t));
}

RingBufferPool::~RingBufferPool()
{
    size_t allocatedCount = 0;
    for (AllocList::iterator i = m_buffers.begin(); i != m_buffers.end(); ++i) {
	if (i->second) ++allocatedCount;
    }

    if (allocatedCount > 0) {
	std::cerr << "WARNING: RingBufferPool::~RingBufferPool: deleting pool with " << allocatedCount << " allocated buffers" << std::endl;
    }

    for (AllocList::iterator i = m_buffers.begin(); i != m_buffers.end(); ++i) {
	delete i->first;
    }

    m_buffers.clear();

    pthread_mutex_destroy(&m_lock);
}

void
RingBufferPool::setBufferSize(size_t n)
{
    if (m_bufferSize == n) return;

    pthread_mutex_lock(&m_lock);

#ifdef DEBUG_RING_BUFFER_POOL
    std::cerr << "RingBufferPool::setBufferSize: from " << m_bufferSize
	      << " to " << n << std::endl;
    int c = 0;
#endif

    for (AllocList::iterator i = m_buffers.begin(); i != m_buffers.end(); ++i) {
	if (!i->second) {
	    delete i->first;
	    i->first = new RingBuffer<sample_t>(n);
#ifdef DEBUG_RING_BUFFER_POOL
	    std::cerr << "Resized buffer " << c++ << std::endl;
#endif
	} else {
#ifdef DEBUG_RING_BUFFER_POOL
	    std::cerr << "Buffer " << c++ << " is already in use, resizing in place" << std::endl;
#endif
	    i->first->resize(n);
	}
    }

    m_bufferSize = n;
    pthread_mutex_unlock(&m_lock);
}    

void
RingBufferPool::setPoolSize(size_t n)
{
    pthread_mutex_lock(&m_lock);

#ifdef DEBUG_RING_BUFFER_POOL
    std::cerr << "RingBufferPool::setPoolSize: from " << m_buffers.size()
	      << " to " << n << std::endl;
#endif

    size_t allocatedCount = 0, count = 0;

    for (AllocList::iterator i = m_buffers.begin(); i != m_buffers.end(); ++i) {
	if (i->second) ++allocatedCount;
	++count;
    }

    if (count > n) {
	for (AllocList::iterator i = m_buffers.begin(); i != m_buffers.end(); ) {
	    if (!i->second) {
		delete i->first;
		m_buffers.erase(i);
		if (--count == n) break;
	    } else {
		++i;
	    }
	}
    }

    while (count < n) { 
	m_buffers.push_back(AllocPair(new RingBuffer<sample_t>(m_bufferSize),
				      false));
	++count;
    }	

#ifdef DEBUG_RING_BUFFER_POOL
    std::cerr << "RingBufferPool::setPoolSize: have " << m_buffers.size()
	      << " buffers (" << allocatedCount << " allocated)" << std::endl;
#endif

    pthread_mutex_unlock(&m_lock);
}

bool
RingBufferPool::getBuffers(size_t n, RingBuffer<sample_t> **buffers)
{
    pthread_mutex_lock(&m_lock);

    size_t count = 0;

    for (AllocList::iterator i = m_buffers.begin(); i != m_buffers.end(); ++i) {
	if (!i->second && ++count == n) break;
    }

    if (count < n) {
#ifdef DEBUG_RING_BUFFER_POOL
	std::cerr << "RingBufferPool::getBuffers(" << n << "): not available" << std::endl;
#endif
	pthread_mutex_unlock(&m_lock);
	return false;
    }
    count = 0;

#ifdef DEBUG_RING_BUFFER_POOL
    std::cerr << "RingBufferPool::getBuffers(" << n << "): available" << std::endl;
#endif

    for (AllocList::iterator i = m_buffers.begin(); i != m_buffers.end(); ++i) {
	if (!i->second) {
	    i->second = true;
	    i->first->reset();
	    i->first->mlock();
	    buffers[count] = i->first;
	    if (++count == n) break;
	}
    }

    pthread_mutex_unlock(&m_lock);
    return true;
}

void
RingBufferPool::returnBuffer(RingBuffer<sample_t> *buffer)
{
    pthread_mutex_lock(&m_lock);

#ifdef DEBUG_RING_BUFFER_POOL
    std::cerr << "RingBufferPool::returnBuffer" << std::endl;
#endif

    buffer->munlock();

    for (AllocList::iterator i = m_buffers.begin(); i != m_buffers.end(); ++i) {
	if (i->first == buffer) {
	    i->second = false;
	    if (buffer->getSize() != m_bufferSize) {
		delete buffer;
		i->first = new RingBuffer<sample_t>(m_bufferSize);
	    }
	}
    }

    pthread_mutex_unlock(&m_lock);
}


AudioCache PlayableAudioFile::m_smallFileCache;

std::vector<PlayableAudioFile::sample_t *> PlayableAudioFile::m_workBuffers;
size_t PlayableAudioFile::m_workBufferSize = 0;

char *PlayableAudioFile::m_rawFileBuffer;
size_t PlayableAudioFile::m_rawFileBufferSize = 0;

RingBufferPool *PlayableAudioFile::m_ringBufferPool = 0;

PlayableAudioFile::PlayableAudioFile(InstrumentId instrumentId,
                                     AudioFile *audioFile,
                                     const RealTime &startTime,
                                     const RealTime &startIndex,
                                     const RealTime &duration,
				     size_t bufferSize,
				     size_t smallFileSize,
				     int targetChannels,
				     int targetSampleRate) :
    m_startTime(startTime),
    m_startIndex(startIndex),
    m_duration(duration),
    m_file(0),
    m_audioFile(audioFile),
    m_instrumentId(instrumentId),
    m_targetChannels(targetChannels),
    m_targetSampleRate(targetSampleRate),
    m_runtimeSegmentId(-1),
    m_isSmallFile(false),
    m_currentScanPoint(RealTime::zeroTime),
    m_autoFade(false),
    m_fadeInTime(RealTime::zeroTime),
    m_fadeOutTime(RealTime::zeroTime)
{
#ifdef DEBUG_PLAYABLE
    std::cerr << "PlayableAudioFile::PlayableAudioFile - creating " << this << " for instrument " << instrumentId << " with file " << (m_audioFile ? m_audioFile->getShortFilename() : "(none)") << std::endl;
#endif

    if (!m_ringBufferPool) {
	//!!! Problematic -- how do we deal with different playable audio
	// files requiring different buffer sizes?  That shouldn't be the
	// usual case, but it's not unthinkable.
	m_ringBufferPool = new RingBufferPool(bufferSize);
    } else {
	m_ringBufferPool->setBufferSize
	    (std::max(bufferSize, m_ringBufferPool->getBufferSize()));
    }

    initialise(bufferSize, smallFileSize);
}


void
PlayableAudioFile::setRingBufferPoolSizes(size_t n, size_t nframes)
{
    if (!m_ringBufferPool) {
	m_ringBufferPool = new RingBufferPool(nframes);
    } else {
	m_ringBufferPool->setBufferSize
	    (std::max(nframes, m_ringBufferPool->getBufferSize()));
    }
    m_ringBufferPool->setPoolSize(n);
}


void
PlayableAudioFile::initialise(size_t bufferSize, size_t smallFileSize)
{
#ifdef DEBUG_PLAYABLE
    std::cerr << "PlayableAudioFile::initialise() " << this << std::endl;
#endif

    checkSmallFileCache(smallFileSize);

    if (!m_isSmallFile) {

	m_file = new std::ifstream(m_audioFile->getFilename().c_str(),
				   std::ios::in | std::ios::binary);

	//!!! need to catch this

	//!!! I sometimes see this being thrown for a file that's been
	//played many many times already in this composition. Are we
	//leaking fds?
	if (!*m_file)
	    throw(std::string("PlayableAudioFile - can't open file"));
    }

    // Scan to the beginning of the data chunk we need
    //
#ifdef DEBUG_PLAYABLE
    std::cerr << "PlayableAudioFile::initialise - scanning to " << m_startIndex << std::endl;
#endif
    scanTo(m_startIndex);

#ifdef DEBUG_PLAYABLE
    std::cerr << "PlayableAudioFile::initialise: buffer size is " << bufferSize << " frames, file size is " << m_audioFile->getSize() << std::endl;
#endif

    if (m_targetChannels <= 0) m_targetChannels = m_audioFile->getChannels();
    if (m_targetSampleRate <= 0) m_targetSampleRate = m_audioFile->getSampleRate();

    m_ringBuffers = new RingBuffer<sample_t> *[m_targetChannels];
    for (int ch = 0; ch < m_targetChannels; ++ch) {
	m_ringBuffers[ch] = 0;
    }
}

PlayableAudioFile::~PlayableAudioFile()
{
    if (m_file) {
        m_file->close();
        delete m_file;
    }

    returnRingBuffers();
    delete[] m_ringBuffers;

    if (m_isSmallFile) {
	m_smallFileCache.decrementReference(m_audioFile);
    }

#ifdef DEBUG_PLAYABLE
//    std::cerr << "PlayableAudioFile::~PlayableAudioFile - destroying - " << this << std::endl;
#endif
}

void
PlayableAudioFile::returnRingBuffers()
{
    for (int i = 0; i < m_targetChannels; ++i) {
	if (m_ringBuffers[i]) {
	    m_ringBufferPool->returnBuffer(m_ringBuffers[i]);
	    m_ringBuffers[i] = 0;
	}
    }
}

bool
PlayableAudioFile::scanTo(const RealTime &time)
{
#ifdef DEBUG_PLAYABLE_READ
    std::cerr << "PlayableAudioFile::scanTo(" << time << ")" << std::endl;
#endif
    m_fileEnded = false; // until we know otherwise -- this flag is an
			 // optimisation, not a reliable record

    bool ok = false;

    if (m_isSmallFile) {

	m_currentScanPoint = time;
	ok = true;

    } else {

	ok = m_audioFile->scanTo(m_file, time);
	if (ok) m_currentScanPoint = time;
    }

#ifdef DEBUG_PLAYABLE_READ
    std::cerr << "PlayableAudioFile::scanTo(" << time << "): set m_currentScanPoint to " << m_currentScanPoint << std::endl;
#endif

    return ok;
}


size_t
PlayableAudioFile::getSampleFramesAvailable()
{
    size_t actual = 0;
  
    if (m_isSmallFile) {
	size_t cchannels;
	size_t cframes;
	(void)m_smallFileCache.getData(m_audioFile, cchannels, cframes);
	size_t offset = RealTime::realTime2Frame(m_currentScanPoint,
						 m_targetSampleRate);
	if (cframes > offset) return cframes - offset;
	else return 0;
//	return cframes - m_currentFrameOffset;
    }

    for (int ch = 0; ch < m_targetChannels; ++ch) {
	if (!m_ringBuffers[ch]) return 0;
	size_t thisChannel = m_ringBuffers[ch]->getReadSpace();
	if (ch == 0 || thisChannel < actual) actual = thisChannel;
    }

#ifdef DEBUG_PLAYABLE
    std::cerr << "PlayableAudioFile(" << (m_audioFile ? m_audioFile->getShortFilename() : "(none)") << " " << this << ")::getSampleFramesAvailable: have " << actual << std::endl;
#endif

    return actual;
}

size_t
PlayableAudioFile::addSamples(std::vector<sample_t *> &destination,
			      size_t channels, size_t nframes, size_t offset)
{
#ifdef DEBUG_PLAYABLE_READ
    std::cerr << "PlayableAudioFile::addSamples(" << nframes << "): channels " << channels << ", my target channels " << m_targetChannels << std::endl;
#endif

    if (!m_isSmallFile) {

	size_t qty = 0;
	bool done = m_fileEnded;

	for (int ch = 0; ch < int(channels) && ch < m_targetChannels; ++ch) {
	    if (!m_ringBuffers[ch]) return 0; //!!! fatal
	    size_t here = m_ringBuffers[ch]->readAdding(destination[ch] + offset, nframes);
	    if (ch == 0 || here < qty) qty = here;
	    if (done && (m_ringBuffers[ch]->getReadSpace() > 0)) done = false;
	}

	for (int ch = channels; ch < m_targetChannels; ++ch) {
	    m_ringBuffers[ch]->skip(nframes);
	}

	if (done) {
#ifdef DEBUG_PLAYABLE_READ
	    std::cerr << "PlayableAudioFile::addSamples(" << nframes << "): reached end, returning buffers" << std::endl;
#endif
	    returnRingBuffers();
	}

#ifdef DEBUG_PLAYABLE_READ
	std::cerr << "PlayableAudioFile::addSamples(" << nframes << "): returning " << qty << " frames (at least " << (m_ringBuffers[0] ? m_ringBuffers[0]->getReadSpace() : 0) << " remaining)" << std::endl;
#endif
	return qty;

    } else {

	size_t cchannels;
	size_t cframes;
	float **cached = m_smallFileCache.getData(m_audioFile, cchannels, cframes);

	if (!cached) {
	    std::cerr << "WARNING: PlayableAudioFile::addSamples: Failed to find small file in cache" << std::endl;
	    m_isSmallFile = false;
	} else {

	    size_t scanFrame = RealTime::realTime2Frame(m_currentScanPoint,
							m_targetSampleRate);

	    size_t startFrame = scanFrame;
	    size_t endFrame = scanFrame + nframes;
	    if (endFrame >= cframes) m_fileEnded = true;

#ifdef DEBUG_PLAYABLE_READ
	    std::cerr << "PlayableAudioFile::addSamples: it's a small file: want frames " << startFrame << " to " << endFrame << " of " << cframes << std::endl;
#endif
	    
	    // all this could be neater!

	    if (channels == 1 && cchannels == 2) { // mix
		for (size_t i = 0; i < nframes; ++i) {
		    if (scanFrame + i < cframes) {
			destination[0][i + offset] += 
			    cached[0][scanFrame + i] +
			    cached[1][scanFrame + i];
		    }
		}
	    } else {
		for (size_t ch = 0; ch < channels; ++ch) {
		    int sch = ch;
		    if (ch >= cchannels) {
			if (channels == 2 && cchannels == 1) sch = 0;
			else break;
		    } else {
			for (size_t i = 0; i < nframes; ++i) {
			    if (scanFrame + i < cframes) {
				destination[ch][i + offset] +=
				    cached[sch][scanFrame + i];
			    }
			}
		    }
		}
	    }

	    m_currentScanPoint = m_currentScanPoint +
		RealTime::frame2RealTime(nframes, m_targetSampleRate);
	    return nframes;
	}
    }

    return 0;
}

void
PlayableAudioFile::checkSmallFileCache(size_t smallFileSize)
{    
    if (m_smallFileCache.has(m_audioFile)) {

#ifdef DEBUG_PLAYABLE
	std::cerr << "PlayableAudioFile::checkSmallFileCache: Found file in small file cache" << std::endl;
#endif

	m_smallFileCache.incrementReference(m_audioFile);
	m_isSmallFile = true;

    } else if (m_audioFile->getSize() <= smallFileSize) {

	std::ifstream file(m_audioFile->getFilename().c_str(),
			   std::ios::in | std::ios::binary);

	//!!! need to catch this
	if (!file) throw(std::string("PlayableAudioFile - can't open file"));

#ifdef DEBUG_PLAYABLE
	std::cerr << "PlayableAudioFile::checkSmallFileCache: Adding file to small file cache" << std::endl;
#endif

	// We always encache files with their original number of
	// channels (because they might be called for in any channel
	// configuration subsequently) but with the current sample
	// rate, not their original one.

	m_audioFile->scanTo(&file, RealTime::zeroTime);
	std::string contents = m_audioFile->getSampleFrames
	    (&file, m_audioFile->getSize() / m_audioFile->getBytesPerFrame());

	size_t nch = getSourceChannels();
	size_t nframes = contents.length() / getBytesPerFrame();
	if (int(getSourceSampleRate()) != m_targetSampleRate) {
	    nframes = size_t(float(nframes) * float(m_targetSampleRate) /
			     float(getSourceSampleRate()));
	}

	std::vector<sample_t *> samples;
	for (size_t ch = 0; ch < nch; ++ch) {
	    samples.push_back(new sample_t[nframes]);
	}

	if (!m_audioFile->decode((const unsigned char *)contents.c_str(),
				 contents.length(),
				 m_targetSampleRate,
				 nch,
				 nframes,
				 samples)) {
	    std::cerr << "PlayableAudioFile::checkSmallFileCache: failed to decode file" << std::endl;
	} else {
	    sample_t **toCache = new sample_t *[nch];
	    for (size_t ch = 0; ch < nch; ++ch) {
		toCache[ch] = samples[ch];
	    }
	    m_smallFileCache.addData(m_audioFile, nch, nframes, toCache);
	    m_isSmallFile = true;
	}

	file.close();
    }

    if (m_isSmallFile) {
	if (m_file) {
	    m_file->close();
	    delete m_file;
	    m_file = 0;
	}
    }
}


void
PlayableAudioFile::fillBuffers()
{
#ifdef DEBUG_PLAYABLE
    if (m_audioFile) {
	std::cerr << "PlayableAudioFile(" << m_audioFile->getShortFilename() << ")::fillBuffers() [async] -- scanning to " << m_startIndex << std::endl;
    } else {
	std::cerr << "PlayableAudioFile::fillBuffers() [async] -- scanning to " << m_startIndex << std::endl;
    }
#endif

    scanTo(m_startIndex);
    updateBuffers();
}

void
PlayableAudioFile::clearBuffers()
{
    returnRingBuffers();
}

bool
PlayableAudioFile::fillBuffers(const RealTime &currentTime)
{
#ifdef DEBUG_PLAYABLE
    if (!m_isSmallFile) {
	if (m_audioFile) {
	    std::cerr << "PlayableAudioFile(" << m_audioFile->getShortFilename() << " " << this << ")::fillBuffers(" << currentTime << "):\n my start time " << m_startTime << ", start index " << m_startIndex << ", duration " << m_duration << std::endl;
	} else {
	    std::cerr << "PlayableAudioFile::fillBuffers(" << currentTime << "): my start time " << m_startTime << ", start index " << m_startIndex << ", duration " << m_duration << std::endl;
	}
    }
#endif

    if (currentTime > m_startTime + m_duration) {

#ifdef DEBUG_PLAYABLE
	std::cerr << "PlayableAudioFile::fillBuffers: seeking past end, returning buffers" << std::endl;
#endif
	returnRingBuffers();
	return true;
    }

    RealTime scanTime = m_startIndex;

    if (currentTime > m_startTime) {
	scanTime = m_startIndex + currentTime - m_startTime;
    }

//    size_t scanFrames = RealTime::realTime2Frame
//	(scanTime,
//	 m_isSmallFile ? m_targetSampleRate : m_audioFile->getSampleRate());

    if (scanTime != m_currentScanPoint) {
	scanTo(scanTime);
    }
	
    if (!m_isSmallFile) {
	for (int i = 0; i < m_targetChannels; ++i) {
	    if (m_ringBuffers[i]) m_ringBuffers[i]->reset();
	}
	updateBuffers();
    }

    return true;
}

bool
PlayableAudioFile::updateBuffers()
{
    if (m_isSmallFile) return false;

    if (m_fileEnded) {
#ifdef DEBUG_PLAYABLE_READ
	std::cerr << "PlayableAudioFile::updateBuffers: at end of file already" << std::endl;
#endif
	return false;
    }

    if (!m_ringBuffers[0]) {
	// need a buffer: can we get one?
	if (!m_ringBufferPool->getBuffers(m_targetChannels, m_ringBuffers)) {
#ifdef DEBUG_PLAYABLE_READ
	    std::cerr << "PlayableAudioFile::updateBuffers: no ring buffers available" << std::endl;
#endif
	    return false;
	}
    }

    size_t nframes = 0;

    for (int ch = 0; ch < m_targetChannels; ++ch) {
	size_t writeSpace = m_ringBuffers[ch]->getWriteSpace();
	if (ch == 0 || writeSpace < nframes) nframes = writeSpace;
    }

    if (nframes == 0) {
#ifdef DEBUG_PLAYABLE_READ
	std::cerr << "PlayableAudioFile::updateBuffers: frames == 0, ignoring" << std::endl;
#endif
	return false;
    }

#ifdef DEBUG_PLAYABLE_READ
    std::cerr << "PlayableAudioFile::updateBuffers: want " << nframes << " frames" << std::endl;
#endif


    RealTime block = RealTime::frame2RealTime(nframes, m_targetSampleRate);
    if (m_currentScanPoint + block >= m_startIndex + m_duration) {
	block = m_startIndex + m_duration - m_currentScanPoint;
	nframes = RealTime::realTime2Frame(block, m_targetSampleRate);
	m_fileEnded = true;
    }

    size_t fileFrames = nframes;
    if (m_targetSampleRate != int(getSourceSampleRate())) {
	fileFrames = size_t(float(fileFrames) * float(getSourceSampleRate()) /
			    float(m_targetSampleRate));
    }

#ifdef DEBUG_PLAYABLE_READ
    std::cerr << "Want " << fileFrames << " (" << block << ") from file (" << (m_duration + m_startIndex - m_currentScanPoint - block) << " to go)" << std::endl;
#endif

    //!!! need to be doing this in initialise, want to avoid allocations here
    if ((getBytesPerFrame() * fileFrames) > m_rawFileBufferSize) {
	delete[] m_rawFileBuffer;
	m_rawFileBufferSize = getBytesPerFrame() * fileFrames;
#ifdef DEBUG_PLAYABLE_READ
	std::cerr << "Expanding raw file buffer to " << m_rawFileBufferSize << " chars" << std::endl;
#endif
	m_rawFileBuffer = new char[m_rawFileBufferSize];
    }

    size_t obtained =
	m_audioFile->getSampleFrames(m_file, m_rawFileBuffer, fileFrames);

#ifdef DEBUG_PLAYABLE
    std::cerr << "requested " << fileFrames << " frames from file for " << nframes << " frames, got " << obtained << " frames" << std::endl;
#endif


    if (nframes > m_workBufferSize) {

	for (size_t i = 0; i < m_workBuffers.size(); ++i) {
	    delete[] m_workBuffers[i];
	}

	m_workBuffers.clear();
	m_workBufferSize = nframes;
#ifdef DEBUG_PLAYABLE_READ
	std::cerr << "Expanding work buffer to " << m_workBufferSize << " frames" << std::endl;
#endif
	for (int i = 0; i < m_targetChannels; ++i) {
	    m_workBuffers.push_back(new sample_t[m_workBufferSize]);
	}

    } else {

	while (m_targetChannels > m_workBuffers.size()) {
	    m_workBuffers.push_back(new sample_t[m_workBufferSize]);
	}
    }

    if (m_audioFile->decode((const unsigned char *)m_rawFileBuffer,
			    obtained * getBytesPerFrame(),
			    m_targetSampleRate,
			    m_targetChannels,
			    nframes,
			    m_workBuffers,
			    false)) {

	if (obtained < fileFrames) m_fileEnded = true;

/*!!! No -- GUI and notification side of things isn't up to this yet,
      so comment it out just in case

	if (m_autoFade) {

	    if (m_currentScanPoint < m_startIndex + m_fadeInTime) {

		size_t fadeSamples =
		    RealTime::realTime2Frame(m_fadeInTime, getTargetSampleRate());
		size_t originSamples =
		    RealTime::realTime2Frame(m_currentScanPoint - m_startIndex,
					     getTargetSampleRate());

		for (size_t i = 0; i < nframes; ++i) {
		    if (i + originSamples > fadeSamples) {
			break;
		    }
		    float gain = float(i + originSamples) / float(fadeSamples);
		    for (int ch = 0; ch < m_targetChannels; ++ch) {
			m_workBuffers[ch][i] *= gain;
		    }
		}
	    }

	    if (m_currentScanPoint + block > 
		m_startIndex + m_duration - m_fadeOutTime) {
		
		size_t fadeSamples =
		    RealTime::realTime2Frame(m_fadeOutTime, getTargetSampleRate());
		size_t originSamples = // counting from end
		    RealTime::realTime2Frame
		    (m_startIndex + m_duration - m_currentScanPoint,
		     getTargetSampleRate());

		for (size_t i = 0; i < nframes; ++i) {
		    float gain = 1.0;
		    if (originSamples < i) gain = 0.0;
		    else {
			size_t fromEnd = originSamples - i;
			if (fromEnd < fadeSamples) {
			    gain = float(fromEnd) / float(fadeSamples);
			}
		    }
		    for (int ch = 0; ch < m_targetChannels; ++ch) {
			m_workBuffers[ch][i] *= gain;
		    }
		}
	    }
	}
*/

	m_currentScanPoint = m_currentScanPoint + block;

	for (int ch = 0; ch < m_targetChannels; ++ch) {
	    m_ringBuffers[ch]->write(m_workBuffers[ch], nframes);
	}
    }

    return true;
}


// How many channels in the base AudioFile?
//
unsigned int
PlayableAudioFile::getSourceChannels()
{
    if (m_audioFile)
    {
        return m_audioFile->getChannels();
    }
    return 0;
}

unsigned int
PlayableAudioFile::getTargetChannels()
{
    return m_targetChannels;
}

unsigned int
PlayableAudioFile::getBytesPerFrame()
{
    if (m_audioFile)
    {
        return m_audioFile->getBytesPerFrame();
    }
    return 0;
}

unsigned int
PlayableAudioFile::getSourceSampleRate()
{
    if (m_audioFile)
    {
        return m_audioFile->getSampleRate();
    }
    return 0;
}

unsigned int
PlayableAudioFile::getTargetSampleRate()
{
    return m_targetSampleRate;
}


// How many bits per sample in the base AudioFile?
//
unsigned int
PlayableAudioFile::getBitsPerSample()
{
    if (m_audioFile) {
        return m_audioFile->getBitsPerSample();
    }
    return 0;
}


// ------------- RecordableAudioFile -------------
//
//
RecordableAudioFile::RecordableAudioFile(AudioFile *audioFile,
					 size_t bufferSize) :
    m_audioFile(audioFile),
    m_status(IDLE)
{
    for (unsigned int ch = 0; ch < audioFile->getChannels(); ++ch) {

	m_ringBuffers.push_back(new RingBuffer<sample_t>(bufferSize));

	if (!m_ringBuffers[ch]->mlock()) {
	    std::cerr << "WARNING: RecordableAudioFile::initialise: couldn't lock buffer into real memory, performance may be impaired" << std::endl;
	}
    }
}

RecordableAudioFile::~RecordableAudioFile()
{
    write();
    m_audioFile->close();
    delete m_audioFile;

    for (size_t i = 0; i < m_ringBuffers.size(); ++i) {
	delete m_ringBuffers[i];
    }
}

size_t
RecordableAudioFile::buffer(const sample_t *data, int channel, size_t frames)
{
    if (channel >= int(m_ringBuffers.size())) {
	std::cerr << "RecordableAudioFile::buffer: No such channel as "
		  << channel << std::endl;
	return 0;
    }

    size_t available = m_ringBuffers[channel]->getWriteSpace();

    if (frames > available) {
	std::cerr << "RecordableAudioFile::buffer: buffer maxed out!" << std::endl;
	frames = available;
    }

#ifdef DEBUG_RECORDABLE
    std::cerr << "RecordableAudioFile::buffer: buffering " << frames << " frames on channel " << channel << std::endl;
#endif
    
    m_ringBuffers[channel]->write(data, frames);
    return frames;
}

void
RecordableAudioFile::write()
{
    // Use a static buffer -- this obviously requires that write() is
    // only called from a single thread
    static size_t bufferSize = 0;
    static sample_t *buffer = 0;
    static char *encodeBuffer = 0;

    unsigned int bits = m_audioFile->getBitsPerSample();

    if (bits != 16 && bits != 32) {
	std::cerr << "ERROR: RecordableAudioFile::write: file has " << bits
		  << " bits per sample; only 16 or 32 are supported" << std::endl;
	return;
    }

    unsigned int channels = m_audioFile->getChannels();
    unsigned char b1, b2;

    // We need the same amount of available data on every channel
    size_t s = 0;
    for (unsigned int ch = 0; ch < channels; ++ch) {
	size_t available = m_ringBuffers[ch]->getReadSpace();
#ifdef DEBUG_RECORDABLE
	std::cerr << "RecordableAudioFile::write: " << available << " frames available to write on channel " << ch << std::endl;
#endif
	if (ch == 0 || available < s) s = available;
    }
    if (s == 0) return;

    size_t bufferReqd = channels * s;
    if (bufferReqd > bufferSize) {
	if (buffer) {
	    buffer = (sample_t *)realloc(buffer, bufferReqd * sizeof(sample_t));
	    encodeBuffer = (char *)realloc(encodeBuffer, bufferReqd * 4);
	} else {
	    buffer = (sample_t *) malloc(bufferReqd * sizeof(sample_t));
	    encodeBuffer = (char *)malloc(bufferReqd * 4);
	}
	bufferSize = bufferReqd;
    }
    
    for (unsigned int ch = 0; ch < channels; ++ch) {
	m_ringBuffers[ch]->read(buffer + ch * s, s);
    }

    // interleave and convert

    if (bits == 16) {
	size_t index = 0;
	for (size_t i = 0; i < s; ++i) {
	    for (unsigned int ch = 0; ch < channels; ++ch) {
		float sample = buffer[i + ch * s];
		b2 = (unsigned char)((long)(sample * 32767.0) & 0xff);
		b1 = (unsigned char)((long)(sample * 32767.0) >> 8);
		encodeBuffer[index++] = b2;
		encodeBuffer[index++] = b1;
	    }
	}
    } else {
	char *encodePointer = encodeBuffer;
	for (size_t i = 0; i < s; ++i) {
	    for (unsigned int ch = 0; ch < channels; ++ch) {
		float sample = buffer[i + ch * s];
		*(float *)encodePointer = sample;
		encodePointer += sizeof(float);
	    }
	}
    }

#ifdef DEBUG_RECORDABLE
    std::cerr << "RecordableAudioFile::write: writing " << s << " frames at " << channels << " channels and " << bits << " bits to file" << std::endl;
#endif

    m_audioFile->appendSamples(encodeBuffer, s);
}

}

