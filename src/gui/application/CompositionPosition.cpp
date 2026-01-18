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
    m_positionAsElapsedTime(RealTime::zero()),
    m_documentPosition(0)
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

RealTime CompositionPosition::getElapsedTime() const
{
    RG_DEBUG << "getPositionAsElapsedTime" << m_positionAsElapsedTime;
    return m_positionAsElapsedTime;
}

void CompositionPosition::setPositionForNewDocument(timeT time)
{
    // We cannot set the position immediately as the new document has
    // not been properly loaded yet. Park it here
    m_documentPosition = time;
}

void CompositionPosition::slotSet(timeT time)
{
    RG_DEBUG << "slotSet" << m_position << "->" << time;
    RosegardenDocument* doc = RosegardenDocument::currentDocument;
    if (! doc) return;

    // even if the time has not changed pass it on otherwise it may
    // mess up the trensport request system
    Composition& comp = doc->getComposition();
    m_position = time;
    m_positionAsElapsedTime = comp.getElapsedRealTime(time);
    RG_DEBUG << "slotSet" << m_positionAsElapsedTime;
    SequenceManager* sequenceManager = doc->getSequenceManager();
    sequenceManager->jumpTo(m_positionAsElapsedTime);

    emit changed(m_position);
}

void CompositionPosition::slotSetDocumentTime()
{
    // now set the time from the loaded document
    slotSet(m_documentPosition);
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
        RG_DEBUG << "slotUpdate new position position" << m_position;
        emit changed(m_position);
    }
}

void CompositionPosition::documentLoaded(RosegardenDocument* doc)
{
    RG_DEBUG << "documentLoaded" << doc;
    // Nw we can set the document position
    QTimer::singleShot(0, this, &CompositionPosition::slotSetDocumentTime);
}


}
