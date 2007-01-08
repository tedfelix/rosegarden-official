/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2007
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>
 
    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "RespellCommand.h"

#include <klocale.h>
#include "base/NotationTypes.h"
#include "base/Selection.h"
#include "document/BasicSelectionCommand.h"
#include "base/BaseProperties.h"
#include <qstring.h>


namespace Rosegarden
{
using namespace BaseProperties;
using namespace Accidentals;

QString
RespellCommand::getGlobalName(Type type, Accidental accidental)
{
    switch (type) {

    case Set: {
            QString s(i18n("Respell with %1"));
            //!!! should be in notationstrings:
            if (accidental == DoubleSharp) {
                s = s.arg(i18n("Do&uble Sharp"));
            } else if (accidental == Sharp) {
                s = s.arg(i18n("&Sharp"));
            } else if (accidental == Flat) {
                s = s.arg(i18n("&Flat"));
            } else if (accidental == DoubleFlat) {
                s = s.arg(i18n("Dou&ble Flat"));
            } else if (accidental == Natural) {
                s = s.arg(i18n("&Natural"));
            } else {
                s = s.arg(i18n("N&one"));
            }
            return s;
        }

    case Up:
        return i18n("Respell Accidentals &Upward");

    case Down:
        return i18n("Respell Accidentals &Downward");

    case Restore:
        return i18n("&Restore Computed Accidentals");
    }

    return i18n("Respell Accidentals");
}

void
RespellCommand::modifySegment()
{
    EventSelection::eventcontainer::iterator i;

    for (i = m_selection->getSegmentEvents().begin();
            i != m_selection->getSegmentEvents().end(); ++i) {

        if ((*i)->isa(Note::EventType)) {

            if (m_type == Up || m_type == Down) {

                Accidental acc = NoAccidental;
                (*i)->get
                <String>(ACCIDENTAL, acc);

                if (m_type == Down) {
                    if (acc == DoubleFlat) {
                        acc = Flat;
                    } else if (acc == Flat || acc == NoAccidental) {
                        acc = Sharp;
                    } else if (acc == Sharp) {
                        acc = DoubleSharp;
                    }
                } else {
                    if (acc == Flat) {
                        acc = DoubleFlat;
                    } else if (acc == Sharp || acc == NoAccidental) {
                        acc = Flat;
                    } else if (acc == DoubleSharp) {
                        acc = Sharp;
                    }
                }

                (*i)->set
                <String>(ACCIDENTAL, acc);

            } else if (m_type == Set) {

                // trap respelling black key notes as natural; which is
                // impossible, and makes rawPitchToDisplayPitch() do crazy
                // things as a consequence (fixes #1349782)
                // 1 = C#, 3 = D#, 6 = F#, 8 = G#, 10 = A#
                long pitch;
                (*i)->get
                <Int>(PITCH, pitch);
                pitch %= 12;
                if ((pitch == 1 || pitch == 3 || pitch == 6 || pitch == 8 || pitch == 10 )
                        && m_accidental == Natural) {
                    // fail silently; is there anything to do here?
                } else {
                    (*i)->set
                    <String>(ACCIDENTAL, m_accidental);
                }

            } else {

                (*i)->unset(ACCIDENTAL);
            }
        }
    }
}

}
