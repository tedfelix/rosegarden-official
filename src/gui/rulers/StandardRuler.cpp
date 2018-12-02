/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[StandardRuler]"

#include "StandardRuler.h"

#include "misc/Debug.h"
#include "MarkerRuler.h"
#include "base/RulerScale.h"
#include "document/RosegardenDocument.h"
#include "document/CommandHistory.h"
#include "gui/application/RosegardenMainWindow.h"
#include "gui/general/GUIPalette.h"
#include "gui/rulers/LoopRuler.h"
#include "document/RosegardenDocument.h"

#include <QObject>
#include <QToolTip>
#include <QWidget>
#include <QVBoxLayout>


namespace Rosegarden
{

StandardRuler::StandardRuler(RosegardenDocument *doc,
                             RulerScale *rulerScale,
                             bool invert,
                             bool isForMainWindow,
                             QWidget* parent) :
        QWidget(parent),
        m_invert(invert),
        m_isForMainWindow(isForMainWindow),
        m_loopRulerHeight(10),
        m_currentXOffset(0),
        m_doc(doc),
        m_rulerScale(rulerScale),
        m_markerRuler(nullptr)
{
//    QString localStyle("QWidget { background-color: #EEEEEE; color: #000000; }");

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);
    layout->setSpacing(0);
    setLayout(layout);
	
    if (!m_invert) {
        m_markerRuler = new MarkerRuler
                       (m_doc, m_rulerScale, this);
        layout->addWidget(m_markerRuler);
    }

    m_loopRuler = new LoopRuler
                  (m_doc, m_rulerScale, m_loopRulerHeight, m_invert, m_isForMainWindow, this);
    layout->addWidget(m_loopRuler);

    if (m_invert) {
        m_markerRuler = new MarkerRuler
                       (m_doc, m_rulerScale, this);
        layout->addWidget(m_markerRuler);
    }

    QObject::connect
        (CommandHistory::getInstance(), SIGNAL(commandExecuted()),
         this, SLOT(update()));

    if (RosegardenMainWindow::self()) {
        QObject::connect
                (m_markerRuler, &MarkerRuler::editMarkers,
                 RosegardenMainWindow::self(), &RosegardenMainWindow::slotEditMarkers);

        QObject::connect
                (m_markerRuler, &MarkerRuler::addMarker,
                 RosegardenMainWindow::self(), &RosegardenMainWindow::slotAddMarker);

        QObject::connect
                (m_markerRuler, &MarkerRuler::deleteMarker,
                 RosegardenMainWindow::self(), &RosegardenMainWindow::slotDeleteMarker);

        QObject::connect
                (m_loopRuler, &LoopRuler::setPlayPosition,
                 RosegardenMainWindow::self(), &RosegardenMainWindow::slotSetPlayPosition);
    }
}

void StandardRuler::setSnapGrid(const SnapGrid *grid)
{
    m_loopRuler->setSnapGrid(grid);
}

void StandardRuler::connectRulerToDocPointer(RosegardenDocument *doc)
{
    RG_DEBUG << "StandardRuler::connectRulerToDocPointer";

    Q_ASSERT(m_loopRuler);
    Q_ASSERT(m_markerRuler);

    // use the document as a hub for pointer and loop set related signals
    // pointer and loop drag signals are specific to the current view,
    // so they are re-emitted from the loop ruler by this widget
    //
    QObject::connect
    (m_loopRuler, &LoopRuler::setPointerPosition,
     doc, &RosegardenDocument::slotSetPointerPosition);

    QObject::connect
    (m_markerRuler, &MarkerRuler::setPointerPosition,
     doc, &RosegardenDocument::slotSetPointerPosition);

    QObject::connect
    (m_loopRuler, &LoopRuler::dragPointerToPosition,
     this, &StandardRuler::dragPointerToPosition);

    QObject::connect
    (m_loopRuler, &LoopRuler::dragLoopToPosition,
     this, &StandardRuler::dragLoopToPosition);

    QObject::connect
    (m_markerRuler, &MarkerRuler::setLoop,
     doc, &RosegardenDocument::slotSetLoop);

    QObject::connect
    (m_loopRuler, &LoopRuler::setLoop,
     doc, &RosegardenDocument::slotSetLoop);

    QObject::connect
    (doc, &RosegardenDocument::loopChanged,
     m_loopRuler,
     &LoopRuler::slotSetLoopMarker);

//    m_loopRuler->setBackgroundColor(GUIPalette::getColour(GUIPalette::PointerRuler));
}

void StandardRuler::slotScrollHoriz(int x)
{
    m_loopRuler->scrollHoriz(x);
    m_markerRuler->scrollHoriz(x);
}

void StandardRuler::setMinimumWidth(int width)
{
    m_markerRuler->setMinimumWidth(width);
    m_loopRuler->setMinimumWidth(width);
}

void StandardRuler::updateStandardRuler()
{
    m_markerRuler->update();
    m_loopRuler->update();
}

}
