/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */
/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#ifndef RG_TRANSPORT_STATUS_H
#define RG_TRANSPORT_STATUS_H

typedef enum
{
     STOPPED,
     PLAYING,
     RECORDING,
     STOPPING,
     STARTING_TO_PLAY,
     STARTING_TO_RECORD,
     // SequenceManager uses this state when we go to record.  We stay
     // in this state until recording begins.  Usually recording begins
     // immediately, but in the past, the CountdownDialog would delay
     // the actual start of recording.
     RECORDING_ARMED,
     QUIT
} TransportStatus;

#endif // RG_TRANSPORT_STATUS_H

