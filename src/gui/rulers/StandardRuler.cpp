/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "StandardRuler.h"

#include <klocale.h>
#include "misc/Debug.h"
#include "MarkerRuler.h"
#include "base/RulerScale.h"
#include "document/RosegardenGUIDoc.h"
#include "document/MultiViewCommandHistory.h"
#include "gui/application/RosegardenGUIApp.h"
#include "gui/general/GUIPalette.h"
#include "gui/rulers/LoopRuler.h"
#include "document/RosegardenGUIDoc.h"
#include <qobject.h>
#include <qtooltip.h>
#include <qvbox.h>
#include <qwidget.h>


namespace Rosegarden
{

StandardRuler::StandardRuler(RosegardenGUIDoc *doc,
                             RulerScale *rulerScale,
                             double xorigin,
                             int barHeight,
                             bool invert,
                             QWidget* parent,
                             const char* name,
                             WFlags f):
        QVBox(parent, name, f),
        m_invert(invert),
        m_loopRulerHeight(10),
        m_currentXOffset(0),
        m_doc(doc),
        m_rulerScale(rulerScale),
        m_hButtonBar(0)
{
    setSpacing(0);

    if (!m_invert) {
        m_hButtonBar = new MarkerRuler
                       (m_doc, m_rulerScale, barHeight - m_loopRulerHeight, xorigin, this);
    }

    m_loopRuler = new LoopRuler
                  (m_doc, m_rulerScale, m_loopRulerHeight, xorigin, m_invert, this, name);

    if (m_invert) {
        m_hButtonBar = new MarkerRuler
                       (m_doc, m_rulerScale, barHeight - m_loopRulerHeight, xorigin, this);
    }

    QObject::connect
        (doc->getCommandHistory(), SIGNAL(commandExecuted()),
         this, SLOT(update()));

}

void StandardRuler::setSnapGrid(SnapGrid *grid)
{
    m_loopRuler->setSnapGrid(grid);
}

void StandardRuler::connectRulerToDocPointer(RosegardenGUIDoc *doc)
{

    RG_DEBUG << "StandardRuler::connectRulerToDocPointer" << endl;

    // use the document as a hub for pointer and loop set related signals
    // pointer and loop drag signals are specific to the current view,
    // so they are re-emitted from the loop ruler by this widget
    //
    QObject::connect
    (m_loopRuler, SIGNAL(setPointerPosition(timeT)),
     doc, SLOT(slotSetPointerPosition(timeT)));

    QObject::connect
    (m_hButtonBar, SIGNAL(setPointerPosition(timeT)),
     doc, SLOT(slotSetPointerPosition(timeT)));

    QObject::connect
    (m_hButtonBar, SIGNAL(editMarkers()),
     RosegardenGUIApp::self(), SLOT(slotEditMarkers()));

    QObject::connect
    (m_hButtonBar, SIGNAL(addMarker(timeT)),
     RosegardenGUIApp::self(), SLOT(slotAddMarker(timeT)));

    QObject::connect
    (m_hButtonBar, SIGNAL(deleteMarker(int, timeT, QString, QString)),
     RosegardenGUIApp::self(), SLOT(slotDeleteMarker(int, timeT, QString, QString)));

    QObject::connect
    (m_loopRuler, SIGNAL(dragPointerToPosition(timeT)),
     this, SIGNAL(dragPointerToPosition(timeT)));

    QObject::connect
    (m_loopRuler, SIGNAL(dragLoopToPosition(timeT)),
     this, SIGNAL(dragLoopToPosition(timeT)));

    QObject::connect
    (m_loopRuler, SIGNAL(setPlayPosition(timeT)),
     RosegardenGUIApp::self(), SLOT(slotSetPlayPosition(timeT)));

    QObject::connect
    (m_hButtonBar, SIGNAL(setLoop(timeT, timeT)),
     doc, SLOT(slotSetLoop(timeT, timeT)));

    QObject::connect
    (m_loopRuler, SIGNAL(setLoop(timeT, timeT)),
     doc, SLOT(slotSetLoop(timeT, timeT)));

    QObject::connect
    (doc, SIGNAL(loopChanged(timeT, timeT)),
     m_loopRuler,
     SLOT(slotSetLoopMarker(timeT, timeT)));

    m_loopRuler->setBackgroundColor(GUIPalette::getColour(GUIPalette::PointerRuler));
}

void StandardRuler::slotScrollHoriz(int x)
{
    m_loopRuler->scrollHoriz(x);
    m_hButtonBar->scrollHoriz(x);
}

void StandardRuler::setMinimumWidth(int width)
{
    m_hButtonBar->setMinimumWidth(width);
    m_loopRuler->setMinimumWidth(width);
}

void StandardRuler::setHScaleFactor(double dy)
{
    m_hButtonBar->setHScaleFactor(dy);
    m_loopRuler->setHScaleFactor(dy);
}

void StandardRuler::paintEvent(QPaintEvent *e)
{
    m_hButtonBar->update();
    m_loopRuler->update();
    QWidget::paintEvent(e);
}

}
#include "StandardRuler.moc"
