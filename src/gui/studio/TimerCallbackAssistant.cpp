/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "TimerCallbackAssistant.h"

#include <QObject>
#include <QTimer>


namespace Rosegarden
{

TimerCallbackAssistant::TimerCallbackAssistant(int ms, void (*callback)(void *data),
        void *data) :
        m_callback(callback),
        m_data(data)
{
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &TimerCallbackAssistant::slotCallback);
    timer->start(ms); // not a single shot timer
}

TimerCallbackAssistant::~TimerCallbackAssistant()
{
    // nothing -- the QTimer is deleted automatically by its parent QObject (me)
}

void
TimerCallbackAssistant::slotCallback()
{
    m_callback(m_data);
}

}
