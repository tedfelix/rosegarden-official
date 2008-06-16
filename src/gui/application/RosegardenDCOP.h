// -*- c-basic-offset: 4 -*-
/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.
    See the AUTHORS file for more details.

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

#define ROSEGARDEN_GUI_APP_NAME         "rosegarden"
#define ROSEGARDEN_GUI_IFACE_NAME       "RosegardenIface"


// Sequencer communicates its state through this enum
//
typedef enum
{
     STOPPED,
     PLAYING,
     RECORDING,
     STOPPING,
     STARTING_TO_PLAY,
     STARTING_TO_RECORD,
     RECORDING_ARMED,                   // gui only state
     QUIT
} TransportStatus;


#endif // _ROSEGARDEN_DCOP_H_
