
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.

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
/**
 * This class is used by AudioPeaksGenerator to generate peaks for each
 * Segment in the Composition.
 */
class AudioPeaksThread : public QThread
{
public:
    AudioPeaksThread(AudioFileManager *manager);

    struct Request {
        int audioFileId;
        RealTime audioStartTime;
        RealTime audioEndTime;
        int width;  // Width of the Segment's QRect.
        bool showMinima;
        QObject *notify;
    };

    /// Add a request for peak generation to the queue.  Returns a token.
    /**
     * As each request is completed, an AudioPeaksReadyEvent is sent to the
     * application's event queue.
     */
    int requestPeaks(const Request &request);
    /// Remove a request for peak generation from the queue.
    void cancelPeaks(int token);
    /// Once peak generation is complete, call this to get the peaks.
    /**
     * This is called in response to an AudioPeaksReadyEvent.
     */
    void getPeaks(int token,
                  unsigned int &channels,
                  std::vector<float> &values);

    /// Connect to receive an AudioPeaksQueueEmpty when the queue is empty.
    /**
     * CompositionView connects to this to find out when all peaks have been
     * generated.  In response, CompositionView redraws the segment previews.
     */
    void setEmptyQueueListener(QObject *o)  { m_emptyQueueListener = o; }
    /// Event sent to the empty queue listener.
    static const QEvent::Type AudioPeaksQueueEmpty;

    /// Stop all peak generation.
    /**
     * Used by RosegardenDocument's dtor to stop all peak generation.
     */
    void finish();

protected:
    // QThread override
    virtual void run();

private:
    bool process();

    AudioFileManager *m_manager;
    int m_nextToken;
    bool m_exiting;

    QObject *m_emptyQueueListener;

    typedef std::pair<int /* token */, Request> RequestRec;
    /// Sorted by width so that the smaller segments are done first.
    typedef std::multimap<int /* width */, RequestRec> RequestQueue;
    RequestQueue m_queue;

    typedef std::pair<unsigned int /* channels */,
                      std::vector<float> /* results */ > ResultsPair;
    typedef std::map<int /* token */, ResultsPair> ResultsQueue;
    ResultsQueue m_results;

    QMutex m_mutex;
};


}

#endif
