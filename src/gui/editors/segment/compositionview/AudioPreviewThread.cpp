/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "AudioPreviewThread.h"
#include "AudioPreviewReadyEvent.h"

#include "base/RealTime.h"
#include "sound/AudioFileManager.h"
#include "sound/PeakFileManager.h"
#include <QApplication>
#include <QEvent>
#include <QMutex>
#include <QObject>
#include <QThread>

#define DEBUG_AUDIO_PREVIEW_THREAD 1

namespace Rosegarden
{


const QEvent::Type AudioPreviewThread::AudioPreviewReady       = QEvent::Type(QEvent::User + 1);
const QEvent::Type AudioPreviewThread::AudioPreviewQueueEmpty  = QEvent::Type(QEvent::User + 2);

AudioPreviewThread::AudioPreviewThread(AudioFileManager *manager) :
        m_manager(manager),
        m_nextToken(0),
        m_exiting(false),
        m_emptyQueueListener(0)
{}

void
AudioPreviewThread::run()
{
    bool emptyQueueSignalled = false;

#ifdef DEBUG_AUDIO_PREVIEW_THREAD

    std::cerr << "AudioPreviewThread::run entering\n";
#endif

    while (!m_exiting) {

        if (m_queue.empty()) {
            if (m_emptyQueueListener && !emptyQueueSignalled) {
                QApplication::postEvent(m_emptyQueueListener,
                                        new QEvent(AudioPreviewQueueEmpty));
                emptyQueueSignalled = true;
            }

            usleep(300000);
        } else {
            process();
        }
    }

#ifdef DEBUG_AUDIO_PREVIEW_THREAD
    std::cerr << "AudioPreviewThread::run exiting\n";
#endif
}

void
AudioPreviewThread::finish()
{
    m_exiting = true;
}

bool
AudioPreviewThread::process()
{
#ifdef DEBUG_AUDIO_PREVIEW_THREAD
    std::cerr << "AudioPreviewThread::process()\n";
#endif

    if (!m_queue.empty()) {

        int failed = 0;
        int inQueue = 0;
        //int count = 0;

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
#ifdef DEBUG_AUDIO_PREVIEW_THREAD
            std::cerr << "AudioPreviewThread::process() file id " << req.audioFileId << std::endl;
#endif

            // Requires thread-safe AudioFileManager::getPreview
            results = m_manager->getPreview(req.audioFileId,
                                            req.audioStartTime,
                                            req.audioEndTime,
                                            req.width,
                                            req.showMinima);
        } catch (AudioFileManager::BadAudioPathException e) {

#ifdef DEBUG_AUDIO_PREVIEW_THREAD
            std::cerr << "AudioPreviewThread::process: failed to update preview for audio file " << req.audioFileId << ": bad audio path: " << e.getMessage() << std::endl;
#endif

            // OK, we hope this just means we're still recording -- so
            // leave this one in the queue
            ++failed;

        } catch (PeakFileManager::BadPeakFileException e) {

#ifdef DEBUG_AUDIO_PREVIEW_THREAD
            std::cerr << "AudioPreviewThread::process: failed to update preview for audio file " << req.audioFileId << ": bad peak file: " << e.getMessage() << std::endl;
#endif

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
            AudioFile *audioFile = m_manager->getAudioFile(req.audioFileId);
            // If there's an audio file to work with
            if (audioFile != NULL) {
                unsigned int channels = audioFile->getChannels();
                m_results[token] = ResultsPair(channels, results);
                QObject *notify = req.notify;
                QApplication::postEvent(notify, new AudioPreviewReadyEvent(token));
            }
        }

        m_mutex.unlock();

        if (failed > 0 && failed == inQueue) {
#ifdef DEBUG_AUDIO_PREVIEW_THREAD
            std::cerr << "AudioPreviewThread::process() - return true\n";
#endif

            return true; // delay and try again
        }
    }

#ifdef DEBUG_AUDIO_PREVIEW_THREAD
    std::cerr << "AudioPreviewThread::process() - return false\n";
#endif

    return false;
}

int
AudioPreviewThread::requestPreview(const Request &request)
{
    m_mutex.lock();

#ifdef DEBUG_AUDIO_PREVIEW_THREAD

    std::cerr << "AudioPreviewThread::requestPreview for file id " << request.audioFileId << ", start " << request.audioStartTime << ", end " << request.audioEndTime << ", width " << request.width << ", notify " << request.notify << std::endl;
#endif 
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

#ifdef DEBUG_AUDIO_PREVIEW_THREAD
    std::cerr << "AudioPreviewThread::requestPreview - token = " << token << std::endl;
#endif

    return token;
}

void
AudioPreviewThread::cancelPreview(int token)
{
    m_mutex.lock();

#ifdef DEBUG_AUDIO_PREVIEW_THREAD

    std::cerr << "AudioPreviewThread::cancelPreview for token " << token << std::endl;
#endif

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
        return ;
    }

    channels = m_results[token].first;
    values = m_results[token].second;
    m_results.erase(m_results.find(token));

    m_mutex.unlock();

    return ;
}


}
