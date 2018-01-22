
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

    This file is Copyright 2005
        Toni Arnold         <toni__arnold@bluewin.ch>

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_LIRCCLIENT_H
#define RG_LIRCCLIENT_H

#ifdef HAVE_LIRC

#include <QObject>
#include <lirc/lirc_client.h>

class QSocketNotifier;


namespace Rosegarden
{



class LircClient : public QObject
{
    Q_OBJECT
public:
    LircClient(void);
    ~LircClient();
    
public slots:
    void readButton();
    
signals:
    void buttonPressed(const char *);
        
private:
    int                 m_socket;
    QSocketNotifier     *m_socketNotifier;
    struct lirc_config  *m_config;
    char                *m_command;
};



}

#endif

#endif
