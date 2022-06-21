/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2022 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "RespellCommand.h"

#include "base/NotationTypes.h"
#include "base/Selection.h"
#include "document/CommandRegistry.h"
#include "base/BaseProperties.h"

#include <QString>


namespace Rosegarden
{


QString
RespellCommand::getGlobalName(RespellType type)
{
    switch (type.type) {

    case RespellType::Set: {
            QString s(tr("Respell with %1"));
            //!!! should be in notationstrings:
            if (type.accidental == Accidentals::DoubleSharp) {
                s = s.arg(tr("Do&uble Sharp"));
            } else if (type.accidental == Accidentals::Sharp) {
                s = s.arg(tr("&Sharp"));
            } else if (type.accidental == Accidentals::Flat) {
                s = s.arg(tr("&Flat"));
            } else if (type.accidental == Accidentals::DoubleFlat) {
                s = s.arg(tr("Dou&ble Flat"));
            } else if (type.accidental == Accidentals::Natural) {
                s = s.arg(tr("&Natural"));
            } else {
                s = s.arg(tr("N&one"));
            }
            return s;
        }

    case RespellType::Up:
        return tr("Respell Accidentals &Upward");

    case RespellType::Down:
        return tr("Respell Accidentals &Downward");

    case RespellType::Restore:
        return tr("&Restore Accidentals");
    }

    return tr("Respell Accidentals");
}

RespellCommand::RespellType
RespellCommand::getArgument(QString actionName, CommandArgumentQuerier &)
{
    RespellType type;
    type.type = RespellType::Set;
    type.accidental = Accidentals::Natural;

    if (actionName == "respell_doubleflat") {
        type.accidental = Accidentals::DoubleFlat;
    } else if (actionName == "respell_flat") {
        type.accidental = Accidentals::Flat;
    } else if (actionName == "respell_natural") {
        type.accidental = Accidentals::Natural;
    } else if (actionName == "respell_sharp") {
        type.accidental = Accidentals::Sharp;
    } else if (actionName == "respell_doublesharp") {
        type.accidental = Accidentals::DoubleSharp;
    } else if (actionName == "respell_restore") {
        type.type = RespellType::Restore;
    } else if (actionName == "respell_up") {
        type.type = RespellType::Up;
    } else if (actionName == "respell_down") {
        type.type = RespellType::Down;
    }

    return type;
}

void
RespellCommand::registerCommand(CommandRegistry *r)
{
    r->registerCommand
        ("respell_doubleflat",
         new ArgumentAndSelectionCommandBuilder<RespellCommand>());

    r->registerCommand
        ("respell_flat",
         new ArgumentAndSelectionCommandBuilder<RespellCommand>());

    r->registerCommand
        ("respell_natural",
         new ArgumentAndSelectionCommandBuilder<RespellCommand>());

    r->registerCommand
        ("respell_sharp",
         new ArgumentAndSelectionCommandBuilder<RespellCommand>());

    r->registerCommand
        ("respell_doublesharp",
         new ArgumentAndSelectionCommandBuilder<RespellCommand>());

    r->registerCommand
        ("respell_up",
         new ArgumentAndSelectionCommandBuilder<RespellCommand>());

    r->registerCommand
        ("respell_down",
         new ArgumentAndSelectionCommandBuilder<RespellCommand>());

    r->registerCommand
        ("respell_restore",
         new ArgumentAndSelectionCommandBuilder<RespellCommand>());
}

void
RespellCommand::modifySegment()
{
    EventContainer::iterator i;

    for (i = m_selection->getSegmentEvents().begin();
         i != m_selection->getSegmentEvents().end(); ++i) {

        if ((*i)->isa(Note::EventType)) {

            if (m_type.type == RespellType::Up ||
                m_type.type == RespellType::Down) {

                Accidental acc = Accidentals::NoAccidental;
                (*i)->get<String>(BaseProperties::ACCIDENTAL, acc);

                if (m_type.type == RespellType::Down) {
                    if (acc == Accidentals::DoubleFlat) {
                        acc = Accidentals::Flat;
                    } else if (acc == Accidentals::Flat || acc == Accidentals::NoAccidental) {
                        acc = Accidentals::Sharp;
                    } else if (acc == Accidentals::Sharp) {
                        acc = Accidentals::DoubleSharp;
                    }
                } else {
                    if (acc == Accidentals::Flat) {
                        acc = Accidentals::DoubleFlat;
                    } else if (acc == Accidentals::Sharp || acc == Accidentals::NoAccidental) {
                        acc = Accidentals::Flat;
                    } else if (acc == Accidentals::DoubleSharp) {
                        acc = Accidentals::Sharp;
                    }
                }

                (*i)->set<String>(BaseProperties::ACCIDENTAL, acc);

            } else if (m_type.type == RespellType::Set) {

                // trap respelling black key notes as natural; which is
                // impossible, and makes rawPitchToDisplayPitch() do crazy
                // things as a consequence (fixes #1349782)
                // 1 = C#, 3 = D#, 6 = F#, 8 = G#, 10 = A#
                long pitch;
                pitch = 0;  // Avoid a "may be used uninitialized" compilation warning
                (*i)->get<Int>(BaseProperties::PITCH, pitch);
                pitch %= 12;
                if ((pitch == 1 || pitch == 3 || pitch == 6 || pitch == 8 || pitch == 10 )
                    && m_type.accidental == Accidentals::Natural) {
                    // fail silently; is there anything to do here?
                } else {
                    (*i)->set<String>(BaseProperties::ACCIDENTAL, m_type.accidental);
                }

            } else {

                (*i)->unset(BaseProperties::ACCIDENTAL);
            }
        }
    }
}


}
