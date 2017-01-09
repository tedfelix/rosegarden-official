
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

#ifndef RG_TIMERCALLBACKASSISTANT_H
#define RG_TIMERCALLBACKASSISTANT_H

#include <QObject>




namespace Rosegarden
{


/* This assistant class is here simply to work around the fact that
   AudioPluginOSCGUI cannot be a QObject because it's only
   conditionally compiled. */

class TimerCallbackAssistant : public QObject
{
    Q_OBJECT

public:
    TimerCallbackAssistant(int ms, void (*callback)(void *data), void *data);
    virtual ~TimerCallbackAssistant();

protected slots:
    void slotCallback();
    
private:
    void (*m_callback)(void *);
    void *m_data;
};


}

#endif
