
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

#ifndef RG_AUDIOPEAKSTHREAD_H
#define RG_AUDIOPEAKSTHREAD_H

#include "base/RealTime.h"
#include <map>
#include <QEvent>
#include <QMutex>
#include <QThread>
#include <utility>
#include <vector>


class QObject;


namespace Rosegarden
{


class AudioFileManager;


/// Generate audio peaks asynchronously.
class AudioPeaksThread : public QThread
{
public:
    AudioPeaksThread(AudioFileManager *manager);
    
    virtual void run();
    virtual void finish();
    
    struct Request {
        int audioFileId;
        RealTime audioStartTime;
        RealTime audioEndTime;
        int width;
        bool showMinima;
        QObject *notify;
    };

    virtual int requestPeaks(const Request &request);
    virtual void cancelPeaks(int token);
    virtual void getPeaks(int token, unsigned int &channels,
                            std::vector<float> &values);

    void setEmptyQueueListener(QObject* o) { m_emptyQueueListener = o; }

    static const QEvent::Type AudioPeaksReady;
    static const QEvent::Type AudioPeaksQueueEmpty;
    

protected:
    virtual bool process();


    AudioFileManager *m_manager;
    int m_nextToken;
    bool m_exiting;

    QObject* m_emptyQueueListener;

    typedef std::pair<int, Request> RequestRec;
    typedef std::multimap<int, RequestRec> RequestQueue;
    RequestQueue m_queue;

    typedef std::pair<unsigned int, std::vector<float> > ResultsPair;
    typedef std::map<int, ResultsPair> ResultsQueue;
    ResultsQueue m_results;

    QMutex m_mutex;
};

}

#endif
