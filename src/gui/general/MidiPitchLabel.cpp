/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[MidiPitchLabel]"
#define RG_NO_DEBUG_PRINT

#include "MidiPitchLabel.h"

#include "misc/ConfigGroups.h"
#include "misc/Debug.h"

#include <QApplication>
#include <QSettings>
#include <QString>


namespace Rosegarden
{


namespace
{

    // this was refactored to take advantage of these translations being
    // available in other contexts, and to avoid extra work for translators
    QString a_notes[] = {
        QObject::tr("C",  "note name"), QObject::tr("C#", "note name"),
        QObject::tr("D",  "note name"), QObject::tr("D#", "note name"),
        QObject::tr("E",  "note name"), QObject::tr("F",  "note name"),
        QObject::tr("F#", "note name"), QObject::tr("G",  "note name"),
        QObject::tr("G#", "note name"), QObject::tr("A",  "note name"),
        QObject::tr("A#", "note name"), QObject::tr("B",  "note name")
    };

    std::vector<QString> initPitchTable()
    {
        std::vector<QString> pitchTable{128};

        QSettings settings;
        settings.beginGroup(GeneralOptionsConfigGroup);
        int baseOctave = settings.value("midipitchoctave", -2).toInt() ;

        for (int pitch = 0; pitch < 128; ++pitch) {
            const int octave = int(pitch / 12.0) + baseOctave;
            pitchTable[pitch] =
                    QString("%1 %2").arg(a_notes[pitch % 12]).arg(octave);
        }

        return pitchTable;
    }

}

QString MidiPitchLabel::pitchToString(int pitch)
{
    static std::vector<QString> pitchTable = initPitchTable();

    if (pitch < 0  ||  pitch > 127)
        return QString("*%1*").arg(pitch);

    return pitchTable[pitch];
}


}
