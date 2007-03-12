// -*- c-basic-offset: 4 -*-
// -*- c-file-style:  "bsd" -*-

// This does some rather shoddy tests on a small selection of core classes.

#include "Lock.h"
#include "Composition.h"
#include "Segment.h"
#include "Event.h"

#include <cstdio>
#include <sys/times.h>
#include <iostream>

#include <pthread.h>
#include <unistd.h>

using namespace std;
using namespace Rosegarden;

static void*
writer_thread1(void *arg)
{
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    cout << "write_thread1 - init" << endl;

    Rosegarden::Composition *comp = 
        static_cast<Rosegarden::Composition*>(arg);

    Rosegarden::Composition::segmentcontainer segs = comp->getSegments();
    Rosegarden::Composition::segmentcontainer::iterator it = segs.begin();
    Rosegarden::Segment *segment = *it;

    Rosegarden::timeT insertTime = 50000;
    while (true)
    {
        usleep(90000);
        cout << "LENGTH = " << comp->getNbBars() << endl;
        segment->insert(new Event(Note::EventType, insertTime));
        insertTime += 96;
    }
}

static void*
write_thread2(void *arg)
{
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    cout << "write_thread2 - init" << endl;

    Rosegarden::Composition *comp = 
        static_cast<Rosegarden::Composition*>(arg);

    Rosegarden::Composition::segmentcontainer segs = comp->getSegments();
    Rosegarden::Composition::segmentcontainer::iterator it = segs.begin();
    Rosegarden::Segment *segment = *it;

    Rosegarden::timeT insertTime = 0;
    while (true)
    {
        usleep(50);
        cout << "LENGTH = " << comp->getNbBars() << endl;
        segment->insert(new Event(Note::EventType, insertTime));
        insertTime += 96;
    }
}


int
main(int argc, char **argv)
{
    clock_t st, et;
    struct tms spare;

    cout << "Threading test" << endl;

    pthread_t thread1;
    pthread_t thread2;
    Rosegarden::Composition comp;
    Rosegarden::Segment segment;
    comp.addSegment(&segment);

    if (pthread_create(&thread1, 0, writer_thread1, &comp))
    {
        cerr << "Couldn't start thread 1" << endl;
        exit(1);
    }
    pthread_detach(thread1);

    if (pthread_create(&thread2, 0, write_thread2, &comp))
    {
        cerr << "Couldn't start thread 2" << endl;
        exit(1);
    }
    pthread_detach(thread2);

    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    static Lock lock;

    if (lock.getWriteLock(1))
    {
        cout << "got write lock" << endl;
    }

    if (lock.getWriteLock(0))
    {
        cout << "got second write lock" << endl;
    }
    else
    {
        cout << "couldn't get second write lock" << endl;
    }

    Rosegarden::timeT insertTime = 0;
    while(true)
    {
        usleep(50000);

        cout << "Inserting Event at time " << insertTime << endl;
        segment.insert(new Event(Note::EventType, insertTime));
        insertTime += 96;
    }

};


