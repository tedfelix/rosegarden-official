/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2009 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "MidiPitchLabel.h"
#include "document/ConfigGroups.h"

#include <QApplication>
#include <QSettings>
#include <QString>


namespace Rosegarden
{

MidiPitchLabel::MidiPitchLabel(int pitch)
{
    static QString notes[] = {
        tr("C%1"),  tr("C#%1"), tr("D%1"),  tr("D#%1"),
        tr("E%1"),  tr("F%1"),  tr("F#%1"), tr("G%1"),
        tr("G#%1"), tr("A%1"),  tr("A#%1"), tr("B%1")
    };

    if (pitch < 0 || pitch > 127) {

        m_midiNote = "";

    } else {

        QSettings settings;
        settings.beginGroup( GeneralOptionsConfigGroup );

        int baseOctave = settings.value("midipitchoctave", -2).toInt() ;

        int octave = (int)(((float)pitch) / 12.0) + baseOctave;
        m_midiNote = notes[pitch % 12].arg(octave);

        settings.endGroup();
    }
}

std::string
MidiPitchLabel::getString() const
{
    return std::string(m_midiNote.toUtf8().data());
}

QString
MidiPitchLabel::getQString() const
{
    return m_midiNote;
}

}
