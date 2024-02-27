/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2022 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_LV2_WORKER_H
#define RG_LV2_WORKER_H

#include <QObject>

#include <queue>
#include <map>

#include "sound/LV2Utils.h"

class QTimer;


namespace Rosegarden
{


/// LV2 Worker Feature
/**
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

    struct WorkerJob
    {
        uint32_t size;
        const void *data;
    };
    WorkerJob *getResponse(const LV2Utils::PluginPosition& pp);

    LV2_Worker_Status scheduleWork(uint32_t size,
                                   const void* data,
                                   const LV2Utils::PluginPosition& pp);
    LV2_Worker_Status respondWork(uint32_t size,
                                  const void* data,
                                  const LV2Utils::PluginPosition& pp);

    /// Stop the timer which was started on first call to getInstance().
    void stop();

public slots:

    void workTimeUp();

private:
    // Singleton.  Use getInstance().
    LV2Worker();

    QTimer* m_workTimer;

    typedef std::queue<WorkerJob> JobQueue;
    typedef std::map<LV2Utils::PluginPosition, JobQueue> JobQueues;
    JobQueues m_workerJobs;
    JobQueues m_workerResponses;
};


}

#endif
