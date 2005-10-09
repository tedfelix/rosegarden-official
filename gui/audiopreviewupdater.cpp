// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2005
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

#include "audiopreviewupdater.h"

#include "audiopreviewthread.h"

AudioPreviewUpdater::AudioPreviewUpdater(AudioPreviewThread &thread,
                                         const Rosegarden::Composition& c, const Rosegarden::Segment* s,
                                         const QRect& r,
                                         CompositionModelImpl* parent)
    : QObject(parent),
      m_thread(thread),
      m_composition(c),
      m_segment(s),
      m_rect(r),
      m_showMinima(false),
      m_channels(0),
      m_previewToken(-1)
{
}

AudioPreviewUpdater::~AudioPreviewUpdater()
{
    RG_DEBUG << "AudioPreviewUpdater::~AudioPreviewUpdater on " << this << " ( token " << m_previewToken << ")" << endl;
    if (m_previewToken >= 0) m_thread.cancelPreview(m_previewToken);
}

void AudioPreviewUpdater::update()
{
    // Get sample start and end times and work out duration
    //
    Rosegarden::RealTime audioStartTime = m_segment->getAudioStartTime();
    Rosegarden::RealTime audioEndTime = audioStartTime +
        m_composition.getElapsedRealTime(m_segment->getEndMarkerTime()) -
        m_composition.getElapsedRealTime(m_segment->getStartTime()) ;

    RG_DEBUG << "AudioPreviewUpdater::update() - for file id "
	     << m_segment->getAudioFileId() << " requesting values - thread running : "
             << m_thread.running() << " - thread finished : " << m_thread.finished() << endl;
    
    AudioPreviewThread::Request request;
    request.audioFileId = m_segment->getAudioFileId();
    request.audioStartTime = audioStartTime;
    request.audioEndTime = audioEndTime;
    request.width = m_rect.width();
    request.showMinima = m_showMinima;
    request.notify = this;
    if (m_previewToken >= 0) m_thread.cancelPreview(m_previewToken);
    m_previewToken = m_thread.requestPreview(request);
    if (!m_thread.running()) m_thread.start();
}

bool AudioPreviewUpdater::event(QEvent *e)
{
    RG_DEBUG << "AudioPreviewUpdater::event (" << this << ")" << endl;

    if (e->type() == AudioPreviewThread::AudioPreviewReady) {
	QCustomEvent *ev = dynamic_cast<QCustomEvent *>(e);
	if (ev) {
	    int token = (int)ev->data();
            m_channels = 0; // to be filled as getPreview return value

	    RG_DEBUG << "AudioPreviewUpdater::token " << token << ", my token " << m_previewToken << endl;

	    if (m_previewToken >= 0 && token >= m_previewToken) {
		m_previewToken = -1;
		m_thread.getPreview(token, m_channels, m_values);

		if (m_channels == 0) {
		    RG_DEBUG << "AudioPreviewUpdater: failed to find preview!\n";
		} else {

		    RG_DEBUG << "AudioPreviewUpdater: got correct preview (" << m_channels
                             << " channels, " << m_values.size() << " samples)\n";
		}

	    } else {

		// this one is out of date already
		std::vector<float> tmp;
		unsigned int tmpChannels;
		m_thread.getPreview(token, tmpChannels, tmp);

		RG_DEBUG << "AudioPreviewUpdater: got obsolete preview (" << tmpChannels
                         << " channels, " << tmp.size() << " samples)\n";
	    }

            emit audioPreviewComplete(this);
	    m_previewToken = -1;

	    return true;
	}
    }

    return QObject::event(e);
}

#include "audiopreviewupdater.moc"
