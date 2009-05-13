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
#include "misc/ConfigGroups.h"

#include <QApplication>
#include <QSettings>
#include <QString>


namespace Rosegarden
{

MidiPitchLabel::MidiPitchLabel(int pitch)
{
    // this was refactored to take advantage of these translations being
    // available in other contexts, and to avoid extra work for translators
    static QString notes[] = {
        QObject::tr("C"),  QObject::tr("C#"), QObject::tr("D"),  QObject::tr("D#"),
        QObject::tr("E"),  QObject::tr("F"),  QObject::tr("F#"), QObject::tr("G"),
        QObject::tr("G#"), QObject::tr("A"),  QObject::tr("A#"), QObject::tr("B")
    };

    if (pitch < 0 || pitch > 127) {

        m_midiNote = "";

    } else {

        QSettings settings;
        settings.beginGroup( GeneralOptionsConfigGroup );

        int baseOctave = settings.value("midipitchoctave", -2).toInt() ;

        int octave = (int)(((float)pitch) / 12.0) + baseOctave;
        m_midiNote = QString("%1 %2").arg(notes[pitch % 12]).arg(octave);

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
