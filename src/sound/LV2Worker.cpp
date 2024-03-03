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
    // ??? LOCK m_workerResponses
    //QMutexLocker responsesLock(&m_responsesMutex);
    // ??? But are we inside a lock for m_workerJobs at this point?

    // called in the audio thread
    //RG_DEBUG << "getResponse called" << pp.instrument << pp.position <<
    //  m_workerResponses.size();
    auto it = m_responses.find(pp);
    if (it == m_responses.end()) return nullptr;
    WorkerQueue& jq = (*it).second;
    if (jq.empty()) return nullptr;
    RG_DEBUG << "getResponse" << pp.instrument << pp.position;
    WorkerData *jobcopy = new WorkerData(jq.front()); // copy pointer
    jq.pop();
    return jobcopy;
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
    memcpy((void*)job.data, data, size);

    // ??? LOCK m_workerJobs
    //QMutexLocker jobsLock(&m_jobsMutex);
    // ??? There is no lock in here right now.  So there is a data race on
    //     m_workerJobs.

    WorkerQueue& jq = m_jobs[pp];
    jq.push(job);

    for(auto& pair : m_jobs) {
        const LV2Utils::PluginPosition& ppd = pair.first;
        WorkerQueue& jqd = pair.second;
        RG_DEBUG << "sched job queue" << ppd.instrument << ppd.position <<
            jqd.size();
    }

    return LV2_WORKER_SUCCESS;
}

LV2_Worker_Status LV2Worker::respondWork(uint32_t size,
                                         const void* data,
                                         const LV2Utils::PluginPosition& pp)
{
    // this is called by the plugin in the non audio thread

    WorkerData job;
    job.size = size;
    job.data = new char[size];
    memcpy((void*)job.data, data, size);

    LV2Utils* lv2utils = LV2Utils::getInstance();

    // ??? LOCK m_workerResponses
    //QMutexLocker responsesLock(&m_responsesMutex);

    RG_DEBUG << "respondWork called" << pp.instrument << pp.position <<
        m_responses.size();

    lv2utils->lock();
    WorkerQueue& jq = m_responses[pp];
    jq.push(job);
    lv2utils->unlock();

    return LV2_WORKER_SUCCESS;
}

void LV2Worker::workTimeUp()
{
    //RG_DEBUG << "workTimeUp" << m_workerJobs.size();
    LV2Utils* lv2utils = LV2Utils::getInstance();

    // ??? LV2Worker should have its own mutexes.  It shouldn't
    //     use LV2Utils's.
    lv2utils->lock();

    // ??? LOCK m_workerJobs
    //QMutexLocker jobsLock(&m_jobsMutex);

    // For each job queue...
    for(WorkerQueues::value_type &pair : m_jobs) {
        const LV2Utils::PluginPosition& pp = pair.first;
        WorkerQueue& jq = pair.second;
        //RG_DEBUG << "job queue" << jq.size();

        // For each entry in the job queue...
        while(! jq.empty()) {
            RG_DEBUG << "work to do" << pp.instrument << pp.position;
            WorkerData &job = jq.front();
            // call work
            // ??? This reads m_pluginInstanceData, so we might need to
            //     lock within.  But that's it.
            lv2utils->runWork(pp, job.size, job.data, respondWorkC);

            delete[] (char*)job.data;
            jq.pop();
        }
    }

    lv2utils->unlock();
}

void LV2Worker::stop()
{
    m_workTimer->stop();
}


}
