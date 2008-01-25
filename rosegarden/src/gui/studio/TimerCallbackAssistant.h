
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2008
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>

    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_TIMERCALLBACKASSISTANT_H_
#define _RG_TIMERCALLBACKASSISTANT_H_

#include <qobject.h>




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
