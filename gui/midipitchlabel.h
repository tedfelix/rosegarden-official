// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
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

#ifndef _MIDI_PITCH_LABEL_H_
#define _MIDI_PITCH_LABEL_H_

#include <qstring.h>
#include <string>


// Just a little helper class to turn MIDI note values into text.

namespace Rosegarden
{

class MidiPitchLabel
{
public:
    MidiPitchLabel(int pitch);

    std::string getString() const;
    QString getQString() const;

private:
    QString m_midiNote;

};

}

#endif // _MIDI_PITCH_LABEL_H_
