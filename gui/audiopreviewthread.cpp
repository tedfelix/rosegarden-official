// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2004
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

#include "audiopreviewthread.h"
#include "rosedebug.h"

#include "AudioFileManager.h"

#include <qapplication.h>


AudioPreviewThread::AudioPreviewThread(Rosegarden::AudioFileManager *manager) :
    m_manager(manager),
    m_nextToken(0),
    m_exiting(false)
{
}

void
AudioPreviewThread::run()
{
    RG_DEBUG << "AudioPreviewThread::run entering" << endl;

    while (!m_exiting) {
	if (!process()) break;
	usleep(100000);
    }

    RG_DEBUG << "AudioPreviewThread::run exiting" << endl;
}

void
AudioPreviewThread::finish()
{
    m_exiting = true;
}

bool
AudioPreviewThread::process()
{
    if (m_queue.empty()) return false;

    m_mutex.lock();

    RequestQueue::iterator i = m_queue.begin();

    // i->first is width, which we use only to provide an ordering to
    // ensure we do smaller previews first.  We don't use it here.

    RequestRec &rec = i->second;
    int token = rec.first;
    Request req = rec.second;
    m_queue.erase(i);
    
    m_mutex.unlock();
    
    std::vector<float> results;
    
    try {
	// Requires thread-safe AudioFileManager::getPreview
	results = m_manager->getPreview(req.audioFileId,
					req.audioStartTime,
					req.audioEndTime,
					req.width,
					req.showMinima);
    } catch (std::string e) {
	
	RG_DEBUG << "AudioPreviewThread::process: failed to update preview for audio file " << req.audioFileId << ":\n" << e.c_str() << endl;
	
	// OK, we hope this just means we're still recording -- so
	// restore this one to the queue
	m_mutex.lock();
	m_queue.insert(RequestQueue::value_type(req.width,
						RequestRec(token, req)));
	m_mutex.unlock();
	return true;
    }
    
    m_mutex.lock();
    unsigned int channels = m_manager->getAudioFile(req.audioFileId)->getChannels();
    m_results[token] = ResultsPair(channels, results);
    QObject *notify = req.notify;
    QApplication::postEvent
	(notify,
	 new QCustomEvent(QEvent::Type(QEvent::User + 1), (void *)token));
    
    m_mutex.unlock();

    return true;
}

int
AudioPreviewThread::requestPreview(const Request &request)
{
    m_mutex.lock();

    for (RequestQueue::iterator i = m_queue.begin(); i != m_queue.end(); ++i) {
	if (i->second.second.notify == request.notify) {
	    m_queue.erase(i);
	    break;
	}
    }

    int token = m_nextToken;
    m_queue.insert(RequestQueue::value_type(request.width,
					    RequestRec(token, request)));
    ++m_nextToken;

    m_mutex.unlock();

    if (!running()) start();

    return token;
}

void
AudioPreviewThread::getPreview(int token, unsigned int &channels,
			       std::vector<float> &values)
{
    m_mutex.lock();

    values.clear();
    if (m_results.find(token) == m_results.end()) {
	channels = 0;
	m_mutex.unlock();
	return;
    }

    channels = m_results[token].first;
    values = m_results[token].second;
    m_results.erase(m_results.find(token));

    m_mutex.unlock();

    return;
}

