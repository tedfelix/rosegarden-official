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

#define RG_MODULE_STRING "[LircCommander]"

#include "LircCommander.h"
#include "LircClient.h"

#ifdef HAVE_LIRC

#include "misc/Debug.h"
#include "document/RosegardenDocument.h"
#include "gui/editors/segment/TrackButtons.h"
#include "RosegardenMainWindow.h"
#include "RosegardenMainViewWidget.h"
#include "document/CommandHistory.h"

#include <QObject>
#include <unistd.h>

namespace Rosegarden
{

LircCommander::LircCommander(LircClient *lirc, RosegardenMainWindow *rgGUIApp)
        : QObject()
{
    m_lirc = lirc;
    m_rgGUIApp = rgGUIApp;
    connect(m_lirc, &LircClient::buttonPressed,
            this, &LircCommander::slotExecute );

    connect(this, &LircCommander::play,
            m_rgGUIApp, &RosegardenMainWindow::slotPlay );
    connect(this, &LircCommander::stop,
            m_rgGUIApp, &RosegardenMainWindow::slotStop );
    connect(this, &LircCommander::record,
            m_rgGUIApp, &RosegardenMainWindow::slotRecord );
    connect(this, &LircCommander::rewind,
            m_rgGUIApp, &RosegardenMainWindow::slotRewind );
    connect(this, &LircCommander::rewindToBeginning,
            m_rgGUIApp, &RosegardenMainWindow::slotRewindToBeginning );
    connect(this, &LircCommander::fastForward,
            m_rgGUIApp, &RosegardenMainWindow::slotFastforward );
    connect(this, &LircCommander::fastForwardToEnd,
            m_rgGUIApp, &RosegardenMainWindow::slotFastForwardToEnd );
    connect(this, &LircCommander::toggleRecord,
            m_rgGUIApp, &RosegardenMainWindow::slotToggleRecord );
    connect(this, &LircCommander::trackDown,
            m_rgGUIApp, &RosegardenMainWindow::slotTrackDown );
    connect(this, &LircCommander::trackUp,
            m_rgGUIApp, &RosegardenMainWindow::slotTrackUp );
    connect(this, SIGNAL(trackMute()),
            m_rgGUIApp, SLOT(slotToggleMute()) );
    connect(this, &LircCommander::trackRecord,
            m_rgGUIApp, &RosegardenMainWindow::slotToggleRecordCurrentTrack );
    connect(this, &LircCommander::undo,
            CommandHistory::getInstance(), &CommandHistory::undo );
    connect(this, &LircCommander::redo,
            CommandHistory::getInstance(), &CommandHistory::redo );
    connect(this, &LircCommander::aboutrg,
            m_rgGUIApp, &RosegardenMainWindow::slotHelpAbout );
    connect(this, &LircCommander::editInMatrix,
            m_rgGUIApp, &RosegardenMainWindow::slotEditInMatrix );
    connect(this, &LircCommander::editInPercussionMatrix,
            m_rgGUIApp, &RosegardenMainWindow::slotEditInPercussionMatrix );
    connect(this, &LircCommander::editInEventList,
            m_rgGUIApp, &RosegardenMainWindow::slotEditInEventList );
    connect(this, &LircCommander::editAsNotation,
            m_rgGUIApp, &RosegardenMainWindow::slotEditAsNotation );
    connect(this, &LircCommander::quit,
            m_rgGUIApp, &RosegardenMainWindow::slotQuit );
    connect(this, &LircCommander::closeTransport,
            m_rgGUIApp, &RosegardenMainWindow::slotCloseTransport );
    connect(this, &LircCommander::toggleTransportVisibility,
            m_rgGUIApp, &RosegardenMainWindow::slotToggleTransportVisibility );
}

LircCommander::command LircCommander::commands[] =
    {
        { "ABOUTRG",            cmd_aboutrg },
        { "CLOSETRANSPORT",     cmd_closeTransport },
        { "EDITEVLIST",         cmd_editInEventList },
        { "EDITMATRIX",         cmd_editInMatrix },
        { "EDITNOTATION",       cmd_editAsNotation },
        { "EDITPMATRIX",        cmd_editInPercussionMatrix },
        { "FORWARD",            cmd_fastForward },
        { "FORWARDTOEND",       cmd_fastForwardToEnd },
        { "PLAY",               cmd_play },
        { "PUNCHINRECORD",      cmd_toggleRecord },
        { "QUIT",               cmd_quit },
        { "RECORD",             cmd_record },
        { "REDO",               cmd_redo },
        { "REWIND",             cmd_rewind },
        { "REWINDTOBEGINNING",  cmd_rewindToBeginning },
        { "STOP",               cmd_stop },
        { "TOGGLETRANSPORT",    cmd_toggleTransportVisibility },
        { "TRACK+",             cmd_trackUp },
        { "TRACK-",             cmd_trackDown },
        { "TRACK-MUTE",         cmd_trackMute },
        { "TRACK-RECORD",       cmd_trackRecord },
        { "UNDO",               cmd_undo },
    };


int LircCommander::compareCommandName(const void *c1, const void *c2)
{
    return (strcmp(((struct command *)c1)->name, ((struct command *)c2)->name));
}

void LircCommander::slotExecute(const char *command)
{
    struct command tmp, *res;

    RG_DEBUG << "LircCommander::slotExecute: invoking command: " << command;

    // find the function for the name
    tmp.name = command;
    res = (struct command *)bsearch(&tmp, commands,
                                    sizeof(commands) / sizeof(struct command),
                                    sizeof(struct command),
                                    compareCommandName);
    if (res != nullptr)
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
        case cmd_undo:
            emit undo(); 
            break;
        case cmd_redo:
            emit redo(); 
            break;
        case cmd_aboutrg:
            emit aboutrg(); 
            break;
        case cmd_editInEventList:
            emit editInEventList(); 
            break;
        case cmd_editInMatrix:
            emit editInMatrix(); 
            break;
        case cmd_editInPercussionMatrix:
            emit editInPercussionMatrix(); 
            break;
        case cmd_editAsNotation:
            emit editAsNotation(); 
            break;
        case cmd_quit:
            emit quit(); 
            break;
        case cmd_closeTransport:
            emit closeTransport(); 
            break;
        case cmd_toggleTransportVisibility:
            emit toggleTransportVisibility(); 
            break;
        default:
            RG_DEBUG <<  "LircCommander::slotExecute: unhandled command " << command;
            return;
        }
        RG_DEBUG <<  "LircCommander::slotExecute: handled command: " << command;
    }
    else
    {
        RG_DEBUG <<  "LircCommander::slotExecute: invoking command: " << command
                 <<  " failed (command not defined in LircCommander::commands[])" << endl;
    };
}

}


#endif
