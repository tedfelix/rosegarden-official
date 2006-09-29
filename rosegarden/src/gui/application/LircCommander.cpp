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

#ifdef HAVE_LIRC

#include "LircCommander.h"

#include "misc/Debug.h"
#include "LircClient.h"
#include "RosegardenGUIApp.h"
#include <qobject.h>


namespace Rosegarden
{

LircCommander::LircCommander(LircClient *lirc, RosegardenGUIApp *rgGUIApp)
        : QObject()
{
    m_lirc = lirc;
    m_rgGUIApp = rgGUIApp;
    connect(m_lirc, SIGNAL(buttonPressed(char *)), this, SLOT(execute(char *)) );
}

LircCommander::command LircCommander::commands[] =
    {
        { "FORWARD", &LircCommander::f_fastForward },
        { "FORWARDTOEND", &LircCommander::f_fastForwardToEnd },
        { "PLAY", &LircCommander::f_play },
        { "RECORD", &LircCommander::f_record },
        { "REWIND", &LircCommander::f_rewind },
        { "REWINDTOBEGINNING", &LircCommander::f_rewindToBeginning },
        { "STOP", &LircCommander::f_stop },
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
                                    sizeof(commands) / sizeof(struct command), sizeof(struct command),
                                    compareCommandName);

    if (res != NULL) {

        (*this.*res->function)();   // C++ ...

        RG_DEBUG << "LircCommander::execute: done command " << command << endl;
    } else {
        RG_DEBUG << "LircCommander::execute: invoking command " << command
        << " failed (command not defined in LircCommander::commands[])" << endl;
    };
}

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

}

#include "LircCommander.moc"

#endif
