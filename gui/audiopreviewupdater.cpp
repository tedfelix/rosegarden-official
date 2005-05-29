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
                                         CompositionModel::AudioPreviewData& apData, CompositionModelImpl* parent)
    : QObject(parent),
      m_thread(thread),
      m_composition(c),
      m_segment(s),
      m_rect(r),
      m_showMinima(false),
      m_apData(apData),
      m_previewToken(-1)
{
}

bool AudioPreviewUpdater::update()
{
    m_apData.setSegmentRect(m_rect);

    // Get sample start and end times and work out duration
    //
    Rosegarden::RealTime audioStartTime = m_segment->getAudioStartTime();
    Rosegarden::RealTime audioEndTime = audioStartTime +
        m_composition.getElapsedRealTime(m_segment->getEndMarkerTime()) -
        m_composition.getElapsedRealTime(m_segment->getStartTime()) ;

//    RG_DEBUG << "SegmentAudioPreview::updatePreview() - for file id "
//	     << m_segment->getAudioFileId() << " requesting values" <<endl;
    
    AudioPreviewThread::Request request;
    request.audioFileId = m_segment->getAudioFileId();
    request.audioStartTime = audioStartTime;
    request.audioEndTime = audioEndTime;
    request.width = m_rect.width();
    request.showMinima = m_showMinima;
    request.notify = this;
    if (m_previewToken >= 0) m_thread.cancelPreview(m_previewToken);
    m_previewToken = m_thread.requestPreview(request);
}

bool AudioPreviewUpdater::event(QEvent *e)
{
    RG_DEBUG << "AudioPreviewUpdater::event" <<endl;

    if (e->type() == QEvent::User + 1) {
	QCustomEvent *ev = dynamic_cast<QCustomEvent *>(e);
	if (ev) {
	    int token = (int)ev->data();
            unsigned int channels = 0;

	    RG_DEBUG << "AudioPreviewUpdater::token " << token << ", my token " << m_previewToken <<endl;

	    if (m_previewToken >= 0 && token >= m_previewToken) {
		m_previewToken = -1;
		m_thread.getPreview(token, channels, m_apData.getValues());

	    } else {
		// this one is out of date already
		std::vector<float> tmp;
		m_thread.getPreview(token, channels, tmp);
	    }

            m_apData.setChannels(channels);

            emit audioPreviewComplete(this);

	    return true;
	}
    }

    return QObject::event(e);
}
