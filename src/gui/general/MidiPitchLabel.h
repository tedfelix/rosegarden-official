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

#ifndef RG_MIDIPITCHLABEL_H
#define RG_MIDIPITCHLABEL_H

#include <QString>

#include <QCoreApplication>


namespace Rosegarden
{


/// Convert a pitch number to a string.
/**
 */
class MidiPitchLabel
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::MidiPitchLabel)

public:

    // ??? This belongs in base, not gui/general.  Maybe in base/MidiTypes.h.
    //     Or maybe NotationTypes.h which has the Pitch class which has a
    //     getAsString().  Maybe move this to a static Pitch::toString()
    //     and a Pitch::toStringNoOctave().
    // ??? There are some other places that seem to do the same thing
    //     and would probably benefit from using this routine which is a
    //     very fast table lookup.
    //       - Pitch::getAsString()
    //       - NotationStaff::getNoteNameAtSceneCoords()
    //       - TrackParameterBox::updateWidgets2() (uses Pitch::getAsString())
    static QString pitchToString(int pitch);

};


}

#endif
