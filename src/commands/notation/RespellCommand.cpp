/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.
 
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
#include "document/BasicSelectionCommand.h"
#include "document/CommandRegistry.h"
#include "base/BaseProperties.h"
#include <QString>


namespace Rosegarden
{
using namespace BaseProperties;
using namespace Accidentals;

QString
RespellCommand::getGlobalName(RespellType type)
{
    switch (type.type) {

    case RespellType::Set: {
            QString s(tr("Respell with %1"));
            //!!! should be in notationstrings:
            if (type.accidental == DoubleSharp) {
                s = s.arg(tr("Do&uble Sharp"));
            } else if (type.accidental == Sharp) {
                s = s.arg(tr("&Sharp"));
            } else if (type.accidental == Flat) {
                s = s.arg(tr("&Flat"));
            } else if (type.accidental == DoubleFlat) {
                s = s.arg(tr("Dou&ble Flat"));
            } else if (type.accidental == Natural) {
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
    type.accidental = Natural;

    if (actionName == "respell_doubleflat") {
        type.accidental = DoubleFlat;
    } else if (actionName == "respell_flat") {
        type.accidental = Flat;
    } else if (actionName == "respell_natural") {
        type.accidental = Natural;
    } else if (actionName == "respell_sharp") {
        type.accidental = Sharp;
    } else if (actionName == "respell_doublesharp") {
        type.accidental = DoubleSharp;
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
    RespellType type;
    type.type = RespellType::Set;

    type.accidental = DoubleFlat;
    r->registerCommand
        ("respell_doubleflat",
         new ArgumentAndSelectionCommandBuilder<RespellCommand>());

    type.accidental = Flat;
    r->registerCommand
        ("respell_flat",
         new ArgumentAndSelectionCommandBuilder<RespellCommand>());

    type.accidental = Natural;
    r->registerCommand
        ("respell_natural",
         new ArgumentAndSelectionCommandBuilder<RespellCommand>());

    type.accidental = Sharp;
    r->registerCommand
        ("respell_sharp",
         new ArgumentAndSelectionCommandBuilder<RespellCommand>());

    type.accidental = DoubleSharp;
    r->registerCommand
        ("respell_doublesharp",
         new ArgumentAndSelectionCommandBuilder<RespellCommand>());

    type.accidental = Natural;
    
    type.type = RespellType::Up;
    r->registerCommand
        ("respell_up",
         new ArgumentAndSelectionCommandBuilder<RespellCommand>());
    
    type.type = RespellType::Down;
    r->registerCommand
        ("respell_down",
         new ArgumentAndSelectionCommandBuilder<RespellCommand>());
    
    type.type = RespellType::Restore;
    r->registerCommand
        ("respell_restore",
         new ArgumentAndSelectionCommandBuilder<RespellCommand>());
}

void
RespellCommand::modifySegment()
{
    EventSelection::eventcontainer::iterator i;

    for (i = m_selection->getSegmentEvents().begin();
         i != m_selection->getSegmentEvents().end(); ++i) {

        if ((*i)->isa(Note::EventType)) {

            if (m_type.type == RespellType::Up ||
                m_type.type == RespellType::Down) {

                Accidental acc = NoAccidental;
                (*i)->get<String>(ACCIDENTAL, acc);

                if (m_type.type == RespellType::Down) {
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

                (*i)->set<String>(ACCIDENTAL, acc);

            } else if (m_type.type == RespellType::Set) {

                // trap respelling black key notes as natural; which is
                // impossible, and makes rawPitchToDisplayPitch() do crazy
                // things as a consequence (fixes #1349782)
                // 1 = C#, 3 = D#, 6 = F#, 8 = G#, 10 = A#
                long pitch;
                pitch = 0;  // Avoid a "may be used uninitialized" compilation warning
                (*i)->get<Int>(PITCH, pitch);
                pitch %= 12;
                if ((pitch == 1 || pitch == 3 || pitch == 6 || pitch == 8 || pitch == 10 )
                    && m_type.accidental == Natural) {
                    // fail silently; is there anything to do here?
                } else {
                    (*i)->set<String>(ACCIDENTAL, m_type.accidental);
                }

            } else {

                (*i)->unset(ACCIDENTAL);
            }
        }
    }
}

}
