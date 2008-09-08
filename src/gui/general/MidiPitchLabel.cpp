/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.
 
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

#include <klocale.h>

namespace Rosegarden
{

static QString notes[] = {
    i18n("C%1"),  i18n("C#%1"), i18n("D%1"),  i18n("D#%1"),
    i18n("E%1"),  i18n("F%1"),  i18n("F#%1"), i18n("G%1"),
    i18n("G#%1"), i18n("A%1"),  i18n("A#%1"), i18n("B%1")
};


MidiPitchLabel::MidiPitchLabel(int pitch)
{
    if (pitch < 0 || pitch > 127) {

        m_midiNote = "";

    } else {

        QSettings config;
        config.beginGroup( GeneralOptionsConfigGroup );
        // 
        // FIX-manually-(GW), add:
        // config.endGroup();		// corresponding to: config.beginGroup( GeneralOptionsConfigGroup );
        //  

        int baseOctave = config.value("midipitchoctave", -2).toInt() ;

        int octave = (int)(((float)pitch) / 12.0) + baseOctave;
        m_midiNote = notes[pitch % 12].arg(octave);
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
