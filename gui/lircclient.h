// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2005
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    This file is Copyright 2005
        Toni Arnold         <toni__arnold@bluewin.ch>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

// LircClient
// It connects to the lircd daemon and emits a buttonPressed signal when
// an infrared remote button event has been detected.
//
//
#ifndef LIRC_H
#define LIRC_H

#ifdef HAVE_LIRC

#include <qobject.h>


class QSocketNotifier;


class LircClient : public QObject
{
    Q_OBJECT
public:
    LircClient(void);
    ~LircClient();
    
public slots:
    void readButton();
    
signals:
    void buttonPressed(char *);
        
private:
    int                 m_socket;
    QSocketNotifier     *m_socketNotifier;
    struct lirc_config  *m_config;
    char                *m_command;
};


#endif // HAVE_LIRC

#endif // LIRC_H
