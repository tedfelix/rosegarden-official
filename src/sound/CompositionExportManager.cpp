/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[CompositionExportManager]"
#define RG_NO_DEBUG_PRINT 1

#include "CompositionExportManager.h"

#include "misc/Debug.h"
#include "sound/audiostream/AudioWriteStream.h"
#include "sound/audiostream/AudioWriteStreamFactory.h"

namespace Rosegarden
{

CompositionExportManager::CompositionExportManager
(const QString& fileName) :
    m_fileName(fileName),
    m_sampleRate(0),
    m_start(false),
    m_stop(false),
    m_leftChannelBuffer(nullptr),
    m_rightChannelBuffer(nullptr),
    m_running(false),
    m_ws(nullptr)
{
  RG_DEBUG << "ctor" << fileName;
}

void CompositionExportManager::start(int sampleRate)
{
    RG_DEBUG << "start";
    m_sampleRate = sampleRate;
    // create the ring buffers
    m_leftChannelBuffer = new RingBuffer<sample_t>(20000);
    m_rightChannelBuffer = new RingBuffer<sample_t>(20000);

    m_start = true;
}

void CompositionExportManager::stop()
{
    RG_DEBUG << "stop";
    m_stop = true;
}

void CompositionExportManager::addSamples(sample_t *left,
                                          sample_t *right,
                                          size_t numSamples)
{
    RG_DEBUG << "addSamples" << left << right << numSamples;
    if (! m_running) {
        RG_DEBUG << "addSamples not running";
        return;
    }
    size_t spacel = m_leftChannelBuffer->getWriteSpace();
    size_t spacer = m_rightChannelBuffer->getWriteSpace();
    if (spacel < numSamples || spacer < numSamples) {
        RG_WARNING << "export to audio buffer overflow";
        return;
    }
    m_leftChannelBuffer->write(left, numSamples);
    m_rightChannelBuffer->write(right, numSamples);
}

void CompositionExportManager::update()
{
    if (m_running) {
        size_t spacel = m_leftChannelBuffer->getReadSpace();
        size_t spacer = m_rightChannelBuffer->getReadSpace();
        size_t toRead = spacel;
        if (spacer < spacel) toRead = spacer;
        if (toRead > 0) {
            sample_t lbuf[toRead];
            sample_t rbuf[toRead];
            sample_t ileaveBuf[2 * toRead];
            RG_DEBUG << "update read" << toRead;
            m_leftChannelBuffer->read(lbuf, toRead);
            m_rightChannelBuffer->read(rbuf, toRead);
            double ssq = 0.0;
            for (size_t is=0; is<toRead; is++) {
                // interleave
                sample_t s0 = lbuf[is];
                sample_t s1 = rbuf[is];
                ssq += s0 * s0;
                ssq += s1 * s1;
                ileaveBuf[is*2] = s0;
                ileaveBuf[is*2 + 1] = s1;
            }
            RG_DEBUG << "render frames" << toRead << ssq;
            if (m_ws)
                m_ws->putInterleavedFrames(toRead, ileaveBuf);
        }
        if (m_stop) {
            RG_DEBUG << "stop - delete write stream";
            m_running = false;
            delete m_ws;
            m_ws = nullptr;
            delete m_leftChannelBuffer;
            m_leftChannelBuffer = nullptr;
            delete m_rightChannelBuffer;
            m_rightChannelBuffer = nullptr;
        }
    } else {
        if (m_start) {
            m_start = false;
            RG_DEBUG << "update create audio stream";
            m_ws = AudioWriteStreamFactory::createWriteStream
                (m_fileName, 2, m_sampleRate);
            if (!m_ws) {
                RG_WARNING << "Cannot create write stream.";
                return;
            }
            m_running = true;
        }
    }
}

bool CompositionExportManager::isComplete() const
{
    return (m_stop && ! m_running);
}

}
