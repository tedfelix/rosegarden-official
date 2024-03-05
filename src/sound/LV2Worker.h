/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_LV2_WORKER_H
#define RG_LV2_WORKER_H

#include "sound/LV2Utils.h"

#include <QObject>
#include <QMutex>

#include <queue>
#include <map>

class QTimer;


namespace Rosegarden
{


/// LV2 Worker Feature
/**
 * https://lv2plug.in/ns/ext/worker
 *
 * The LV2Worker class provides the LV2 Worker feature which
 * allows plugins to schedule non-real-time tasks in another thread.
 *
 * Work is done on a QTimer, so this runs in the UI thread.
 */
class LV2Worker : public QObject
{
    Q_OBJECT

public:
    // Singleton.
    static LV2Worker *getInstance();

    ~LV2Worker();

    decltype(LV2_Worker_Schedule::schedule_work) getScheduler();

    /// Called by the plugin to schedule non-real-time work to be done.
    /**
     * Called from the audio thread.
     *
     * See workTimeUp() which sends the scheduled work to the plugin from
     * the UI (work) thread.
     */
    LV2_Worker_Status scheduleWork(uint32_t size,
                                   const void* data,
                                   const LV2Utils::PluginPosition& pp);

    /// Called by the plugin from the UI (work) thread to provide responses.
    LV2_Worker_Status respondWork(uint32_t size,
                                  const void* data,
                                  const LV2Utils::PluginPosition& pp);

    /// Job and Response data.
    struct WorkerData
    {
        uint32_t size;
        const void *data;
    };
    /// Called to get responses to send to the plugin on the audio thread.
    WorkerData *getResponse(const LV2Utils::PluginPosition& pp);

    /// Stop the timer which was started on first call to getInstance().
    void stop();

public slots:

    /// Timer handler.
    /**
     * Gets work that was scheduled via scheduleWork() and sends it to
     * the plugin from the UI (work) thread.
     *
     * @see m_workTimer
     */
    void workTimeUp();

private:
    // Singleton.  Use getInstance().
    LV2Worker();

    /// Worker "thread".  Really the UI thread.
    // ??? If we used a new thread, we would avoid blocking the UI
    //     thread.  That should be relatively easy to do once we move
    //     this off the LV2Utils mutex.
    QTimer* m_workTimer;

    // ??? Because all the plugins share these work queues, there will be
    //     unnecessary contention.  We could
    //     move toward finer grain.  A set of mutexes for each plugin
    //     instance.  Move toward LV2PluginInstance having an instance of
    //     LV2Worker.  This should reduce mutex contention with only a small
    //     memory cost.
    //
    //     There is only one audio thread and only one worker thread, so we
    //     should probably analyze the potential contention to see how much
    //     gain there would actually be before moving forward on this.  E.g.
    //     plugins won't be contending with each other on the audio thread.
    //     However, finer grain locking means the worker thread and the audio
    //     thread will be less likely to contend with each other.

    typedef std::queue<WorkerData> WorkerQueue;
    typedef std::map<LV2Utils::PluginPosition, WorkerQueue> WorkerQueues;

    /// Jobs waiting to be run by the plugin in the worker thread.
    /**
     * Jobs are created by the audio thread and consumed by the worker thread.
     */
    WorkerQueues m_jobs;
    // ??? Proposed.  Finer grain than LV2Utils::lock().  Should be better.
    //QMutex m_jobsMutex;

    /// Jobs waiting to be consumed by the plugin in the audio thread.
    /**
     * Responses are created by the worker thread and consumed by the audio
     * thread.
     */
    WorkerQueues m_responses;
    // ??? Proposed.  Finer grain than LV2Utils::lock().  Should be better.
    //QMutex m_responsesMutex;
};


}

#endif
