// -*- c-basic-offset: 4 -*-

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

#include "PlayableAudioFile.h"
#include <cassert>

namespace Rosegarden
{

//#define DEBUG_PLAYABLE 1
#define DEBUG_RECORDABLE 1

PlayableAudioFile::SmallFileMap PlayableAudioFile::m_smallFileCache;

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
    m_status(IDLE),
    m_file(0),
    m_audioFile(audioFile),
    m_instrumentId(instrumentId),
//    m_ringBufferThreshold(0),
    m_targetChannels(targetChannels),
    m_targetSampleRate(targetSampleRate),
    m_initialised(false),
    m_runtimeSegmentId(-1),
    m_smallFileIndex(0),
    m_smallFileSize(smallFileSize),
    m_isSmallFile(false),
    m_workBuffer(0),
    m_workBufferSize(0)
{
#ifdef DEBUG_PLAYABLE
    std::cerr << "PlayableAudioFile::PlayableAudioFile - creating " << this << std::endl;
#endif
    initialise(bufferSize);
}


void
PlayableAudioFile::initialise(size_t bufferSize)
{
    if (m_initialised) return;

    //!!! This used to have some randomness to try to ensure that not
    //all simultaneously-starting files would want servicing
    //simultaneously.  We could perhaps restore that feature.

#ifdef DEBUG_PLAYABLE
    std::cerr << "PlayableAudioFile::initialise() " << this << std::endl;
#endif

    SmallFileMap::iterator smfi = m_smallFileCache.find(m_audioFile);

    if (smfi != m_smallFileCache.end()) {

#ifdef DEBUG_PLAYABLE
	std::cerr << "PlayableAudioFile::initialise: Found file in small file cache" << std::endl;
#endif

	++(smfi->second.first);
	m_isSmallFile = true;
	m_file = 0;

    } else {

	m_file = new std::ifstream(m_audioFile->getFilename().c_str(),
				   std::ios::in | std::ios::binary);

	//!!! need to catch this

	//!!! I sometimes see this being thrown for a file that's been
	//played many many times already in this composition. Are we
	//leaking fds?
	if (!*m_file)
	    throw(std::string("PlayableAudioFile - can't open file"));
    }

    if (m_file && m_audioFile->getSize() <= m_smallFileSize) {
	
#ifdef DEBUG_PLAYABLE
	std::cerr << "PlayableAudioFile::initialise: Adding file to small file cache" << std::endl;
#endif

	m_audioFile->scanTo(m_file, RealTime::zeroTime);
	std::string contents = m_audioFile->getSampleFrames
	    (m_file,
	     m_audioFile->getSize() / m_audioFile->getBytesPerFrame());
	m_smallFileCache[m_audioFile] = SmallFileData(1, contents);
	m_isSmallFile = true;
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

    for (int ch = 0; ch < m_targetChannels; ++ch) {
	m_ringBuffers.push_back(new RingBuffer<sample_t>(bufferSize));
    }
    
#ifdef DEBUG_PLAYABLE
    std::cerr << "PlayableAudioFile: created " << bufferSize << "-sample ring buffer" << std::endl;
#endif

//    m_ringBufferThreshold = 
//	(double(rand()) * bufferSize) / (double(RAND_MAX) * 4)
//	+ (bufferSize / 10);
//!!!    m_ringBufferThreshold = bufferSize / 4;

    m_initialised = true;
}

PlayableAudioFile::~PlayableAudioFile()
{
    if (m_file) {
        m_file->close();
        delete m_file;
    }

    for (size_t i = 0; i < m_ringBuffers.size(); ++i) {
	delete m_ringBuffers[i];
    }

    if (m_isSmallFile) {
	SmallFileMap::iterator i = m_smallFileCache.find(m_audioFile);
	if (i != m_smallFileCache.end()) {
	    if (--(i->second.first) == 0) {
#ifdef DEBUG_PLAYABLE
		std::cerr << "PlayableAudioFile::~PlayableAudioFile: Removing file with no other references from small file cache" << std::endl;
#endif
		m_smallFileCache.erase(i);
	    }
	}
    }

    delete[] m_workBuffer;

#ifdef DEBUG_PLAYABLE
    std::cerr << "PlayableAudioFile::~PlayableAudioFile - destroying - " << this << std::endl;
#endif
}

bool
PlayableAudioFile::mlock()
{
    if (!m_initialised) return false;

    bool success = true;
    for (int ch = 0; ch < m_targetChannels; ++ch) {
	if (!m_ringBuffers[ch]->mlock()) {
	    std::cerr << "WARNING: PlayableAudioFile::initialise: couldn't lock buffer into real memory, performance may be impaired" << std::endl;
	    success = false;
	}
    }
    return success;
}
 
bool
PlayableAudioFile::isFinished() const
{
    if (!m_initialised || !m_fileEnded) return false;
    for (int ch = 0; ch < m_targetChannels; ++ch) {
	if (m_ringBuffers[ch]->getReadSpace() > 0) return false;
    }
    return true;
}

bool
PlayableAudioFile::scanTo(const RealTime &time)
{
#ifdef DEBUG_PLAYABLE
    std::cerr << "PlayableAudioFile::scanTo(" << time << ")" << std::endl;
#endif
    m_fileEnded = false; // until we know otherwise -- this flag is an
			 // optimisation, not a reliable record

    if (m_isSmallFile) {
	size_t frames = RealTime::realTime2Frame
	    (time, m_audioFile->getSampleRate());
	m_smallFileIndex = frames * m_audioFile->getBytesPerFrame();

    } else {
	if (m_audioFile) {
	    return m_audioFile->scanTo(m_file, time);
	}
    }
    return false;
}


size_t
PlayableAudioFile::getSampleFramesAvailable()
{
    size_t actual = 0;

    for (int ch = 0; ch < m_targetChannels; ++ch) {
	size_t thisChannel = m_ringBuffers[ch]->getReadSpace();
	if (ch == 0 || thisChannel < actual) actual = thisChannel;
    }

    return actual;
}

size_t
PlayableAudioFile::getSamples(sample_t *destination, int channel, size_t samples)
{
    return m_ringBuffers[channel]->read(destination, samples);
}

size_t
PlayableAudioFile::addSamples(sample_t *destination, int channel, size_t samples)
{
    return m_ringBuffers[channel]->readAdding(destination, samples);
}

size_t
PlayableAudioFile::skipSamples(int channel, size_t samples)
{
    return m_ringBuffers[channel]->skip(samples);
}

PlayableAudioFile::sample_t
PlayableAudioFile::getSample(int channel)
{
    return m_ringBuffers[channel]->readOne();
}

void
PlayableAudioFile::checkSmallFileCache()
{    
    return;//!!!
    if (m_isSmallFile) return; // already done this

    SmallFileMap::iterator smfi = m_smallFileCache.find(m_audioFile);

    if (smfi != m_smallFileCache.end()) {

#ifdef DEBUG_PLAYABLE
	std::cerr << "PlayableAudioFile::checkSmallFileCache: Found file in small file cache" << std::endl;
#endif

	++(smfi->second.first);
	m_isSmallFile = true;

    } else if (m_audioFile->getSize() <= m_smallFileSize) {

#ifdef DEBUG_PLAYABLE
	std::cerr << "PlayableAudioFile::checkSmallFileCache: Adding file to small file cache" << std::endl;
#endif

	m_audioFile->scanTo(m_file, RealTime::zeroTime);
	std::string contents = m_audioFile->getSampleFrames
	    (m_file,
	     m_audioFile->getSize() / m_audioFile->getBytesPerFrame());
	m_smallFileCache[m_audioFile] = SmallFileData(1, contents);
	m_isSmallFile = true;
    }
}


void
PlayableAudioFile::fillBuffers()
{
    if (!m_initialised) {
	std::cerr << "PlayableAudioFile::fillRingBuffer() [async]: not initialised" << std::endl;
	return;
    }

#ifdef DEBUG_PLAYABLE
    std::cerr << "PlayableAudioFile::fillRingBuffer() [async] -- scanning to " << m_startIndex << std::endl;
#endif

    checkSmallFileCache();
    scanTo(m_startIndex);
    updateBuffers();
}

bool
PlayableAudioFile::isBufferable(const RealTime &currentTime)
{
    if (m_startTime >= currentTime) {

	RealTime gap = m_startTime - currentTime;
	if (gap.sec > 0) return false; // nice to be able to forget about .sec

	unsigned int gapFrames =
	    (unsigned int)(double(gap.nsec) * m_targetSampleRate / 1000000000);

	// Note that all ringbuffers have the same size and all are to be
	// reset in fillBuffers, so we can just compare against the first here

//!!!	if (gapFrames > m_ringBuffers[0]->getSize() - m_ringBufferThreshold) {
	if (gapFrames > m_ringBuffers[0]->getSize()) {
	    return false;
	}

    } else {
	RealTime gap = currentTime - m_startTime;
	if (gap + m_startIndex >= m_duration) return false;
    }

    return true;
}

bool
PlayableAudioFile::fillBuffers(const RealTime &currentTime)
{
    if (!m_initialised) {
	std::cerr << "PlayableAudioFile::fillRingBuffer() [timed]: not initialised" << std::endl;
	return false;
    }

    checkSmallFileCache();

#ifdef DEBUG_PLAYABLE
    std::cerr << "PlayableAudioFile::fillRingBuffer(" << currentTime << ") -- my start time is " << m_startTime << std::endl;
#endif

    // We don't bother doing this if we're so far ahead of the audio
    // file's start time that the first data would be after the end of
    // the buffer
//!!! less the buffer threshold.

    //!!! merge in some of the isBufferable code

    if (m_startTime >= currentTime) {

	RealTime gap = m_startTime - currentTime;
	if (gap.sec > 0) return false; // nice to be able to forget about .sec

	// If we see the start of the audio file approaching on the
	// playback horizon, we fill the buffer with zeros until the
	// precise sample where the audio file starts so that it starts
	// at the right time even if that's in the middle of a
	// timeslice.  At this point we need to use the playback
	// sample rate (from JACK or wherever) so that the audio file
	// starts at the right time even if it's then playing at the
	// wrong speed because its own sample rate differs.

	unsigned int gapFrames =
	    (unsigned int)(double(gap.nsec) * m_targetSampleRate / 1000000000);

	// Note that all ringbuffers have the same size and all are to be
	// reset, so we can just compare against the first here

//!!!	if (gapFrames > m_ringBuffers[0]->getSize() - m_ringBufferThreshold) {
	if (gapFrames > m_ringBuffers[0]->getSize()) {
	    return false;
	} else {
#ifdef DEBUG_PLAYABLE
	    std::cerr << "PlayableAudioFile::fillRingBuffer: zeroing " << gapFrames << " samples" << std::endl;
#endif

	    for (int ch = 0; ch < m_targetChannels; ++ch) {
		m_ringBuffers[ch]->reset();
		// fill with space, before writing from the start of the file
		if (gapFrames > 0) m_ringBuffers[ch]->zero(gapFrames);
	    }
	}

	scanTo(m_startIndex);

    } else {

	RealTime gap = currentTime - m_startTime;
	if (gap + m_startIndex >= m_duration) return false;

#ifdef DEBUG_PLAYABLE
	    std::cerr << "PlayableAudioFile::fillRingBuffer: scanning to " << m_startIndex + gap << std::endl;
#endif
	
	// skip to the right point in the file before writing from there
	scanTo(m_startIndex + gap);
    }
    
    // now fill whatever space we have available
    updateBuffers();

    return true;
}

void
PlayableAudioFile::updateBuffers()
{
    if (!m_initialised) return;

    if (m_fileEnded) {
#ifdef DEBUG_PLAYABLE
	std::cerr << "PlayableAudioFile::updateBuffers: at end of file already" << std::endl;
#endif
	return;
    }

    size_t frames = 0;

    for (int ch = 0; ch < m_targetChannels; ++ch) {
	size_t writeSpace = m_ringBuffers[ch]->getWriteSpace();
	if (ch == 0 || writeSpace < frames) frames = writeSpace;
    }

//!!!    if (frames < m_ringBufferThreshold) {
    if (frames == 0) {
#ifdef DEBUG_PLAYABLE
	std::cerr << "PlayableAudioFile::updateBuffers: " << frames << " == 0, ignoring" << std::endl;
//	std::cerr << "PlayableAudioFile::updateBuffers: " << frames << " < " << m_ringBufferThreshold << ", ignoring" << std::endl;
	return;
#endif
    }

#ifdef DEBUG_PLAYABLE
    std::cerr << "PlayableAudioFile::updateBuffers: want " << frames << " frames" << std::endl;
#endif

    int sourceChannels = m_audioFile->getChannels();
    int sourceSampleRate = m_audioFile->getSampleRate();

    bool resample = false;
    size_t fileFrames = frames;

    if (sourceSampleRate != m_targetSampleRate) {
	resample = true;
	fileFrames = size_t(float(frames) * float(sourceSampleRate) /
			    float(m_targetSampleRate));
    }

    const unsigned char *ubuf = 0;

    if (m_isSmallFile) {

	size_t bytes = m_audioFile->getBytesPerFrame() * fileFrames;
	std::string &source = m_smallFileCache[m_audioFile].second;
	
#ifdef DEBUG_PLAYABLE
	std::cerr << "PlayableAudioFile::updateBuffers: looking for "
		  << bytes << " bytes from small file cache" << std::endl;
#endif

	if (m_smallFileIndex >= source.size()) {
	    bytes = 0;
	    m_fileEnded = true;
	} else {
	    if (m_smallFileIndex + bytes >= source.size()) {
		bytes = source.size() - m_smallFileIndex;
		m_fileEnded = true;
	    }
	    ubuf = (const unsigned char *)source.c_str()
		+ m_smallFileIndex;
//		data = source.substr(m_smallFileIndex, bytes);
		
	    m_smallFileIndex += bytes;
	}
	fileFrames = bytes / m_audioFile->getBytesPerFrame();

    } else {

	// get the frames

	try {
#ifdef DEBUG_PLAYABLE
	    std::cerr << "PlayableAudioFile::updateBuffers: asking file for "
		      << fileFrames << " frames" << std::endl;
#endif
	    m_fileBuffer = m_audioFile->getSampleFrames(m_file, fileFrames);
	} catch (std::string e) {
	    // most likely we've run out of data in the file -
	    // we can live with this - just write out what we
	    // have got.
#ifdef DEBUG_PLAYABLE
	    std::cerr << "PlayableAudioFile::updateBuffers - "
		      << e << std::endl;
	    std::cerr << "PlayableAudioFile::updateBuffers: got " << m_fileBuffer.size() << " bytes" << std::endl;
#endif

	    m_fileEnded = true;
	}

	// update frames to the number we actually managed to read
	fileFrames = m_fileBuffer.size() / m_audioFile->getBytesPerFrame();
	ubuf = (const unsigned char *)m_fileBuffer.c_str();
    }

    if (!ubuf) {
#ifdef DEBUG_PLAYABLE
	std::cerr << "PlayableAudioFile::updateBuffers: nothing left to write"  << std::endl;
#endif
	return;
    } else {
#ifdef DEBUG_PLAYABLE
	std::cerr << "PlayableAudioFile::updateBuffers: actually retrieved " << fileFrames << " frames" << std::endl;
#endif
    }	

//!!!    float *buffer = new float[std::max(frames, fileFrames)]; 
    if (std::max(frames, fileFrames) > m_workBufferSize) {
	delete[] m_workBuffer;
	m_workBufferSize = std::max(frames, fileFrames);
	m_workBuffer = new sample_t[m_workBufferSize];
    }
    sample_t *buffer = m_workBuffer;
    
//    const unsigned char *ubuf = (const unsigned char *)data.c_str();

    //!!! How come this code isn't in WAVAudioFile?

    // If we're reading a stereo file onto a mono target, we mix the
    // two channels.  If we're reading mono to stereo, we duplicate
    // the mono channel.  Otherwise if the numbers of channels differ,
    // we just copy across the ones that do match and zero the rest.

    bool reduceToMono = (m_targetChannels == 1 && sourceChannels == 2);

    for (int ch = 0; ch < sourceChannels; ++ch) {

	if (!reduceToMono || ch == 0) {
	    if (ch >= m_targetChannels) break;
	    memset(buffer, 0, frames * sizeof(sample_t));
	}

	switch (getBitsPerSample()) {
	    
	case 8:
	    // WAV stores 8-bit samples unsigned, other sizes signed.
	    for (size_t i = 0; i < fileFrames; ++i) {
		buffer[i] += (float)(ubuf[ch + i * sourceChannels] - 128.0)
		    / 128.0;
	    }
	    break;

	case 16:
	    for (size_t i = 0; i < fileFrames; ++i) {
		// Two's complement little-endian 16-bit integer.
		// We convert endianness (if necessary) but assume 16-bit short.
		unsigned char b2 = ubuf[2 * (ch + i * sourceChannels)];
		unsigned char b1 = ubuf[2 * (ch + i * sourceChannels) + 1];
		unsigned int bits = (b1 << 8) + b2;
		buffer[i] += (float)(short(bits)) / 32767.0;
	    }
	    break;

	case 24:
	    for (size_t i = 0; i < fileFrames; ++i) {
		// Two's complement little-endian 24-bit integer.
		// Again, convert endianness but assume 32-bit int.
		unsigned char b3 = ubuf[2 * (ch + i * sourceChannels)];
		unsigned char b2 = ubuf[2 * (ch + i * sourceChannels) + 1];
		unsigned char b1 = ubuf[2 * (ch + i * sourceChannels) + 2];
		// Rotate 8 bits too far in order to get the sign bit
		// in the right place; this gives us a 32-bit value,
		// hence the larger float divisor
		unsigned int bits = (b1 << 24) + (b2 << 16) + (b3 << 8);
		buffer[i] += (float)(int(bits)) / 2147483647.0;
	    }
	    break;
	    
	default:
	    std::cerr << "PlayableAudioFile::updateBuffers: unsupported " <<
		getBitsPerSample() << "-bit sample size" << std::endl;
	}

	if (resample && (!reduceToMono || ch == 1)) {

	    // resample (very crudely) in-place

	    float ratio = float(sourceSampleRate) / float(m_targetSampleRate);

	    if (m_targetSampleRate > sourceSampleRate) {
		for (size_t i = frames; i > 0; --i) {
		    size_t j = size_t((i-1) * ratio);
		    if (j >= fileFrames) j = fileFrames - 1;
		    buffer[i-1] = buffer[j];
		}
	    } else {
		for (size_t i = 0; i < frames; ++i) {
		    size_t j = size_t(i * ratio);
		    if (j >= fileFrames) j = fileFrames - 1;
		    buffer[i] = buffer[j];
		}
	    }
	}

	if (!reduceToMono) {

	    m_ringBuffers[ch]->write(buffer, frames);

	} else if (ch == 1) {

	    for (size_t i = 0; i < frames; ++i) {
		buffer[i] /= 2;
	    }
	    m_ringBuffers[0]->write(buffer, frames);
	}	    
    }

    // Now deal with any excess target channels

    for (int ch = sourceChannels; ch < m_targetChannels; ++ch) {
	if (ch == 1 && m_targetChannels == 2) {
	    // copy mono to stereo -- we still have the mono buffer handy
	    m_ringBuffers[ch]->write(buffer, frames);
	} else {
	    m_ringBuffers[ch]->zero(frames);
	}
    }

    m_fileBuffer = "";
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

void 
RecordableAudioFile::buffer(const sample_t *data, int channel, size_t frames)
{
    size_t available = m_ringBuffers[channel]->getWriteSpace();

    if (frames > available) {
	std::cerr << "RecordableAudioFile::buffer: buffer maxed out!" << std::endl;
	frames = available;
    }

#ifdef DEBUG_RECORDABLE
    std::cerr << "RecordableAudioFile::buffer: buffering " << frames << " frames on channel " << channel << std::endl;
#endif
    
    m_ringBuffers[channel]->write(data, frames);
}

void
RecordableAudioFile::write()
{
    //!!! The file is assumed to be 16-bit WAV
    
    // not quick, and wasteful of memory

    unsigned char b1, b2;

    // We need the same amount of available data on every channel
    size_t s = 0;
    for (unsigned int ch = 0; ch < m_audioFile->getChannels(); ++ch) {
	size_t available = m_ringBuffers[ch]->getReadSpace();
#ifdef DEBUG_RECORDABLE
	std::cerr << "RecordableAudioFile::write: " << available << " frames available to write on channel " << ch << std::endl;
#endif
	if (ch == 0 || available < s) s = available;
    }
    if (s == 0) return;

    sample_t *buffer = new sample_t[m_audioFile->getChannels() * s];
    for (unsigned int ch = 0; ch < m_audioFile->getChannels(); ++ch) {
	m_ringBuffers[ch]->read(buffer + ch * s, s);
    }

    std::string sbuf;
    for (size_t i = 0; i < s; ++i) {
	for (unsigned int ch = 0; ch < m_audioFile->getChannels(); ++ch) {
	    float sample = buffer[i + ch * s];
	    b2 = (unsigned char)((long)(sample * 32767.0) & 0xff);
	    b1 = (unsigned char)((long)(sample * 32767.0) >> 8);
	    sbuf += b2;
	    sbuf += b1;
	}
    }

#ifdef DEBUG_RECORDABLE
    std::cerr << "RecordableAudioFile::write: writing " << sbuf.length() << " bytes to file" << std::endl;
#endif

    m_audioFile->appendSamples(sbuf);
    delete[] buffer;
}

}

