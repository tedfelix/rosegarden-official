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

#ifndef _MIDI_PITCH_LABEL_H_
#define _MIDI_PITCH_LABEL_H_

#include <map>
#include <qstring.h>
#include <string>

namespace Rosegarden
{

static map<int, char> midiTextMap;

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
            int octave = (int)(((float)pitch)/12.0) - 2;
            string note;

            switch(pitch%12)
            {
                case 0:
                    note = "C";
                    break;

                case 1:
                    note = "C#";
                    break;

                case 2:
                    note = "D";
                    break;

                case 3:
                    note = "D#";
                    break;

                case 4:
                    note = "E";
                    break;

                case 5:
                    note = "F";
                    break;

                case 6:
                    note = "F#";
                    break;

                case 7:
                    note = "G";
                    break;

                case 8:
                    note = "G#";
                    break;

                case 9:
                    note = "A";
                    break;

                case 10:
                    note = "A#";
                    break;

                case 11:
                    note = "B";
                    break;

                default:
                    note = "";
                    break;
            }

            m_midiNote.sprintf("  %s%d", note.c_str(), octave);
        }
    }
    ~MidiPitchLabel();

    string getString() const { return string(m_midiNote.data()); }
    QString getQString() const { return m_midiNote; }

private:
    QString m_midiNote;

};

}

#endif // _MIDI_PITCH_LABEL_H_
