// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include <kmessagebox.h>

#include "BaseProperties.h"
#include "SegmentMatrixHelper.h"

#include "matrixtool.h"
#include "matrixview.h"
#include "matrixstaff.h"

#include "rosedebug.h"

using Rosegarden::EventSelection;

//////////////////////////////////////////////////////////////////////
//                     MatrixToolBox
//////////////////////////////////////////////////////////////////////

MatrixToolBox::MatrixToolBox(MatrixView* parent)
    : EditToolBox(parent),
      m_mParentView(parent)
{
}

EditTool* MatrixToolBox::createTool(const QString& toolName)
{
    MatrixTool* tool = 0;

    QString toolNamelc = toolName.lower();

    if (toolNamelc == MatrixPainter::ToolName)

        tool = new MatrixPainter(m_mParentView);

    else if (toolNamelc == MatrixEraser::ToolName)

        tool = new MatrixEraser(m_mParentView);

    else if (toolNamelc == MatrixSelector::ToolName)

        tool = new MatrixSelector(m_mParentView);

    else {
        KMessageBox::error(0, QString("NotationToolBox::createTool : unrecognised toolname %1 (%2)")
                           .arg(toolName).arg(toolNamelc));
        return 0;
    }

    m_tools.insert(toolName, tool);

    return tool;
    
}

//////////////////////////////////////////////////////////////////////
//                     MatrixTools
//////////////////////////////////////////////////////////////////////

MatrixTool::MatrixTool(const QString& menuName, MatrixView* parent)
    : EditTool(menuName, parent),
      m_mParentView(parent)
{
}


using Rosegarden::Event;
using Rosegarden::Note;
using Rosegarden::timeT;

#include "basiccommand.h"
#include "Segment.h"

class MatrixInsertionCommand : public BasicCommand
{
public:
    MatrixInsertionCommand(Rosegarden::Segment &segment,
                           timeT time,
                           timeT endTime,
                           MatrixStaff*,
                           Rosegarden::Event *event);

    virtual ~MatrixInsertionCommand();
    
protected:
    virtual void modifySegment();

    MatrixStaff* m_staff;
    Rosegarden::Event* m_event;
    bool m_firstModify;
};

MatrixInsertionCommand::MatrixInsertionCommand(Rosegarden::Segment &segment,
                                               timeT time,
                                               timeT endTime,
                                               MatrixStaff* staff,
                                               Event *event) :
    BasicCommand("Insert Note", segment, time, endTime),
    m_staff(staff),
    m_event(event),
    m_firstModify(true)
{
    // nothing
}

MatrixInsertionCommand::~MatrixInsertionCommand()
{
    if (!m_firstModify) // we made our own copy of the event so delete it
        delete m_event;
}

void
MatrixInsertionCommand::modifySegment()
{
    kdDebug(KDEBUG_AREA) << "MatrixInsertionCommand::modifySegment()\n";

    Rosegarden::SegmentMatrixHelper helper(getSegment());

    if (m_firstModify) {
        m_staff->setWrapAddedEvents(false); // this makes insertion not to create a new MatrixElement
        m_firstModify = false;
    } else
        m_staff->setWrapAddedEvents(true);

    helper.insertNote(m_event);
    m_staff->setWrapAddedEvents(true);

    // We need to make a copy of the event each time
    // because if we're called again it means the insertion has been
    // undone, and therefore the inserted event has been deleted
    // This copy is made so that we still have a valid event
    // to insert if we're called again.
    //
    m_event = new Event(*m_event);
    
}

//------------------------------

class MatrixEraseCommand : public BasicCommand
{
public:
    MatrixEraseCommand(Rosegarden::Segment &segment,
                       MatrixStaff*,
                       Rosegarden::Event *event);

    virtual Rosegarden::timeT getRelayoutEndTime();

protected:
    virtual void modifySegment();

    MatrixStaff* m_staff;
    Rosegarden::Event *m_event; // only used on 1st execute (cf bruteForceRedo)
    Rosegarden::timeT m_relayoutEndTime;
};

MatrixEraseCommand::MatrixEraseCommand(Rosegarden::Segment &segment,
                                       MatrixStaff* staff,
                                       Event *event) :
    BasicCommand("Erase Note",
                 segment,
		 event->getAbsoluteTime(),
		 event->getAbsoluteTime() + event->getDuration(),
		 true),
    m_staff(staff),
    m_event(event),
    m_relayoutEndTime(getEndTime())
{
    // nothing
}

timeT
MatrixEraseCommand::getRelayoutEndTime()
{
    return m_relayoutEndTime;
}

void
MatrixEraseCommand::modifySegment()
{
    Rosegarden::SegmentMatrixHelper helper(getSegment());

    std::string eventType = m_event->getType();

    if (eventType == Note::EventType) {

	helper.deleteNote(m_event, false);
	return;

    }
}


//------------------------------


MatrixPainter::MatrixPainter(MatrixView* parent)
    : MatrixTool("MatrixPainter", parent),
      m_currentElement(0),
      m_currentStaff(0),
      m_resolution(Note::QuarterNote),
      m_basicDuration(0)
{
    Note tmpNote(m_resolution);

    m_basicDuration = tmpNote.getDuration();
}

void MatrixPainter::handleLeftButtonPress(Rosegarden::timeT time,
                                          int pitch,
                                          int staffNo,
                                          QMouseEvent*,
                                          Rosegarden::ViewElement*)
{
    kdDebug(KDEBUG_AREA) << "MatrixPainter::handleLeftButtonPress : pitch = "
                         << pitch << ", time : " << time << endl;

    Note newNote(m_resolution);

    // Round event time to a multiple of resolution
    timeT noteDuration = newNote.getDuration();
    
    time = (time / noteDuration) * noteDuration;

    Event* el = newNote.getAsNoteEvent(time, pitch);

    // set a default velocity
    using Rosegarden::BaseProperties::VELOCITY;
    el->set<Rosegarden::Int>(VELOCITY, 100);

    m_currentElement = new MatrixElement(el);

    m_currentStaff = m_mParentView->getStaff(staffNo);

    int y = m_currentStaff->getLayoutYForHeight(pitch) - m_currentStaff->getElementHeight() / 2;

    m_currentElement->setLayoutY(y);
    m_currentElement->setLayoutX(time * m_currentStaff->getTimeScaleFactor());
    m_currentElement->setHeight(m_currentStaff->getElementHeight());

    double width = noteDuration * m_currentStaff->getTimeScaleFactor();
    m_currentElement->setWidth(int(width));

    m_currentStaff->positionElement(m_currentElement);
    m_mParentView->update();
}

void MatrixPainter::handleMouseMove(Rosegarden::timeT newTime,
                                    int,
                                    QMouseEvent*)
{
    // sanity check
    if (!m_currentElement) return;

    //!!! Rather than using m_basicDuration as the unit for painting,
    // we should probably be using a Rosegarden::SnapGrid with an
    // appropriate snap time (SnapToUnit as a default, for example).
    // SnapGrid requires a RulerScale -- but we have one of those, as
    // HorizontalLayout subclasses RulerScale.  Apart from anything
    // else, using SnapGrid will ensure that time-signature changes
    // are handled correctly (at least if MatrixHLayout handles them,
    // which it presently doesn't but needs to be made to anyway)

    newTime = (newTime / m_basicDuration) * m_basicDuration;

    if (newTime == m_currentElement->getAbsoluteTime()) return;

    timeT newDuration = newTime - m_currentElement->getAbsoluteTime();

    using Rosegarden::BaseProperties::PITCH;

    kdDebug(KDEBUG_AREA) << "MatrixPainter::handleMouseMove : new time = "
                         << newTime << ", old time = "
                         << m_currentElement->getAbsoluteTime()
                         << ", new duration = "
                         << newDuration
                         << ", pitch = "
                         << m_currentElement->event()->get<Rosegarden::Int>(PITCH)
                         << endl;

    m_currentElement->setDuration(newDuration);

    double width = newDuration * m_currentStaff->getTimeScaleFactor();
    m_currentElement->setWidth(int(width));

    m_mParentView->update();
}

void MatrixPainter::handleMouseRelease(Rosegarden::timeT,
                                       int,
                                       QMouseEvent*)
{
    // This can happen in case of screen/window capture - we only get a mouse release,
    // the window snapshot tool got the mouse down
    if (!m_currentElement) return;

    // Insert element if it has a non null duration,
    // discard it otherwise
    //
    if (m_currentElement->getDuration() != 0) {
        
        Rosegarden::SegmentMatrixHelper helper(m_currentStaff->getSegment());
        kdDebug(KDEBUG_AREA) << "MatrixPainter::handleMouseRelease() : helper.insertNote()\n";

        //         m_currentStaff->setWrapAddedEvents(false);
        //         helper.insertNote(m_currentElement->event());
        //         m_currentStaff->setWrapAddedEvents(true);

        timeT time = m_currentElement->getAbsoluteTime();
        timeT endTime = time + m_currentElement->getDuration();

        MatrixInsertionCommand* command = 
            new MatrixInsertionCommand(m_currentStaff->getSegment(),
                                       time, endTime,
                                       m_currentStaff,
                                       m_currentElement->event());


        m_mParentView->addCommandToHistory(command);

        m_currentStaff->getViewElementList()->insert(m_currentElement);

        m_mParentView->update();
        
    } else {

        Event* ev = m_currentElement->event();
        delete m_currentElement;
        delete ev;
    }
    
    m_currentElement = 0;

}

void MatrixPainter::slotSetResolution(Rosegarden::Note::Type note)
{
    m_resolution = note;
}

//------------------------------

MatrixEraser::MatrixEraser(MatrixView* parent)
    : MatrixTool("MatrixEraser", parent),
      m_currentStaff(0)
{
}

void MatrixEraser::handleLeftButtonPress(Rosegarden::timeT,
                                         int,
                                         int staffNo,
                                         QMouseEvent*,
                                         Rosegarden::ViewElement* el)
{
    kdDebug(KDEBUG_AREA) << "MatrixEraser::handleLeftButtonPress : el = "
                         << el << endl;

    if (!el) return; // nothing to erase

    m_currentStaff = m_mParentView->getStaff(staffNo);

    MatrixEraseCommand* command =
        new MatrixEraseCommand(m_currentStaff->getSegment(),
                               m_currentStaff, el->event());

    m_mParentView->addCommandToHistory(command);

    m_mParentView->update();
}

//------------------------------

MatrixSelector::MatrixSelector(MatrixView* view)
    : MatrixTool("MatrixSelector", view),
      m_selectionRect(0),
      m_updateRect(false),
      m_currentStaff(0),
      m_clickedElement(0)
{
    connect(m_parentView, SIGNAL(usedSelection()),
            this,         SLOT(slotHideSelection()));
}

void MatrixSelector::handleLeftButtonPress(Rosegarden::timeT,
                                           int,
                                           int staffNo,
                                           QMouseEvent* e,
                                           Rosegarden::ViewElement *element)
{
    kdDebug(KDEBUG_AREA) << "MatrixSelector::handleMousePress" << endl;

    m_currentStaff = m_mParentView->getStaff(staffNo);

    m_clickedElement = dynamic_cast<MatrixElement*>(element);

    m_selectionRect->setX(e->x());
    m_selectionRect->setY(e->y());
    m_selectionRect->setSize(0,0);

    m_selectionRect->show();
    m_updateRect = true;

    //m_parentView->setCursorPosition(p.x());
}

// void MatrixSelector::handleMouseDblClick(Rosegarden::timeT,
//                                            int,
//                                            int staffNo,
//                                            QMouseEvent* e,
//                                            ViewElement *element)
// {
//     kdDebug(KDEBUG_AREA) << "MatrixSelector::handleMouseDblClick" << endl;
//     m_clickedStaff = staffNo;
//     m_clickedElement = dynamic_cast<MatrixElement*>(element);
    
//     MatrixStaff *staff = m_mParentView->getStaff(staffNo);
//     if (!staff) return;

//     QRect rect = staff->getBarExtents(e->x(), e->y());

//     m_selectionRect->setX(rect.x() + 1);
//     m_selectionRect->setY(rect.y());
//     m_selectionRect->setSize(rect.width() - 1, rect.height());

//     m_selectionRect->show();
//     m_updateRect = false;
//     return;
// }

void MatrixSelector::handleMouseMove(timeT, int,
                                     QMouseEvent* e)
{
    if (!m_updateRect) return;

    int w = int(e->x() - m_selectionRect->x());
    int h = int(e->y() - m_selectionRect->y());

    // Qt rectangle dimensions appear to be 1-based
    if (w > 0) ++w; else --w;
    if (h > 0) ++h; else --h;

    m_selectionRect->setSize(w,h);

    m_mParentView->canvas()->update();
}

void MatrixSelector::handleMouseRelease(timeT, int, QMouseEvent*)
{
    kdDebug(KDEBUG_AREA) << "MatrixSelector::handleMouseRelease" << endl;
    m_updateRect = false;
    setViewCurrentSelection();

    // If we didn't drag out a meaningful area, but _did_ click on
    // an individual event, then select just that event
    
//     if (m_selectionRect->width()  > -3 &&
//         m_selectionRect->width()  <  3 &&
//         m_selectionRect->height() > -3 &&
//         m_selectionRect->height() <  3) {

// 	m_selectionRect->hide();

// 	if (m_clickedElement != 0 &&
// 	    m_clickedStaff   >= 0) {

// 	    m_mParentView->setSingleSelectedEvent
// 		(m_clickedStaff, m_clickedElement->event());
// 	}
//     }
}

void MatrixSelector::ready()
{
    m_selectionRect = new QCanvasRectangle(m_mParentView->canvas());
    
    m_selectionRect->hide();
    m_selectionRect->setPen(RosegardenGUIColours::SelectionRectangle);

    m_mParentView->setCanvasCursor(Qt::arrowCursor);
//     m_mParentView->setPositionTracking(false);
}

void MatrixSelector::stow()
{
    delete m_selectionRect;
    m_selectionRect = 0;
    m_mParentView->canvas()->update();
}


void MatrixSelector::slotHideSelection()
{
    if (!m_selectionRect) return;
    m_selectionRect->hide();
    m_selectionRect->setSize(0,0);
    m_mParentView->canvas()->update();
}

void MatrixSelector::setViewCurrentSelection()
{
    EventSelection* selection = getSelection();
    m_mParentView->setCurrentSelection(selection);
}

EventSelection* MatrixSelector::getSelection()
{
    // If selection rect is not visible or too small,
    // return 0
    //
    if (!m_selectionRect->visible()) return 0;

    //    kdDebug(KDEBUG_AREA) << "Selection x,y: " << m_selectionRect->x() << ","
    //                         << m_selectionRect->y() << "; w,h: " << m_selectionRect->width() << "," << m_selectionRect->height() << endl;

    if (m_selectionRect->width()  > -3 &&
        m_selectionRect->width()  <  3 &&
        m_selectionRect->height() > -3 &&
        m_selectionRect->height() <  3) return 0;

    if (!m_currentStaff) return 0;

    Rosegarden::Segment& originalSegment = m_currentStaff->getSegment();
    
    EventSelection* selection = new EventSelection(originalSegment);

    QCanvasItemList itemList = m_selectionRect->collisions(true);
    QCanvasItemList::Iterator it;

    QRect rect = m_selectionRect->rect().normalize();

    for (it = itemList.begin(); it != itemList.end(); ++it) {

        QCanvasItem *item = *it;
        QCanvasMatrixRectangle *matrixRect = 0;
        
        if ((matrixRect = dynamic_cast<QCanvasMatrixRectangle*>(item))) {

            if (!rect.contains(matrixRect->rect(), true)) {

                kdDebug(KDEBUG_AREA) << "MatrixSelector::getSelection Skipping item not really in selection rect\n";
                kdDebug(KDEBUG_AREA) << "MatrixSelector::getSelection Rect: x,y: " << rect.x() << ","
                                     << rect.y() << "; w,h: " << rect.width()
                                     << "," << rect.height() << " / Item: x,y: "
                                     << item->x() << "," << item->y() << endl;
                continue;
            } else {
                
                kdDebug(KDEBUG_AREA) << "MatrixSelector::getSelection Item in rect : Rect: x,y: " << rect.x() << ","
                                     << rect.y() << "; w,h: " << rect.width()
                                     << "," << rect.height() << " / Item: x,y: "
                                     << item->x() << "," << item->y() << endl;
            }
            
            selection->addEvent(matrixRect->getMatrixElement().event());

            //             kdDebug(KDEBUG_AREA) << "Selected event : \n";
            //             el.event()->dump(std::cerr);
        }
        
    }

    return (selection->getAddedEvents() > 0) ? selection : 0;
}


const QString MatrixPainter::ToolName = "painter";
const QString MatrixEraser::ToolName  = "eraser";
const QString MatrixSelector::ToolName  = "selector";

