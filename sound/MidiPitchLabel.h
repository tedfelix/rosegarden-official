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

#include <map>
#include <qstring.h>
#include <string>


// Just a little helper class to turn MIDI note values into text.
// Relying on QStrings I know but what the hey.
//


namespace Rosegarden
{

static std::string notes[] = { "C",  "C#", "D",  "D#", "E",  "F",
			       "F#", "G",  "G#", "A",  "A#", "B"  };

class MidiPitchLabel
{
public:
    MidiPitchLabel(const int &pitch)
    {
        if (pitch < 0 || pitch > 127)
        {
            m_midiNote = "";
        }
        else
        {
            // We convert the pitch to a string as follows
            //
            int octave = (int)(((float)pitch)/12.0) - 2;
            m_midiNote.sprintf("%s%d", notes[pitch%12].c_str(), octave);
        }
    }
    ~MidiPitchLabel();


    // Return the string as we like
    //
    std::string getString() const { return std::string(m_midiNote.utf8().data()); }
    QString getQString() const { return m_midiNote; }

private:
    QString m_midiNote;

};

}

#endif // _MIDI_PITCH_LABEL_H_
