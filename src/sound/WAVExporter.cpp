/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[WAVExporter]"
#define RG_NO_DEBUG_PRINT 1

#include "WAVExporter.h"

#include "misc/Debug.h"
#include "sound/audiostream/AudioWriteStream.h"
#include "sound/audiostream/AudioWriteStreamFactory.h"
#include "gui/application/RosegardenMainWindow.h"
#include "sequencer/RosegardenSequencer.h"

#include <QMessageBox>


namespace Rosegarden
{


WAVExporter::WAVExporter(const QString& fileName)
{
    RG_DEBUG << "ctor" << fileName;

    const unsigned sampleRate =
            RosegardenSequencer::getInstance()->getSampleRate();

    // Create the output file.
    m_audioWriteStream.reset(AudioWriteStreamFactory::createWriteStream(
            fileName,
            2,  // channelCount
            sampleRate));
    if (!m_audioWriteStream) {
        QMessageBox::information(
                    RosegardenMainWindow::self(),  // parent
                    QObject::tr("Rosegarden"),  // title
                    QObject::tr(
                            "<p>WAV Export</p>"
                            "<p>Unable to create WAV file.</p>"));  // text

        return;
    }

    // create the ring buffers
    m_leftChannelBuffer.reset(new RingBuffer<sample_t>(sampleRate/2));
    m_rightChannelBuffer.reset(new RingBuffer<sample_t>(sampleRate/2));
}

void WAVExporter::start()
{
    if (!m_audioWriteStream)
        return;

    RG_DEBUG << "start";

    m_running = true;
}

void WAVExporter::stop()
{
    RG_DEBUG << "stop";
    m_stopRequested = true;
}

void WAVExporter::addSamples(sample_t *left,
                             sample_t *right,
                             size_t numSamples)
{
    if (!m_audioWriteStream)
        return;
    if (!m_leftChannelBuffer)
        return;
    if (!m_rightChannelBuffer)
        return;

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

void WAVExporter::update()
{
    if (!m_audioWriteStream)
        return;
    if (!m_leftChannelBuffer)
        return;
    if (!m_rightChannelBuffer)
        return;

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
#ifndef NDEBUG
            // Gather samples squared for debugging.
            double ssq = 0.0;
#endif
            // For each interleaved sample...
            for (size_t is=0; is<toRead; is++) {
                // interleave
                sample_t s0 = lbuf[is];
                sample_t s1 = rbuf[is];
#ifndef NDEBUG
                ssq += s0 * s0;
                ssq += s1 * s1;
#endif
                ileaveBuf[is*2] = s0;
                ileaveBuf[is*2 + 1] = s1;
            }
#ifndef NDEBUG
            RG_DEBUG << "render frames" << toRead << ssq;
#endif
            if (m_audioWriteStream)
                m_audioWriteStream->putInterleavedFrames(toRead, ileaveBuf);
        }
        if (m_stopRequested) {
            RG_DEBUG << "stop requested - deleting write stream";

            m_running = false;

            // Free all the memory since we are done.
            m_audioWriteStream = nullptr;
            m_leftChannelBuffer = nullptr;
            m_rightChannelBuffer = nullptr;
        }
    }
}

bool WAVExporter::isComplete() const
{
    // File creation failed?  We're done.
    if (!m_audioWriteStream)
        return true;

    return (m_stopRequested && ! m_running);
}


}
