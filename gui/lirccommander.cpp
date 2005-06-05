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

#include "config.h"

#ifdef HAVE_LIRC

#include <stdlib.h>
#include <string.h>

#include "rosedebug.h"
#include "rosegardengui.h"

#include "lircclient.h"
#include "lirccommander.h"


LircCommander::LircCommander(LircClient *lirc, RosegardenGUIApp *rgGUIApp)
    : QObject()
{
    m_lirc = lirc;
    m_rgGUIApp = rgGUIApp;
    connect(m_lirc, SIGNAL(buttonPressed(char *)), this, SLOT(execute(char *)) );
} 



// the command -> method mapping table
// make sure to sort command strings alphabetically for binary search!
LircCommander::command LircCommander::commands[] = 
{
    { "FORWARD",            &LircCommander::f_fastForward },
    { "FORWARDTOEND",       &LircCommander::f_fastForwardToEnd },
    { "PLAY",               &LircCommander::f_play },
    { "RECORD",             &LircCommander::f_record },
    { "REWIND",             &LircCommander::f_rewind },
    { "REWINDTOBEGINNING",  &LircCommander::f_rewindToBeginning },
    { "STOP",               &LircCommander::f_stop },
};



int LircCommander::compareCommandName(const void *c1, const void *c2)
{
    return (strcmp(((struct command *)c1)->name, ((struct command *)c2)->name));
}

void LircCommander::execute(char *command)
{
    struct command tmp, *res;

    RG_DEBUG << "LircCommander::execute: invoking command " << command << endl;

    // find the function for the name
    tmp.name = command;
    res = (struct command *)bsearch(&tmp, commands,
                  sizeof(commands)/sizeof(struct command), sizeof(struct command),
                  compareCommandName);
    
    if (res != NULL) {
    
        (*this.*res->function)();   // C++ ...
        
        RG_DEBUG <<  "LircCommander::execute: done command " << command << endl;
    } else {
        RG_DEBUG <<  "LircCommander::execute: invoking command " << command
                 <<  " failed (command not defined in LircCommander::commands[])" << endl;
    };
}



// the command functions themselves (thematical ordering preferred when it gets complex)
void LircCommander::f_play()
{
    this->m_rgGUIApp->play();
}

void LircCommander::f_stop()
{
    this->m_rgGUIApp->stop();
}

void LircCommander::f_record()
{
    this->m_rgGUIApp->record();
}

void LircCommander::f_rewind()
{
    this->m_rgGUIApp->rewind();
}

void LircCommander::f_rewindToBeginning()
{
    this->m_rgGUIApp->rewindToBeginning();
}

void LircCommander::f_fastForward()
{
    this->m_rgGUIApp->fastForward();
}

void LircCommander::f_fastForwardToEnd()
{
    this->m_rgGUIApp->fastForwardToEnd();
}

#include "lirccommander.moc"

#endif // HAVE_LIRC
