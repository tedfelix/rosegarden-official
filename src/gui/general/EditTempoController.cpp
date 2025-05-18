/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[EditTempoController]"

#include "EditTempoController.h"

#include "commands/segment/AddTempoChangeCommand.h"
#include "commands/segment/AddTimeSignatureCommand.h"
#include "commands/segment/AddTimeSignatureAndNormalizeCommand.h"
#include "commands/segment/ModifyDefaultTempoCommand.h"
#include "commands/segment/RemoveTempoChangeCommand.h"
#include "document/CommandHistory.h"
#include "document/RosegardenDocument.h"
#include "gui/dialogs/TempoDialog.h"
#include "gui/dialogs/TimeSignatureDialog.h"


namespace Rosegarden
{


void EditTempoController::setDocument(RosegardenDocument *doc)
{
    m_doc = doc;
    m_composition = &m_doc->getComposition();
}

EditTempoController *EditTempoController::self()
{
    // Guaranteed in C++11 to be lazy initialized and thread-safe.
    // See ISO/IEC 14882:2011 6.7(4).
    static EditTempoController instance;

    return &instance;
}

void EditTempoController::editTempo(
        QWidget *parent, timeT atTime, bool timeEditable)
{
    TempoDialog tempoDialog(parent, m_doc, timeEditable);
    connect(&tempoDialog, &TempoDialog::changeTempo,
            this, &EditTempoController::changeTempo);
    tempoDialog.setTempoPosition(atTime);

    tempoDialog.exec();
}

void EditTempoController::editTimeSignature(QWidget *parent, timeT time)
{
    TimeSignature sig = m_composition->getTimeSignatureAt(time);

    TimeSignatureDialog dialog(parent, m_composition, time, sig);

    if (dialog.exec() == QDialog::Accepted) {

        time = dialog.getTime();

        if (dialog.shouldNormalizeRests()) {
            CommandHistory::getInstance()->addCommand(
                    new AddTimeSignatureAndNormalizeCommand(
                            m_composition, time, dialog.getTimeSignature()));
        } else {
            CommandHistory::getInstance()->addCommand(
                    new AddTimeSignatureCommand(
                            m_composition, time, dialog.getTimeSignature()));
        }
    }
}

void EditTempoController::moveTempo(timeT oldTime, timeT newTime)
{
    const int index = m_composition->getTempoChangeNumberAt(oldTime);

    if (index < 0)
        return;

    MacroCommand *macro = new MacroCommand(tr("Move Tempo Change"));

    const std::pair<timeT, tempoT> tc = m_composition->getTempoChange(index);
    const std::pair<bool, tempoT> tr =
            m_composition->getTempoRamping(index, false);

    macro->addCommand(new RemoveTempoChangeCommand(m_composition, index));
    macro->addCommand(new AddTempoChangeCommand(m_composition,
                                                newTime,
                                                tc.second,
                                                tr.first ? tr.second : -1));

    CommandHistory::getInstance()->addCommand(macro);
}

void EditTempoController::deleteTempoChange(timeT time)
{
    const int index = m_composition->getTempoChangeNumberAt(time);
    if (index >= 0) {
        CommandHistory::getInstance()->addCommand(
                new RemoveTempoChangeCommand(m_composition, index));
    }
}

void EditTempoController::changeTempo(timeT time,
                                      tempoT value,
                                      tempoT target,
                                      TempoDialog::TempoDialogAction action)
{
    if (action == TempoDialog::AddTempo) {
        CommandHistory::getInstance()->addCommand(new AddTempoChangeCommand(
                m_composition, time, value, target));

    } else if (action == TempoDialog::ReplaceTempo) {
        const int index = m_composition->getTempoChangeNumberAt(time);

        // if there's no previous tempo change then just set globally
        //
        if (index == -1) {
            CommandHistory::getInstance()->addCommand(
                    new AddTempoChangeCommand(m_composition, 0, value, target));
            return;
        }

        // get time of previous tempo change
        const timeT prevTime = m_composition->getTempoChange(index).first;

        MacroCommand *macro =
                new MacroCommand(tr("Replace Tempo Change at %1").arg(time));

        macro->addCommand(new RemoveTempoChangeCommand(m_composition, index));
        macro->addCommand(new AddTempoChangeCommand(
                m_composition, prevTime, value, target));

        CommandHistory::getInstance()->addCommand(macro);

    } else if (action == TempoDialog::AddTempoAtBarStart) {
        CommandHistory::getInstance()->addCommand(new AddTempoChangeCommand(
                m_composition,
                m_composition->getBarStartForTime(time),
                value,
                target));

    } else if (action == TempoDialog::GlobalTempo  ||
               action == TempoDialog::GlobalTempoWithDefault) {
        MacroCommand *macro = new MacroCommand(tr("Set Global Tempo"));

        // Remove all tempo changes in reverse order so as the index numbers
        // don't becoming meaningless as the command gets unwound.
        //
        for (int i = 0; i < m_composition->getTempoChangeCount(); ++i)
            macro->addCommand(new RemoveTempoChangeCommand(
                    m_composition,
                    m_composition->getTempoChangeCount() - 1 - i));

        // add tempo change at time zero
        //
        macro->addCommand(new AddTempoChangeCommand(m_composition, 0, value, target));

        // are we setting default too?
        //
        if (action == TempoDialog::GlobalTempoWithDefault) {
            macro->setName(tr("Set Global and Default Tempo"));
            macro->addCommand(new ModifyDefaultTempoCommand(
                    m_composition, value));
        }

        CommandHistory::getInstance()->addCommand(macro);

    } else {
        RG_DEBUG << "changeTempo() unrecognised tempo command";
    }
}


}
