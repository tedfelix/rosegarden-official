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


#include <Q3CanvasPixmap>
#include "MatrixPainter.h"

#include "base/BaseProperties.h"
#include <klocale.h>
#include <kstandarddirs.h>
#include "base/Event.h"
#include "base/NotationTypes.h"
#include "base/SegmentMatrixHelper.h"
#include "base/SnapGrid.h"
#include "base/ViewElement.h"
#include "commands/matrix/MatrixInsertionCommand.h"
#include "commands/matrix/MatrixEraseCommand.h"
#include "commands/matrix/MatrixPercussionInsertionCommand.h"
#include "gui/general/EditTool.h"
#include "gui/general/RosegardenCanvasView.h"
#include "MatrixElement.h"
#include "MatrixStaff.h"
#include "MatrixTool.h"
#include "MatrixView.h"
#include <kaction.h>
#include <kglobal.h>
#include <QIcon>
#include <QPoint>
#include <QString>
#include "misc/Debug.h"


namespace Rosegarden
{

MatrixPainter::MatrixPainter(MatrixView* parent)
        : MatrixTool("MatrixPainter", parent),
        m_currentElement(0),
        m_currentStaff(0)
{
    QString pixmapDir = KGlobal::dirs()->findResource("appdata", "pixmaps/");
    Q3CanvasPixmap pixmap(pixmapDir + "/toolbar/select.xpm");
    QIcon icon = QIcon(pixmap);

    new KAction(i18n("Switch to Select Tool"), icon, Key_F2, this,
                SLOT(slotSelectSelected()), actionCollection(),
                "select");

    new KAction(i18n("Switch to Erase Tool"), "eraser", Key_F4, this,
                SLOT(slotEraseSelected()), actionCollection(),
                "erase");

    new KAction(i18n("Switch to Move Tool"), "move", Key_F5, this,
                SLOT(slotMoveSelected()), actionCollection(),
                "move");

    pixmap.load(pixmapDir + "/toolbar/resize.xpm");
    icon = QIcon(pixmap);
    new KAction(i18n("Switch to Resize Tool"), icon, Key_F6, this,
                SLOT(slotResizeSelected()), actionCollection(),
                "resize");

    createMenu("matrixpainter.rc");
}

MatrixPainter::MatrixPainter(QString name, MatrixView* parent)
        : MatrixTool(name, parent),
        m_currentElement(0),
        m_currentStaff(0)
{}

void MatrixPainter::handleEventRemoved(Event *event)
{
    if (m_currentElement && m_currentElement->event() == event) {
        m_currentElement = 0;
    }
}

void MatrixPainter::handleLeftButtonPress(timeT time,
        int pitch,
        int staffNo,
        QMouseEvent *e,
        ViewElement *element)
{
    MATRIX_DEBUG << "MatrixPainter::handleLeftButtonPress : pitch = "
                 << pitch << ", time : " << time << endl;

    QPoint p = m_mParentView->inverseMapPoint(e->pos());

    m_currentStaff = m_mParentView->getStaff(staffNo);

    // Don't create an overlapping event on the same note on the same channel
    if (dynamic_cast<MatrixElement*>(element)) {
        std::cerr << "MatrixPainter::handleLeftButtonPress : overlap with an other matrix element" << std::endl;
        // In percussion matrix, we delete the existing event rather
        // than just ignoring it -- this is reasonable as the event
        // has no meaningful duration, so we can just toggle it on and
        // off with repeated clicks
        if (m_mParentView->isDrumMode()) {
            if (element->event()) {
                MatrixEraseCommand *command =
                    new MatrixEraseCommand(m_currentStaff->getSegment(),
                                           element->event());
                m_mParentView->addCommandToHistory(command);
            }
        }
        m_currentElement = 0;
        return ;
    }

    // This is needed for the event duration rounding
    SnapGrid grid(getSnapGrid());

    Event *ev = new Event(Note::EventType, time,
                          grid.getSnapTime(double(p.x())));
    ev->set<Int>(BaseProperties::PITCH, pitch);
    ev->set<Int>(BaseProperties::VELOCITY, m_mParentView->getCurrentVelocity());

    m_currentElement = new MatrixElement(ev, m_mParentView->isDrumMode());

    int y = m_currentStaff->getLayoutYForHeight(pitch) -
            m_currentStaff->getElementHeight() / 2;

    m_currentElement->setLayoutY(y);
    m_currentElement->setLayoutX(grid.getRulerScale()->getXForTime(time));
    m_currentElement->setHeight(m_currentStaff->getElementHeight());

    int width = grid.getRulerScale()->getXForTime(time + ev->getDuration())
        - m_currentElement->getLayoutX() + 1;

    m_currentElement->setWidth(width);

    m_currentStaff->positionElement(m_currentElement);
    m_mParentView->update();

    // preview
    m_mParentView->playNote(ev);
}

int MatrixPainter::handleMouseMove(timeT time,
                                   int pitch,
                                   QMouseEvent *e)
{
    // sanity check
    if (!m_currentElement)
        return RosegardenCanvasView::NoFollow;

    if (getSnapGrid().getSnapSetting() != SnapGrid::NoSnap) {
        setContextHelp(i18n("Hold Shift to avoid snapping to beat grid"));
    } else {
        clearContextHelp();
    }

    // We don't want to use the time passed in, because it's snapped
    // to the left and we want a more particular policy

    if (e) {
        QPoint p = m_mParentView->inverseMapPoint(e->pos());
        time = getSnapGrid().snapX(p.x(), SnapGrid::SnapEither);
        if (time >= m_currentElement->getViewAbsoluteTime()) {
            time = getSnapGrid().snapX(p.x(), SnapGrid::SnapRight);
        } else {
            time = getSnapGrid().snapX(p.x(), SnapGrid::SnapLeft);
        }            
    }

    MATRIX_DEBUG << "MatrixPainter::handleMouseMove : pitch = "
                 << pitch << ", time : " << time << endl;

    using BaseProperties::PITCH;

    if (time == m_currentElement->getViewAbsoluteTime()) {
        time =
            m_currentElement->getViewAbsoluteTime() +
            m_currentElement->getViewDuration();
    }

    int width = getSnapGrid().getRulerScale()->getXForTime(time)
        - getSnapGrid().getRulerScale()->getXForTime
        (m_currentElement->getViewAbsoluteTime()) + 1;

    m_currentElement->setWidth(width);
//    std::cerr << "current element width "<< width << std::endl;

    if (m_currentElement->event()->has(PITCH) &&
        pitch != m_currentElement->event()->get<Int>(PITCH)) {

        m_currentElement->event()->set<Int>(PITCH, pitch);

        int y = m_currentStaff->getLayoutYForHeight(pitch) -
                m_currentStaff->getElementHeight() / 2;

        m_currentElement->setLayoutY(y);

        m_currentStaff->positionElement(m_currentElement);

        // preview
        m_mParentView->playNote(m_currentElement->event());
    }

    m_mParentView->update();

    return RosegardenCanvasView::FollowHorizontal |
           RosegardenCanvasView::FollowVertical;
}

void MatrixPainter::handleMouseRelease(timeT endTime,
                                       int,
                                       QMouseEvent *e)
{
    // This can happen in case of screen/window capture -
    // we only get a mouse release, the window snapshot tool
    // got the mouse down
    if (!m_currentElement)
        return ;

    // We don't want to use the time passed in, because it's snapped
    // to the left and we want a more particular policy

    if (e) {
        QPoint p = m_mParentView->inverseMapPoint(e->pos());
        endTime = getSnapGrid().snapX(p.x(), SnapGrid::SnapEither);
        if (endTime >= m_currentElement->getViewAbsoluteTime()) {
            endTime = getSnapGrid().snapX(p.x(), SnapGrid::SnapRight);
        } else {
            endTime = getSnapGrid().snapX(p.x(), SnapGrid::SnapLeft);
        }            
    }

    timeT time = m_currentElement->getViewAbsoluteTime();
    timeT segmentEndTime = m_currentStaff->getSegment().getEndMarkerTime();

    if (m_mParentView->isDrumMode()) {

        if (time > segmentEndTime)
            time = segmentEndTime;

        MatrixPercussionInsertionCommand *command =
            new MatrixPercussionInsertionCommand(m_currentStaff->getSegment(),
                                                 time,
                                                 m_currentElement->event());
        m_mParentView->addCommandToHistory(command);

        Event* ev = m_currentElement->event();
        delete m_currentElement;
        delete ev;

        ev = command->getLastInsertedEvent();
        if (ev)
            m_mParentView->setSingleSelectedEvent(m_currentStaff->getSegment(),
                                                  ev);
    } else {

        // Insert element if it has a non null duration,
        // discard it otherwise
        //
        if (time > endTime)
            std::swap(time, endTime);

        if (endTime == time)
            endTime = time + m_currentElement->getViewDuration();

        if (time < segmentEndTime) {

            if (endTime > segmentEndTime)
                endTime = segmentEndTime;

            SegmentMatrixHelper helper(m_currentStaff->getSegment());
            MATRIX_DEBUG << "MatrixPainter::handleMouseRelease() : helper.insertNote()" << endl;

            MatrixInsertionCommand* command =
                new MatrixInsertionCommand(m_currentStaff->getSegment(),
                                           time,
                                           endTime,
                                           m_currentElement->event());

            m_mParentView->addCommandToHistory(command);

            Event* ev = m_currentElement->event();
            delete m_currentElement;
            delete ev;

            ev = command->getLastInsertedEvent();
            if (ev)
                m_mParentView->setSingleSelectedEvent(m_currentStaff->getSegment(),
                                                      ev);
        } else {

            Event* ev = m_currentElement->event();
            delete m_currentElement;
            delete ev;
        }
    }

    m_mParentView->update();
    m_currentElement = 0;

    setBasicContextHelp();
}

void MatrixPainter::ready()
{
    connect(m_parentView->getCanvasView(), SIGNAL(contentsMoving (int, int)),
            this, SLOT(slotMatrixScrolled(int, int)));

    m_mParentView->setCanvasCursor(Qt::crossCursor);

    setBasicContextHelp();
}

void MatrixPainter::stow()
{
    disconnect(m_parentView->getCanvasView(), SIGNAL(contentsMoving (int, int)),
               this, SLOT(slotMatrixScrolled(int, int)));
}

void MatrixPainter::slotMatrixScrolled(int newX, int newY)
{
    if (!m_currentElement)
        return ;

    QPoint newP1(newX, newY), oldP1(m_parentView->getCanvasView()->contentsX(),
                                    m_parentView->getCanvasView()->contentsY());

    QPoint offset = newP1 - oldP1;

    offset = m_mParentView->inverseMapPoint(offset);

    QPoint p(m_currentElement->getCanvasX() + m_currentElement->getWidth(), m_currentElement->getCanvasY());
    p += offset;

    timeT newTime = getSnapGrid().snapX(p.x());
    int newPitch = m_currentStaff->getHeightAtCanvasCoords(p.x(), p.y());

    handleMouseMove(newTime, newPitch, 0);
}

void MatrixPainter::setBasicContextHelp()
{
    if (getSnapGrid().getSnapSetting() != SnapGrid::NoSnap) {
        setContextHelp(i18n("Click and drag to draw a note; Shift to avoid snapping to grid"));
    } else {
        setContextHelp(i18n("Click and drag to draw a note"));
    }        
}

const QString MatrixPainter::ToolName   = "painter";

}
#include "MatrixPainter.moc"
