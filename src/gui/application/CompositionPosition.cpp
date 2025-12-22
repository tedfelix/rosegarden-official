/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

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

#define RG_MODULE_STRING "[CompositionPosition]"
#define RG_NO_DEBUG_PRINT

#include "CompositionPosition.h"

#include "base/Composition.h"
#include "document/RosegardenDocument.h"
#include "gui/seqmanager/SequenceManager.h"
#include "misc/Debug.h"
#include "sound/SequencerDataBlock.h"

#include <QTimer>

namespace Rosegarden
{

CompositionPosition::CompositionPosition():
    m_position(0),
    m_positionAsElapsedTime(RealTime::zero())
{
    RG_DEBUG << "ctor";
    m_updateTimer = new QTimer(this);
    connect(m_updateTimer, &QTimer::timeout,
            this, &CompositionPosition::slotUpdate);
    m_updateTimer->start(50);
}

CompositionPosition* CompositionPosition::getInstance()
{
    static CompositionPosition instance;

    return &instance;
}

timeT CompositionPosition::get() const
{
    RG_DEBUG << "getPosition" << m_position;
    return m_position;
}

RealTime CompositionPosition::getPositionAsElapsedTime() const
{
    RG_DEBUG << "getPositionAsElapsedTime";
    return m_positionAsElapsedTime;
}

void CompositionPosition::slotSetPosition(timeT time)
{
    RG_DEBUG << "slotSetPosition" << time;
    RosegardenDocument* doc = RosegardenDocument::currentDocument;
    if (! doc) return;

    if (time == m_position) return; // no change
    Composition& comp = doc->getComposition();
    m_position = time;
    m_positionAsElapsedTime = comp.getElapsedRealTime(time);
    SequenceManager* sequenceManager = doc->getSequenceManager();
    sequenceManager->jumpTo(m_positionAsElapsedTime);

    emit changed(m_position);
}

void CompositionPosition::slotUpdate()
{
    RosegardenDocument* doc = RosegardenDocument::currentDocument;
    if (! doc) return;

    Composition& comp = doc->getComposition();
    RealTime position =
        SequencerDataBlock::getInstance()->getPositionPointer();
    if (position != m_positionAsElapsedTime) {
        RG_DEBUG << "slotUpdate new position" << m_positionAsElapsedTime <<
            "->" << position;
        // a new position
        m_positionAsElapsedTime = position;
        m_position = comp.getElapsedTimeForRealTime(position);
        emit changed(m_position);
    }
}


}
