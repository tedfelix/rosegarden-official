
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2006
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

#ifndef _RG_LIRCCOMMANDER_H_
#define _RG_LIRCCOMMANDER_H_

#include <qobject.h>




namespace Rosegarden
{

class RosegardenGUIApp;
class LircClient;


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



}

#endif
