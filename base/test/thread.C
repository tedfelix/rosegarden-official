// -*- c-basic-offset: 4 -*-
// -*- c-file-style:  "bsd" -*-

// This does some rather shoddy tests on a small selection of core classes.

#include "Lock.h"
#include "Composition.h"

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

    pthread_create(&thread1, 0, writer_thread1, *argv);
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

    while(true)
    {
        cout << "Sleeping" << endl;
        usleep(50000);
    }


};


