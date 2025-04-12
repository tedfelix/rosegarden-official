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

#define RG_MODULE_STRING "[CopyCommand]"

#include "CopyCommand.h"

#include "misc/AppendLabel.h"
#include "misc/Debug.h"
#include "misc/Strings.h"
#include "base/Clipboard.h"
#include "base/Composition.h"
#include "base/Selection.h"

#include <QString>


namespace Rosegarden
{

CopyCommand::CopyCommand(EventSelection *selection,
                         Clipboard *clipboard) :
        NamedCommand(getGlobalName()),
        m_targetClipboard(clipboard)
{
    m_sourceClipboard = new Clipboard;
    m_savedClipboard = nullptr;
    std::string label = selection->getSegment().getLabel();
    m_sourceClipboard->newSegment(selection)->setLabel(
            appendLabel(label, qstrtostr(tr("(excerpt)"))));
}

CopyCommand::CopyCommand(
        EventSelection *selection1,
        EventSelection *selection2,
        Clipboard *clipboard) :
    NamedCommand(getGlobalName()),
    m_sourceClipboard(new Clipboard),
    m_targetClipboard(clipboard),
    m_savedClipboard(nullptr)
{
    if (!selection1  &&  !selection2)
        return;

    // Create a new Segment in the source Clipboard containing the
    // selected Events.
    Segment *newSegment = m_sourceClipboard->newSegment(selection1, selection2);

    if (!newSegment)
        return;

    // Add "(excerpt)" to the Segment's label.
    // ??? Why?  This is a temporary Segment in the clipboard.
    std::string label;
    if (selection1)
        label = selection1->getSegment().getLabel();
    else if (selection2)
        label = selection2->getSegment().getLabel();
    newSegment->setLabel(appendLabel(label, qstrtostr(tr("(excerpt)"))));
}

CopyCommand::CopyCommand(SegmentSelection &selection,
                         Clipboard *clipboard) :
        NamedCommand(getGlobalName()),
        m_targetClipboard(clipboard)
{
    m_sourceClipboard = new Clipboard;
    m_savedClipboard = nullptr;

    for (SegmentSelection::iterator i = selection.begin();
            i != selection.end(); ++i) {
        std::string label = (*i)->getLabel();
        // Fix #1446 : Make a deep copy only when segment is truly linked.
        m_sourceClipboard->newSegment(*i, (*i)->isTrulyLinked())->setLabel(
                appendLabel(label, qstrtostr(tr("(copied)"))));
    }
}

CopyCommand::CopyCommand(Composition *composition,
                         timeT beginTime,
                         timeT endTime,
                         Clipboard *clipboard) :
        NamedCommand(tr("Copy Range")),
        m_targetClipboard(clipboard)
{
    m_sourceClipboard = new Clipboard;
    m_savedClipboard = nullptr;

    // For each segment in the composition
    for (Composition::iterator i = composition->begin();
            i != composition->end(); ++i) {
        // If the segment overlaps the copy time range
        if ((*i)->getStartTime() < endTime &&
                (*i)->getRepeatEndTime() > beginTime) {
            // Copy the appropriate portion of the segment to the source
            // clipboard.
            m_sourceClipboard->newSegment(*i, beginTime, endTime, true);
        }
    }

    TimeSignatureSelection tsigsel(*composition, beginTime, endTime, true);
    m_sourceClipboard->setTimeSignatureSelection(tsigsel);

    TempoSelection temposel(*composition, beginTime, endTime, true);
    m_sourceClipboard->setTempoSelection(temposel);

    m_sourceClipboard->setNominalRange(beginTime, endTime);
}

CopyCommand::~CopyCommand()
{
    delete m_sourceClipboard;
    delete m_savedClipboard;
}

void
CopyCommand::execute()
{
    if (!m_savedClipboard) {
        m_savedClipboard = new Clipboard(*m_targetClipboard);
    }

    m_targetClipboard->copyFrom(m_sourceClipboard);
}

void
CopyCommand::unexecute()
{
    m_targetClipboard->copyFrom(m_savedClipboard);
}

}
