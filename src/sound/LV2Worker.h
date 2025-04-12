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
     * See slotWorkTimeUp() which sends the scheduled work back to the
     * plugin from the UI (work) thread.
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
        void *data;
    };
    /// Called to get responses to send to the plugin on the audio thread.
    WorkerData *getResponse(const LV2Utils::PluginPosition& pp);

    /// Stop the timer which was started on first call to getInstance().
    void stop();

private slots:

    /// Timer handler.
    /**
     * Gets work that was scheduled via scheduleWork() and sends it to
     * the plugin from the UI (work) thread.
     *
     * @see m_workTimer
     */
    void slotWorkTimeUp();

private:
    // Singleton.  Use getInstance().
    LV2Worker();

    /// Worker "thread".  Really the UI thread.
    // ??? Consider using a new thread instead of a timer and the UI
    //     thread.  This will avoid blocking the UI.
    QTimer* m_workTimer;

    // ??? Consider giving each LV2PluginInstance its own LV2Worker
    //     instance.  This should reduce contention since each plugin
    //     will have its own queues and mutexes.

    // ??? Consider using a lock-free queue to avoid locking the
    //     audio thread.

    typedef std::queue<WorkerData> WorkerQueue;
    typedef std::map<LV2Utils::PluginPosition, WorkerQueue> WorkerQueues;

    /// Jobs waiting to be run by the plugin in the worker thread.
    /**
     * Jobs are created by the audio thread and consumed by the worker thread.
     */
    WorkerQueues m_jobs;
    QMutex m_jobsMutex;

    /// Responses waiting to be consumed by the plugin in the audio thread.
    /**
     * Responses are created by the worker thread and consumed by the audio
     * thread.
     */
    WorkerQueues m_responses;
    QMutex m_responsesMutex;
};


}

#endif
