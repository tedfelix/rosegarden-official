// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2006
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

// LircCommander
// Any classes that it operates on must be passed via the constructor; 
// currently only RosegardenGUIApp is used.
// It connects to LircClient which invokes methods according to the declared commands
// which in turn are mapped via the ~/.lircrc file to infrared remote buttons.
//
//
#ifndef LIRCCOMMANDER_H
#define LIRCCOMMANDER_H

#ifdef HAVE_LIRC

#include <qobject.h>


class LircClient;
class RosegardenGUIApp;


class LircCommander : public QObject
{
    Q_OBJECT
public:
    LircCommander(LircClient *lirc, RosegardenGUIApp *rgGUIApp);
    
private slots:
    void execute(char *);
    
private:
    LircClient          *m_lirc;
    RosegardenGUIApp    *m_rgGUIApp;
    
    // utilities
    static int compareCommandName(const void *c1, const void *c2);
    
    // commands invoked by lirc
    void f_play();
    void f_stop();
    void f_record();
    void f_rewind();
    void f_rewindToBeginning();
    void f_fastForward();
    void f_fastForwardToEnd();

    // the command -> method mapping table
    static struct command {
            char *name;                         /* command name */
            void (LircCommander::*function)();  /* function to process it */
        } commands[];
};


#endif // HAVE_LIRC

#endif // LIRCCOMMANDER_H
