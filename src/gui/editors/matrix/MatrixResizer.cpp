/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2006
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


#include "MatrixResizer.h"

#include <klocale.h>
#include <kstddirs.h>
#include "base/Event.h"
#include "base/Segment.h"
#include "base/Selection.h"
#include "base/SnapGrid.h"
#include "base/ViewElement.h"
#include "commands/matrix/MatrixModifyCommand.h"
#include "commands/notation/NormalizeRestsCommand.h"
#include "gui/general/EditTool.h"
#include "gui/general/RosegardenCanvasView.h"
#include "MatrixElement.h"
#include "MatrixStaff.h"
#include "MatrixTool.h"
#include "MatrixView.h"
#include <kaction.h>
#include <kglobal.h>
#include <qiconset.h>
#include <qpoint.h>
#include <qstring.h>


namespace Rosegarden
{

MatrixResizer::MatrixResizer(MatrixView* parent)
        : MatrixTool("MatrixResizer", parent),
        m_currentElement(0),
        m_currentStaff(0)
{
    QString pixmapDir = KGlobal::dirs()->findResource("appdata", "pixmaps/");
    QCanvasPixmap pixmap(pixmapDir + "/toolbar/select.xpm");
    QIconSet icon = QIconSet(pixmap);

    new KAction(i18n("Switch to Select Tool"), icon, 0, this,
                SLOT(slotSelectSelected()), actionCollection(),
                "select");

    new KAction(i18n("Switch to Draw Tool"), "pencil", 0, this,
                SLOT(slotDrawSelected()), actionCollection(),
                "draw");

    new KAction(i18n("Switch to Erase Tool"), "eraser", 0, this,
                SLOT(slotEraseSelected()), actionCollection(),
                "erase");

    new KAction(i18n("Switch to Move Tool"), "move", 0, this,
                SLOT(slotMoveSelected()), actionCollection(),
                "move");

    createMenu("matrixresizer.rc");
}

void MatrixResizer::handleEventRemoved(Event *event)
{
    if (m_currentElement && m_currentElement->event() == event) {
        m_currentElement = 0;
    }
}

void MatrixResizer::handleLeftButtonPress(timeT,
        int,
        int staffNo,
        QMouseEvent* e,
        ViewElement* el)
{
    MATRIX_DEBUG << "MatrixResizer::handleLeftButtonPress() : el = "
    << el << endl;

    if (!el)
        return ; // nothing to erase

    m_currentElement = dynamic_cast<MatrixElement*>(el);
    m_currentStaff = m_mParentView->getStaff(staffNo);

    if (m_currentElement) {
        // Add this element and allow movement
        //
        EventSelection* selection = m_mParentView->getCurrentSelection();

        if (selection) {
            EventSelection *newSelection;

            if ((e->state() & Qt::ShiftButton) ||
                    selection->contains(m_currentElement->event()))
                newSelection = new EventSelection(*selection);
            else
                newSelection = new EventSelection(m_currentStaff->getSegment());

            newSelection->addEvent(m_currentElement->event());
            m_mParentView->setCurrentSelection(newSelection, true, true);
            m_mParentView->canvas()->update();
        } else {
            m_mParentView->setSingleSelectedEvent(m_currentStaff->getSegment(),
                                                  m_currentElement->event(),
                                                  true);
            m_mParentView->canvas()->update();
        }
    }
}

int MatrixResizer::handleMouseMove(timeT newTime,
                                   int,
                                   QMouseEvent *)
{
    if (!m_currentElement || !m_currentStaff)
        return RosegardenCanvasView::NoFollow;
    timeT newDuration = newTime - m_currentElement->getViewAbsoluteTime();

    if (newDuration == 0) {
        newDuration += m_mParentView->getSnapGrid().getSnapTime
                       (m_currentElement->getViewAbsoluteTime());
    }

    int initialWidth = m_currentElement->getWidth();
    double width = newDuration * m_currentStaff->getTimeScaleFactor();

    // Don't allow zero width here - always at least _fiddleFactor wide
    if (width > 0)
        width += _fiddleFactor;
    else if (width < 0)
        width -= _fiddleFactor;

    int diffWidth = initialWidth - int(width);

    EventSelection* selection = m_mParentView->getCurrentSelection();
    EventSelection::eventcontainer::iterator it =
        selection->getSegmentEvents().begin();

    MatrixElement *element = 0;
    for (; it != selection->getSegmentEvents().end(); it++) {
        element = m_currentStaff->getElement(*it);

        if (element) {
            int newWidth = element->getWidth() - diffWidth;

            MATRIX_DEBUG << "MatrixResizer::handleMouseMove - "
            << "new width = " << newWidth << endl;

            element->setWidth(newWidth);
            m_currentStaff->positionElement(element);
        }
    }

    m_mParentView->canvas()->update();
    return RosegardenCanvasView::FollowHorizontal;
}

void MatrixResizer::handleMouseRelease(timeT newTime,
                                       int, QMouseEvent*)
{
    if (!m_currentElement || !m_currentStaff)
        return ;

    timeT diffDuration =
        newTime - m_currentElement->getViewAbsoluteTime() -
        m_currentElement->getViewDuration();

    EventSelection *selection = m_mParentView->getCurrentSelection();

    if (selection->getAddedEvents() == 0)
        return ;
    else {
        QString commandLabel = i18n("Resize Event");

        if (selection->getAddedEvents() > 1)
            commandLabel = i18n("Resize Events");

        KMacroCommand *macro = new KMacroCommand(commandLabel);

        EventSelection::eventcontainer::iterator it =
            selection->getSegmentEvents().begin();

        Segment &segment = m_currentStaff->getSegment();

        EventSelection *newSelection = new EventSelection(segment);

        timeT normalizeStart = selection->getStartTime();
        timeT normalizeEnd = selection->getEndTime();

        for (; it != selection->getSegmentEvents().end(); it++) {
            timeT eventTime = (*it)->getAbsoluteTime();
            timeT eventDuration = (*it)->getDuration() + diffDuration;


            MATRIX_DEBUG << "MatrixResizer::handleMouseRelease - "
            << "Time = " << eventTime
            << ", Duration = " << eventDuration << endl;


            if (eventDuration < 0) {
                eventTime += eventDuration;
                eventDuration = -eventDuration;
            }

            if (eventDuration == 0) {
                eventDuration += m_mParentView->getSnapGrid().getSnapTime
                                 (eventTime);
            }

            if (eventTime + eventDuration >= segment.getEndMarkerTime()) {
                eventTime = m_mParentView->getSnapGrid().snapTime
                            (segment.getEndMarkerTime() - 1, SnapGrid::SnapLeft);
                eventDuration = std::min(eventDuration,
                                         segment.getEndMarkerTime() - eventTime);
            }

            Event *newEvent =
                new Event(**it,
                          eventTime,
                          eventDuration);

            macro->addCommand(new MatrixModifyCommand(segment,
                              *it,
                              newEvent,
                              false,
                              false));

            newSelection->addEvent(newEvent);
        }

        normalizeStart = std::min(normalizeStart, newSelection->getStartTime());
        normalizeEnd = std::max(normalizeEnd, newSelection->getEndTime());

        macro->addCommand(new NormalizeRestsCommand(segment,
                          normalizeStart,
                          normalizeEnd));

        m_mParentView->setCurrentSelection(0, false, false);
        m_mParentView->addCommandToHistory(macro);
        m_mParentView->setCurrentSelection(newSelection, false, false);
    }

    m_mParentView->update();
    m_currentElement = 0;
}

void MatrixResizer::ready()
{
    connect(m_parentView->getCanvasView(), SIGNAL(contentsMoving (int, int)),
            this, SLOT(slotMatrixScrolled(int, int)));
}

void MatrixResizer::stow()
{
    disconnect(m_parentView->getCanvasView(), SIGNAL(contentsMoving (int, int)),
               this, SLOT(slotMatrixScrolled(int, int)));
}

void MatrixResizer::slotMatrixScrolled(int newX, int newY)
{
    QPoint newP1(newX, newY), oldP1(m_parentView->getCanvasView()->contentsX(),
                                    m_parentView->getCanvasView()->contentsY());

    QPoint p(newX, newY);

    if (newP1.x() > oldP1.x()) {
        p.setX(newX + m_parentView->getCanvasView()->visibleWidth());
    }

    p = m_mParentView->inverseMapPoint(p);
    int newTime = m_mParentView->getSnapGrid().snapX(p.x());
    handleMouseMove(newTime, 0, 0);
}

}
