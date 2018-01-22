/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
 
    This file is Copyright 2005-2011 Chris Cannam.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "AudioReadStream.h"
#include "misc/Debug.h"
#include "sound/Resampler.h"
#include <cmath>

namespace Rosegarden
{
	
AudioReadStream::FileDRMProtected::FileDRMProtected(QString file) throw() :
    m_file(file)
{
    std::cerr << "ERROR: File is DRM protected: " << file.toStdString() << std::endl;
}

const char *
AudioReadStream::FileDRMProtected::what() const throw()
{
    return QString("File \"%1\" is protected by DRM")
        .arg(m_file).toLocal8Bit().data();
}

AudioReadStream::AudioReadStream() :
    m_channelCount(0),
    m_sampleRate(0),
    m_retrievalRate(0),
    m_resampler(0),
    m_resampleBuffer(0)
{
}

AudioReadStream::~AudioReadStream()
{
    delete m_resampler;
    delete m_resampleBuffer;
}

void
AudioReadStream::setRetrievalSampleRate(size_t rate)
{
    m_retrievalRate = rate;
}

size_t
AudioReadStream::getInterleavedFrames(size_t count, float *frames)
{
    if (m_retrievalRate == 0 ||
        m_retrievalRate == m_sampleRate ||
        m_channelCount == 0) {
        return getFrames(count, frames);
    }

    size_t samples = count * m_channelCount;

    if (!m_resampler) {
        m_resampler = new Resampler(Resampler::Best, m_channelCount);
        m_resampleBuffer = new RingBuffer<float>(samples * 2);
    }

    bool finished = false;

    while (m_resampleBuffer->getReadSpace() < samples && !finished) {

        float ratio = float(m_retrievalRate) / float(m_sampleRate);
        size_t req = size_t(ceil(count / ratio));
        size_t outSz = size_t(ceil(req * ratio));

        float *in  = new float[req * m_channelCount];
        float *out = new float[(outSz + 1) * m_channelCount];   // take one extra space to be sure

        size_t got = getFrames(req, in);
    
        if (got < req) {
            finished = true;
        }

        if (got > 0) {
            int resampled = m_resampler->resampleInterleaved
                (in, out, got, ratio, got < req);

            if (m_resampleBuffer->getWriteSpace() < resampled * m_channelCount) {
                m_resampleBuffer = m_resampleBuffer->resized
                    (m_resampleBuffer->getReadSpace() + resampled * m_channelCount);
            }
        
            m_resampleBuffer->write(out, resampled * m_channelCount);
        }

        delete[] in;
        delete[] out;
    }

    return m_resampleBuffer->read(frames, samples) / m_channelCount;
}

}

