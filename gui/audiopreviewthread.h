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

#ifndef AUDIOPREVIEWTHREAD_H
#define AUDIOPREVIEWTHREAD_H

#include <qthread.h>
#include <qmutex.h>

#include <vector>
#include <map>

#include "RealTime.h"

namespace Rosegarden {
    class AudioFileManager;
}

class AudioPreviewThread : public QThread
{
public:
    AudioPreviewThread(Rosegarden::AudioFileManager *manager);
    
    virtual void run();
    virtual void finish();
    
    struct Request {
	int audioFileId;
	Rosegarden::RealTime audioStartTime;
	Rosegarden::RealTime audioEndTime;
	int width;
	bool showMinima;
	QObject *notify;
    };

    virtual int requestPreview(const Request &request);
    virtual void cancelPreview(int token);
    virtual void getPreview(int token, unsigned int &channels,
			    std::vector<float> &values);

protected:
    virtual bool process();

    Rosegarden::AudioFileManager *m_manager;
    int m_nextToken;
    bool m_exiting;

    typedef std::pair<int, Request> RequestRec;
    typedef std::multimap<int, RequestRec> RequestQueue;
    RequestQueue m_queue;

    typedef std::pair<unsigned int, std::vector<float> > ResultsPair;
    typedef std::map<int, ResultsPair> ResultsQueue;
    ResultsQueue m_results;

    QMutex m_mutex;
};

#endif
