// -*- c-basic-offset: 4 -*-

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

#include <string>

#ifndef _AUDIO_H_
#define _AUDIO_H_

namespace Rosegarden
{

// Constants related to RIFF/WAV files
//
const std::string AUDIO_RIFF_ID = "RIFF";
const std::string AUDIO_WAVE_ID = "WAVE";
const std::string AUDIO_FORMAT_ID = "fmt_";



};

#endif // _AUDIO_H_
