/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "AddFingeringMarkCommand.h"

#include "base/NotationTypes.h"
#include "base/NotationQuantizer.h"
#include "base/Segment.h"
#include "base/Selection.h"
#include "base/Sets.h"
#include "misc/Strings.h"
#include "document/BasicSelectionCommand.h"
#include "document/CommandRegistry.h"
#include <QString>


namespace Rosegarden
{

void
AddFingeringMarkCommand::registerCommand(CommandRegistry *r)
{
    std::vector<std::string> fingerings(getStandardFingerings());
    for (size_t i = 0; i < fingerings.size(); ++i) {
        std::string fingering = fingerings[i];
        r->registerCommand
            (getActionName(fingering),
             new ArgumentAndSelectionCommandBuilder<AddFingeringMarkCommand>());
    }
    r->registerCommand
        (getActionName(),
         new ArgumentAndSelectionCommandBuilder<AddFingeringMarkCommand>());
}

QString
AddFingeringMarkCommand::getGlobalName(std::string fingering)
{
    if (fingering == "") {
        return tr("Add Other &Fingering...");
    } else if (fingering == "0") {
        return tr("Add Fingering &0 (Thumb)");
    } else {
        return tr("Add Fingering &%1").arg(strtoqstr(fingering));
    }
}

QString
AddFingeringMarkCommand::getActionName(std::string fingering)
{
    if (fingering == "") {
        return "add_fingering_mark";
    }
    QString base = "add_fingering_%1";
    if (fingering == "+") {
        return base.arg("plus");
    } else {
        return base.arg(strtoqstr(fingering));
    }
}    

std::string
AddFingeringMarkCommand::getArgument(QString actionName,
                                     CommandArgumentQuerier &querier)
{
    QString pfx = "add_fingering_";
    if (actionName.startsWith(pfx)) {
        QString remainder = actionName.right(actionName.length() - pfx.length());
        if (remainder == "mark") {
            bool ok = false;
            QString txt = querier.getText(tr("Fingering: "), &ok);
            if (!ok) throw CommandCancelled();
            else return qstrtostr(txt);
        } else if (remainder == "plus") {
            return "+";
        } else {
            return qstrtostr(remainder);
        }
    }
    return "";
}

std::vector<std::string>
AddFingeringMarkCommand::getStandardFingerings()
{
    std::vector<std::string> fingerings;
    fingerings.push_back("0");
    fingerings.push_back("1");
    fingerings.push_back("2");
    fingerings.push_back("3");
    fingerings.push_back("4");
    fingerings.push_back("5");
    fingerings.push_back("+");
    return fingerings;
}

void
AddFingeringMarkCommand::modifySegment()
{
    if (m_fingering == "") return;

    EventSelection::eventcontainer::iterator i;
    Segment &segment(m_selection->getSegment());

    std::set<Event *> done;

    for (i = m_selection->getSegmentEvents().begin();
         i != m_selection->getSegmentEvents().end(); ++i) {

        if (done.find(*i) != done.end()) continue;
        if (!(*i)->isa(Note::EventType)) continue;

        // If we find a note in the selection with TIED_BACKWARD set, it should
        // be part of a chain of tied notes.  Putting the same mark on an entire
        // chain of notes is definitely wrong, so we ignore these events.
        if ((*i)->has(BaseProperties::TIED_BACKWARD)) continue;

        // We should do this on a chord-by-chord basis, considering
        // only those notes in a chord that are also in the selection.
        // Apply this fingering to the first note in the chord that
        // does not already have a fingering.  If they all already do,
        // then clear them all and start again.

        Chord chord(segment, segment.findSingle(*i),
                    segment.getComposition()->getNotationQuantizer());

        int attempt = 0;

        while (attempt < 2) {

            int count = 0;

            for (Chord::iterator ci = chord.begin();
                    ci != chord.end(); ++ci) {

                if (!m_selection->contains(**ci))
                    continue;

                if (attempt < 2 &&
                        Marks::getFingeringMark(***ci) ==
                        Marks::NoMark) {
                    Marks::addMark
                    (***ci, Marks::getFingeringMark(m_fingering), true);
                    attempt = 2;
                }

                done.insert(**ci);
                ++count;
            }

            if (attempt < 2) {
                if (count == 0)
                    break;
                for (Chord::iterator ci = chord.begin();
                        ci != chord.end(); ++ci) {
                    if (m_selection->contains(**ci)) {
                        Marks::removeMark
                        (***ci,
                         Marks::getFingeringMark(***ci));
                    }
                }
                ++attempt;
            }
        }
    }
}

}
