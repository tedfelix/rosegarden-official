/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.
 
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

#include "LircCommander.h"
#include "LircClient.h"

#ifdef HAVE_LIRC

#include "misc/Debug.h"
#include "document/RosegardenGUIDoc.h"
#include "gui/editors/segment/TrackButtons.h"
#include "RosegardenGUIApp.h"
#include "RosegardenGUIView.h"

#include <QObject>


namespace Rosegarden
{

LircCommander::LircCommander(LircClient *lirc, RosegardenGUIApp *rgGUIApp)
        : QObject()
{
    m_lirc = lirc;
    m_rgGUIApp = rgGUIApp;
    connect(m_lirc, SIGNAL(buttonPressed(char *)),
            this, SLOT(slotExecute(char *)) );

    connect(this, SIGNAL(play()),
            m_rgGUIApp, SLOT(slotPlay()) );
    connect(this, SIGNAL(stop()),
            m_rgGUIApp, SLOT(slotStop()) );
    connect(this, SIGNAL(record()),
            m_rgGUIApp, SLOT(slotRecord()) );
    connect(this, SIGNAL(rewind()),
            m_rgGUIApp, SLOT(slotRewind()) );
    connect(this, SIGNAL(rewindToBeginning()),
            m_rgGUIApp, SLOT(slotRewindToBeginning()) );
    connect(this, SIGNAL(fastForward()),
            m_rgGUIApp, SLOT(slotFastforward()) );
    connect(this, SIGNAL(fastForwardToEnd()),
            m_rgGUIApp, SLOT(slotFastForwardToEnd()) );
    connect(this, SIGNAL(toggleRecord()),
            m_rgGUIApp, SLOT(slotToggleRecord()) );
    connect(this, SIGNAL(trackDown()),
            m_rgGUIApp, SLOT(slotTrackDown()) );
    connect(this, SIGNAL(trackUp()),
            m_rgGUIApp, SLOT(slotTrackUp()) );
    connect(this, SIGNAL(trackMute()),
            m_rgGUIApp, SLOT(slotToggleMutedCurrentTrack()) );
    connect(this, SIGNAL(trackRecord()),
            m_rgGUIApp, SLOT(slotToggleRecordCurrentTrack()) );
}

LircCommander::command LircCommander::commands[] =
    {
        { "FORWARD",            cmd_fastForward },
        { "FORWARDTOEND",       cmd_fastForwardToEnd },
        { "PLAY",               cmd_play },
        { "PUNCHINRECORD",      cmd_toggleRecord },
        { "RECORD",             cmd_record },
        { "REWIND",             cmd_rewind },
        { "REWINDTOBEGINNING",  cmd_rewindToBeginning },
        { "STOP",               cmd_stop },
        { "TRACK+",             cmd_trackUp },
        { "TRACK-",             cmd_trackDown },
        { "TRACK-MUTE",         cmd_trackMute },
        { "TRACK-RECORD",       cmd_trackRecord },
    };


int LircCommander::compareCommandName(const void *c1, const void *c2)
{
    return (strcmp(((struct command *)c1)->name, ((struct command *)c2)->name));
}

void LircCommander::slotExecute(char *command)
{
    struct command tmp, *res;

    RG_DEBUG << "LircCommander::slotExecute: invoking command: " << command << endl;

    // find the function for the name
    tmp.name = command;
    res = (struct command *)bsearch(&tmp, commands,
                                    sizeof(commands) / sizeof(struct command),
                                    sizeof(struct command),
                                    compareCommandName);
    if (res != NULL)
    {
        switch (res->code)
        {
        case cmd_play:
            emit play();
            break;
        case cmd_stop:
            emit stop();
            break;
        case cmd_record:
            emit record();
            break;
        case cmd_rewind:
            emit rewind();
            break;
        case cmd_rewindToBeginning:
            emit rewindToBeginning();
            break;
        case cmd_fastForward:
            emit fastForward();
            break;
        case cmd_fastForwardToEnd:
            emit fastForwardToEnd();
            break;
        case cmd_toggleRecord:
            emit toggleRecord();
            break;
        case cmd_trackDown:
            emit trackDown();
            break;
        case cmd_trackUp:
            emit trackUp();
            break;
        case cmd_trackMute:
            emit trackMute(); 
            break;
        case cmd_trackRecord:
            emit trackRecord(); 
            break;
        default:
            RG_DEBUG <<  "LircCommander::slotExecute: unhandled command " << command << endl;
            return;
        }
        RG_DEBUG <<  "LircCommander::slotExecute: handled command: " << command << endl;
    }
    else
    {
        RG_DEBUG <<  "LircCommander::slotExecute: invoking command: " << command
                 <<  " failed (command not defined in LircCommander::commands[])" << endl;
    };
}

}

#include "LircCommander.moc"

#endif
