/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[AudioPeaksGenerator]"

#include "AudioPeaksGenerator.h"
#include "AudioPeaksThread.h"
#include "AudioPeaksReadyEvent.h"
#include "CompositionModelImpl.h"

#include "misc/Debug.h"
#include "base/Composition.h"
#include "base/RealTime.h"
#include "base/Segment.h"

#include <QEvent>
#include <QObject>
#include <QRect>


namespace Rosegarden
{

static int apuExtantCount = 0;

AudioPeaksGenerator::AudioPeaksGenerator(AudioPeaksThread &thread,
        const Composition& c, const Segment* s,
        const QRect& r,
        CompositionModelImpl* parent)
        : QObject(parent),
        m_thread(thread),
        m_composition(c),
        m_segment(s),
        m_rect(r),
        m_showMinima(false),
        m_channels(0),
        m_token( -1)
{
    ++apuExtantCount;
    RG_DEBUG << "ctor " << this << " (now " << apuExtantCount << " extant)";
}

AudioPeaksGenerator::~AudioPeaksGenerator()
{
    --apuExtantCount;
    RG_DEBUG << "dtor on " << this << " ( token " << m_token << ") (now " << apuExtantCount << " extant)";
    if (m_token >= 0)
        m_thread.cancelPeaks(m_token);
}

void AudioPeaksGenerator::update()
{
    // Get sample start and end times and work out duration
    //
    RealTime audioStartTime = m_segment->getAudioStartTime();
    RealTime audioEndTime = audioStartTime +
                            m_composition.getElapsedRealTime(m_segment->getEndMarkerTime()) -
                            m_composition.getElapsedRealTime(m_segment->getStartTime()) ;

    //RG_DEBUG << "AudioPeaksGenerator(" << this << ")::update() - for file id "
    //         << m_segment->getAudioFileId() << " requesting values - thread running : "
    //         << m_thread.isRunning() << " - thread finished : " << m_thread.isFinished() << endl;

    AudioPeaksThread::Request request;
    request.audioFileId = m_segment->getAudioFileId();
    request.audioStartTime = audioStartTime;
    request.audioEndTime = audioEndTime;
    request.width = m_rect.width();
    request.showMinima = m_showMinima;
    request.notify = this;

    if (m_token >= 0) m_thread.cancelPeaks(m_token);
    m_token = m_thread.requestPeaks(request);

    if (!m_thread.isRunning()) m_thread.start();
}

void AudioPeaksGenerator::cancel()
{
    if (m_token >= 0) m_thread.cancelPeaks(m_token);
    m_token = -1;
}

bool AudioPeaksGenerator::event(QEvent *e)
{
    
    //RG_DEBUG << "AudioPeaksGenerator(" << this << ")::event (" << e << ")";

    if (e->type() == AudioPeaksThread::AudioPeaksReady) {
        AudioPeaksReadyEvent *ev = dynamic_cast<AudioPeaksReadyEvent *>(e);
        if (ev) {
            int token = (int)ev->data();
            m_channels = 0; // to be filled as getComputedValues() return value

            //RG_DEBUG << "AudioPeaksGenerator::token " << token << ", my token " << m_token;

            if (m_token >= 0 && token >= m_token) {

                m_token = -1;
                m_thread.getPeaks(token, m_channels, m_values);
#if 0
                if (m_channels == 0) {
                    RG_DEBUG << "failed to find peaks!\n";
                } else {

                    RG_DEBUG << "got correct peaks (" << m_channels
                             << " channels, " << m_values.size() << " samples)\n";
                }
#endif
                emit audioPeaksComplete(this);

            } else {

                // this one is out of date already
                std::vector<float> tmp;
                unsigned int tmpChannels;
                m_thread.getPeaks(token, tmpChannels, tmp);

                //RG_DEBUG << "got obsolete peaks (" << tmpChannels
                //         << " channels, " << tmp.size() << " samples)\n";
            }

            return true;
        }
    }

    return QObject::event(e);

}

}
