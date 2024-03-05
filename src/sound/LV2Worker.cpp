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

#define RG_MODULE_STRING "[LV2Worker]"
#define RG_NO_DEBUG_PRINT 1

#include "LV2Worker.h"

#include "sound/LV2Utils.h"

#include <QTimer>

#include "misc/Debug.h"

namespace {
    // cppcheck-suppress unusedFunction
    LV2_Worker_Status scheduleWorkC(LV2_Worker_Schedule_Handle handle,
                                    uint32_t                   size,
                                    const void*                data)
    {
        Rosegarden::LV2Utils::PluginPosition* pp =
            (Rosegarden::LV2Utils::PluginPosition*)handle;
        // ??? Will this be a problem when we are going down?
        //     The old code checked for a null LV2Worker pointer in
        //     LV2Utils.
        return Rosegarden::LV2Worker::getInstance()->scheduleWork(size, data, *pp);
    }

    LV2_Worker_Status respondWorkC(LV2_Worker_Respond_Handle handle,
                                   uint32_t size,
                                   const void *data)
    {
        Rosegarden::LV2Utils::PluginPosition* pp =
            (Rosegarden::LV2Utils::PluginPosition*)handle;
        // ??? Will this be a problem when we are going down?
        //     The old code checked for a null LV2Worker pointer in
        //     LV2Utils.
        return Rosegarden::LV2Worker::getInstance()->respondWork(size, data, *pp);
    }

}


namespace Rosegarden
{


LV2Worker::LV2Worker()
{
    RG_DEBUG << "create LV2Worker";
    m_workTimer = new QTimer(this);
    connect(m_workTimer, &QTimer::timeout,
            this, &LV2Worker::workTimeUp);
    m_workTimer->start(50);
}

LV2Worker *LV2Worker::getInstance()
{
    static LV2Worker instance;
    return &instance;
}

LV2Worker::~LV2Worker()
{
    RG_DEBUG << "~LV2Worker";
}

decltype(LV2_Worker_Schedule::schedule_work) LV2Worker::getScheduler()
{
    return scheduleWorkC;
}

LV2Worker::WorkerData *
LV2Worker::getResponse(const LV2Utils::PluginPosition& pp)
{
    // Called in the audio thread to get worker responses.

    // LOCK m_responses
    // ??? Deadlock?  We are inside LV2PluginInstance::run()'s LV2Utils
    //     lock().  Can we get out of that lock?
    QMutexLocker responsesLock(&m_responsesMutex);

    //RG_DEBUG << "getResponse called" << pp.instrument << pp.position <<
    //  m_workerResponses.size();

    WorkerQueues::iterator it = m_responses.find(pp);
    if (it == m_responses.end())
        return nullptr;

    WorkerQueue &responseQueue = it->second;
    if (responseQueue.empty())
        return nullptr;

    RG_DEBUG << "getResponse" << pp.instrument << pp.position;

    // COPY response
    // Caller is responsible for delete.
    // ??? Can we avoid the copy with a shared_ptr?
    //     This is a time-critical routine so avoiding a copy would be helpful
    //     if possible.
    WorkerData *response = new WorkerData(responseQueue.front());
    responseQueue.pop();

    return response;
}

LV2_Worker_Status LV2Worker::scheduleWork(uint32_t size,
                                          const void* data,
                                          const LV2Utils::PluginPosition& pp)
{
    // this is called by the plugin in the audio thread

    RG_DEBUG << "scheduleWork called" << pp.instrument << pp.position << size;

    // if we were doing direct rendering we could call work here. In
    // real time processing the work must be queued

    WorkerData job;
    job.size = size;
    job.data = new char[size];
    // COPY.  Probably unavoidable.
    memcpy((void*)job.data, data, size);

    // LOCK m_jobs
    QMutexLocker jobsLock(&m_jobsMutex);

    WorkerQueue &jobQueue = m_jobs[pp];
    jobQueue.push(job);

#ifndef NDEBUG
    for (const WorkerQueues::value_type &pair : m_jobs) {
        const LV2Utils::PluginPosition& ppd = pair.first;
        const WorkerQueue &jqd = pair.second;
        RG_DEBUG << "sched job queue" << ppd.instrument << ppd.position <<
            jqd.size();
    }
#endif

    return LV2_WORKER_SUCCESS;
}

LV2_Worker_Status
LV2Worker::respondWork(uint32_t size,
                       const void* data,
                       const LV2Utils::PluginPosition& pp)
{
    // This is called by the plugin in the worker thread to queue
    // up a response.

    WorkerData response;
    response.size = size;
    response.data = new char[size];
    memcpy((void*)response.data, data, size);

    // LOCK m_responses
    QMutexLocker responsesLock(&m_responsesMutex);

    RG_DEBUG << "respondWork called" << pp.instrument << pp.position <<
        m_responses.size();

    WorkerQueue &responseQueue = m_responses[pp];
    responseQueue.push(response);

    return LV2_WORKER_SUCCESS;
}

void LV2Worker::workTimeUp()
{
    // Worker (UI) thread.

    //RG_DEBUG << "workTimeUp" << m_workerJobs.size();
    LV2Utils* lv2utils = LV2Utils::getInstance();

    // LOCK m_jobs
    QMutexLocker jobsLock(&m_jobsMutex);

    // For each job queue...
    for (WorkerQueues::value_type &pair : m_jobs) {
        const LV2Utils::PluginPosition& pp = pair.first;
        WorkerQueue &jobQueue = pair.second;
        //RG_DEBUG << "job queue" << jq.size();

        // For each entry in the job queue...
        while (!jobQueue.empty()) {
            RG_DEBUG << "work to do" << pp.instrument << pp.position;
            WorkerData &job = jobQueue.front();
            // Send the work back to the plugin on the worker (UI) thread.
            lv2utils->runWork(pp, job.size, job.data, respondWorkC);

            delete[] (char*)job.data;
            jobQueue.pop();
        }
    }
}

void LV2Worker::stop()
{
    m_workTimer->stop();
}


}
