// -*- c-basic-offset: 4 -*-
/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2001
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#ifndef _ROSEGARDEN_DCOP_H_
#define _ROSEGARDEN_DCOP_H_


// The names of our applications and interfaces that we share
//
#define ROSEGARDEN_SEQUENCER_APP_NAME   "rosegardensequencer"
#define ROSEGARDEN_SEQUENCER_IFACE_NAME "RosegardenSequencerIface"

#define ROSEGARDEN_GUI_APP_NAME         "rosegardengui"
#define ROSEGARDEN_GUI_IFACE_NAME       "RosegardenGUIIface"


// Sequencer communicates its state through this enum - 
// it's sent over the wire as an "int" for the moment
//
enum TransportStatus
{
     STOPPED,
     PLAYING,
     RECORDING_MIDI,
     RECORDING_AUDIO,
     STOPPING,
     STARTING_TO_PLAY,
     QUIT
};


#endif // _ROSEGARDEN_DCOP_H_
