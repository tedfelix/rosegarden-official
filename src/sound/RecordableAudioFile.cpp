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

#include "RecordableAudioFile.h"

#include <cstdlib>

//#define DEBUG_RECORDABLE 1

namespace Rosegarden
{

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
	return ;
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

	if (ch == 0 || available < s)
	    s = available;
    }
    if (s == 0)
	return ;

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

