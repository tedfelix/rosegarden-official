/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.

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

#ifndef RG_LIRCCOMMANDER_H
#define RG_LIRCCOMMANDER_H

#ifdef HAVE_LIRC

#include <QObject>
#include "base/Track.h"


namespace Rosegarden
{

class RosegardenMainWindow;
class RosegardenDocument;
class TrackButtons;
class LircClient;


class LircCommander : public QObject
{
    Q_OBJECT
public:
    LircCommander(LircClient *lirc, RosegardenMainWindow *rgGUIApp);

signals:
    //for RosegardenMainWindow
    void play();
    void stop();
    void record();
    void rewind();
    void rewindToBeginning();
    void fastForward();
    void fastForwardToEnd();
    void toggleRecord();
    void trackDown();
    void trackUp();
    void trackMute();
    void trackRecord();
    void undo();
    void redo();
    void aboutrg();
    void editInEventList();
    void editInMatrix();
    void editInPercussionMatrix();
    void editAsNotation();
    void quit();
    void closeTransport();
    void toggleTransportVisibility();
    
private slots:
    void slotExecute(const char *);
    //void slotDocumentChanged(RosegardenDocument *);
        
private:
    LircClient          *m_lirc;
    RosegardenMainWindow    *m_rgGUIApp;
    //TrackButtons        *m_trackButtons;
     
    // commands invoked by lirc
    enum commandCode {
    	cmd_play,
    	cmd_stop,
    	cmd_record,
    	cmd_rewind,
    	cmd_rewindToBeginning,
    	cmd_fastForward,
    	cmd_fastForwardToEnd,
    	cmd_toggleRecord,
    	cmd_trackDown,
    	cmd_trackUp,
    	cmd_trackMute,
    	cmd_trackRecord,
	cmd_undo,
    	cmd_redo,
	cmd_aboutrg,
	cmd_editInEventList,
	cmd_editInMatrix,
	cmd_editInPercussionMatrix,
	cmd_editAsNotation,
	cmd_quit,
	cmd_closeTransport,
	cmd_toggleTransportVisibility
    };
    
    
    // the command -> method mapping table
    static struct command
    {
        const char *name;  /* command name */
        commandCode code;  /* function to process it */
    }
    commands[];

    // utilities
    static int compareCommandName(const void *c1, const void *c2);

};



}

#endif

#endif
