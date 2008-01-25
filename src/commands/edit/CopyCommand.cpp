/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2008
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


#include "CopyCommand.h"

#include "misc/Strings.h"
#include "base/Clipboard.h"
#include "base/Composition.h"
#include "base/Selection.h"
#include <qstring.h>


namespace Rosegarden
{

CopyCommand::CopyCommand(EventSelection &selection,
                         Clipboard *clipboard) :
        KNamedCommand(getGlobalName()),
        m_targetClipboard(clipboard)
{
    m_sourceClipboard = new Clipboard;
    m_savedClipboard = 0;
    m_sourceClipboard->newSegment(&selection)->setLabel
    (selection.getSegment().getLabel() + " " + qstrtostr(i18n("(excerpt)")));
}

CopyCommand::CopyCommand(SegmentSelection &selection,
                         Clipboard *clipboard) :
        KNamedCommand(getGlobalName()),
        m_targetClipboard(clipboard)
{
    m_sourceClipboard = new Clipboard;
    m_savedClipboard = 0;

    for (SegmentSelection::iterator i = selection.begin();
            i != selection.end(); ++i) {
        QString newLabel = strtoqstr((*i)->getLabel());
        if (newLabel.contains(i18n("(copied)"))) {
            m_sourceClipboard->newSegment(*i);
        } else {
            m_sourceClipboard->newSegment(*i)->
            setLabel(qstrtostr(i18n("%1 (copied)").arg(newLabel)));
        }
    }
}

CopyCommand::CopyCommand(Composition *composition,
                         timeT beginTime,
                         timeT endTime,
                         Clipboard *clipboard) :
        KNamedCommand(i18n("Copy Range")),
        m_targetClipboard(clipboard)
{
    m_sourceClipboard = new Clipboard;
    m_savedClipboard = 0;

    for (Composition::iterator i = composition->begin();
            i != composition->end(); ++i) {
        if ((*i)->getStartTime() < endTime &&
                (*i)->getRepeatEndTime() > beginTime) {
            m_sourceClipboard->newSegment(*i, beginTime, endTime, true);
        }
    }

    TimeSignatureSelection tsigsel
    (*composition, beginTime, endTime, true);
    m_sourceClipboard->setTimeSignatureSelection(tsigsel);

    TempoSelection temposel
    (*composition, beginTime, endTime, true);
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
