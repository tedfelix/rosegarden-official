// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2006
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
    m_exiting(false),
    m_emptyQueueListener(0)
{
}

void
AudioPreviewThread::run()
{
    bool emptyQueueSignalled = false;

    RG_DEBUG << "AudioPreviewThread::run entering\n";

    while (!m_exiting) {

        if (m_queue.empty()) {        
            if (m_emptyQueueListener && !emptyQueueSignalled) {
                QApplication::postEvent(m_emptyQueueListener,
                                        new QCustomEvent(AudioPreviewQueueEmpty, 0));
                emptyQueueSignalled = true;
            }

            usleep(300000);
        } else {
            process();
        }
    }

    RG_DEBUG << "AudioPreviewThread::run exiting\n";
}

void
AudioPreviewThread::finish()
{
    m_exiting = true;
}

bool
AudioPreviewThread::process()
{
    RG_DEBUG << "AudioPreviewThread::process()\n";

    if (!m_queue.empty()) {

	int failed = 0;
	int inQueue = 0;
        int count = 0;

        m_mutex.lock();

        // process 1st request and leave
	inQueue = m_queue.size();
        RequestQueue::iterator i = m_queue.begin();

        // i->first is width, which we use only to provide an ordering to
        // ensure we do smaller previews first.  We don't use it here.

        RequestRec &rec = i->second;
        int token = rec.first;
        Request req = rec.second;
        m_mutex.unlock();
    
        std::vector<float> results;
    
        try {
            RG_DEBUG << "AudioPreviewThread::process() file id " << req.audioFileId << endl;

            // Requires thread-safe AudioFileManager::getPreview
            results = m_manager->getPreview(req.audioFileId,
                                            req.audioStartTime,
                                            req.audioEndTime,
                                            req.width,
                                            req.showMinima);
        } catch (Rosegarden::AudioFileManager::BadAudioPathException e) {
	    
	    RG_DEBUG << "AudioPreviewThread::process: failed to update preview for audio file " << req.audioFileId << ": bad audio path: " << e.getMessage() << endl;
	
            // OK, we hope this just means we're still recording -- so
            // leave this one in the queue
            ++failed;

        } catch (Rosegarden::PeakFileManager::BadPeakFileException e) {
	
	    RG_DEBUG << "AudioPreviewThread::process: failed to update preview for audio file " << req.audioFileId << ": bad peak file: " << e.getMessage() << endl;
	
            // As above
            ++failed;
        }
    
        m_mutex.lock();

        // We need to check that the token is still in the queue
        // (i.e. hasn't been cancelled).  Otherwise we shouldn't notify

        bool found = false;
        for (RequestQueue::iterator i = m_queue.begin(); i != m_queue.end(); ++i) {
            if (i->second.first == token) {
                found = true;
                m_queue.erase(i);
                break;
            }
        }

        if (found) {
            unsigned int channels =
		m_manager->getAudioFile(req.audioFileId)->getChannels();
            m_results[token] = ResultsPair(channels, results);
            QObject *notify = req.notify;
            QApplication::postEvent
                (notify,
                 new QCustomEvent(AudioPreviewReady, (void *)token));
        }

        m_mutex.unlock();

	if (failed > 0 && failed == inQueue) {
            RG_DEBUG << "AudioPreviewThread::process() - return true\n";
	    return true; // delay and try again
	}
    }

    RG_DEBUG << "AudioPreviewThread::process() - return false\n";
    return false;
}

int
AudioPreviewThread::requestPreview(const Request &request)
{
    m_mutex.lock();

    RG_DEBUG << "AudioPreviewThread::requestPreview for file id " << request.audioFileId << ", start " << request.audioStartTime << ", end " << request.audioEndTime << ", width " << request.width << ", notify " << request.notify << endl;
/*!!!
    for (RequestQueue::iterator i = m_queue.begin(); i != m_queue.end(); ++i) {
	if (i->second.second.notify == request.notify) {
	    m_queue.erase(i);
	    break;
	}
    }
*/
    int token = m_nextToken;
    m_queue.insert(RequestQueue::value_type(request.width,
					    RequestRec(token, request)));
    ++m_nextToken;
    m_mutex.unlock();

//     if (!running()) start();

    RG_DEBUG << "AudioPreviewThread::requestPreview : thread running : " << running()
             << " - thread finished : " << finished() << endl;

    RG_DEBUG << "AudioPreviewThread::requestPreview - token = " << token << endl;
    return token;
}

void
AudioPreviewThread::cancelPreview(int token)
{
    m_mutex.lock();

    RG_DEBUG << "AudioPreviewThread::cancelPreview for token " << token << endl;
    
    for (RequestQueue::iterator i = m_queue.begin(); i != m_queue.end(); ++i) {
	if (i->second.first == token) {
	    m_queue.erase(i);
	    break;
	}
    }

    m_mutex.unlock();
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

const QEvent::Type AudioPreviewThread::AudioPreviewReady       = QEvent::Type(QEvent::User + 1);
const QEvent::Type AudioPreviewThread::AudioPreviewQueueEmpty  = QEvent::Type(QEvent::User + 2);

