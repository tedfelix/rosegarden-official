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
    while (!m_exiting) {
	process();
	usleep(100000);
    }
}

void
AudioPreviewThread::finish()
{
    m_exiting = true;
}

void
AudioPreviewThread::process()
{
    if (!m_queue.empty()) {
	
	m_mutex.lock();

	RequestQueue::iterator i = m_queue.begin();
	int token = i->first;
	Request req = i->second;
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
	}
	
	m_mutex.lock();
	m_results[token] = results;
	QObject *notify = req.notify;
	QApplication::postEvent
	    (notify,
	     new QCustomEvent(QEvent::Type(QEvent::User + 1), (void *)token));
	m_mutex.unlock();
    }
}

int
AudioPreviewThread::requestPreview(const Request &request)
{
    m_mutex.lock();

    int token = m_nextToken;
    m_queue[token] = request;
    ++m_nextToken;

    m_mutex.unlock();

    return token;
}

void
AudioPreviewThread::getPreview(int token, std::vector<float> &values)
{
    m_mutex.lock();

    values.clear();
    if (m_results.find(token) == m_results.end()) {
	m_mutex.unlock();
	return;
    }

    values = m_results[token];
    m_results.erase(m_results.find(token));

    m_mutex.unlock();

    return;
}

