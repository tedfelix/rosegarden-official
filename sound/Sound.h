// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-
/*
  Rosegarden-4 v0.1
  A sequencer and musical notation editor.

  This program is Copyright 2000-2002
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

// Common Sound subsystem stuff 
//

#ifndef _ROSEGARDEN_SOUND_H_
#define _ROSEGARDEN_SOUND_H_

namespace Rosegarden
{


typedef enum
{
    MIDI_AND_AUDIO_SUBSYS_OK,  // Everything's OK
    MIDI_SUBSYS_OK,            // MIDI's OK
    AUDIO_SUBSYS_OK,           // AUDIO's OK
    NO_SEQUENCE_SUBSYS         // Nothing's OK
} SoundSystemStatus;

const float SAMPLE_MAX_8BIT  = (float)(0xff);
const float SAMPLE_MAX_16BIT = (float)(0xffff/2);
const float SAMPLE_MAX_24BIT = (float)(0xffffff/2);



};


#endif // _ROSEGARDEN_SOUND_H_
