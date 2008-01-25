/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2008
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>

    This file is Copyright 2005
        Toni Arnold         <toni__arnold@bluewin.ch>

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

#ifdef HAVE_LIRC

#include <qobject.h>
#include "base/Track.h"


namespace Rosegarden
{

class RosegardenGUIApp;
class RosegardenGUIDoc;
class TrackButtons;
class LircClient;


class LircCommander : public QObject
{
    Q_OBJECT
public:
    LircCommander(LircClient *lirc, RosegardenGUIApp *rgGUIApp);

signals:
    //for RosegardenGUIApp
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
    
private slots:
    void slotExecute(char *);
    //void slotDocumentChanged(RosegardenGUIDoc *);
        
private:
    LircClient          *m_lirc;
    RosegardenGUIApp    *m_rgGUIApp;
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
    	cmd_trackRecord
    };
    
    
    // the command -> method mapping table
    static struct command
    {
        char *name;        /* command name */
        commandCode code;  /* function to process it */
    }
    commands[];

    // utilities
    static int compareCommandName(const void *c1, const void *c2);

};



}

#endif

#endif
