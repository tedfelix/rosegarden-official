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
#include <klocale.h>

#include "BaseProperties.h"
#include "SegmentMatrixHelper.h"
#include "Composition.h"

#include "matrixtool.h"
#include "matrixview.h"
#include "matrixstaff.h"

#include "rosestrings.h"
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

    else if (toolNamelc == MatrixMover::ToolName)

        tool = new MatrixMover(m_mParentView);

    else if (toolNamelc == MatrixResizer::ToolName)

        tool = new MatrixResizer(m_mParentView);

    else {
        KMessageBox::error(0, QString("MatrixToolBox::createTool : unrecognised toolname %1 (%2)")
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
    BasicCommand(i18n("Insert Note"), segment, time, endTime),
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

void MatrixInsertionCommand::modifySegment()
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
    BasicCommand(i18n("Erase Note"),
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

timeT MatrixEraseCommand::getRelayoutEndTime()
{
    return m_relayoutEndTime;
}

void MatrixEraseCommand::modifySegment()
{
    Rosegarden::SegmentMatrixHelper helper(getSegment());

    std::string eventType = m_event->getType();

    if (eventType == Note::EventType) {

	helper.deleteNote(m_event, false);

    }
}

//------------------------------

class MatrixMoveCommand : public BasicCommand
{
public:
    MatrixMoveCommand(Rosegarden::Segment &segment,
                      timeT newTime,
                      int newPitch,
                      MatrixStaff*,
                      Rosegarden::Event *event);

protected:
    virtual void modifySegment();


    timeT m_newTime;
    timeT m_oldTime;
    int m_newPitch;

    MatrixStaff* m_staff;
    Rosegarden::Event* m_event;
};

MatrixMoveCommand::MatrixMoveCommand(Rosegarden::Segment &segment,
                                     timeT newTime,
                                     int newPitch,
                                     MatrixStaff* staff,
                                     Rosegarden::Event *event)
    : BasicCommand(i18n("Move Note"),
                   segment,
                   std::min(newTime, event->getAbsoluteTime()), 
                   std::max(newTime + event->getDuration(), event->getAbsoluteTime() + event->getDuration()),
                   true),
    m_newTime(newTime),
    m_oldTime(event->getAbsoluteTime()),
    m_newPitch(newPitch),
    m_staff(staff),
    m_event(event)
{
}

void MatrixMoveCommand::modifySegment()
{
    std::string eventType = m_event->getType();

    if (eventType == Note::EventType) {

        // Create new event
        Rosegarden::Event *newEvent = new Rosegarden::Event(*m_event, m_newTime);
        newEvent->set<Rosegarden::Int>(Rosegarden::BaseProperties::PITCH, m_newPitch);

        // Delete old one
        getSegment().eraseSingle(m_event);

        // Insert new one
        getSegment().insert(newEvent);

        // Tidy up
        getSegment().normalizeRests(std::min(m_newTime, m_oldTime),
                                    std::max(m_newTime + newEvent->getDuration(), m_oldTime + newEvent->getDuration()));

    }
}

//------------------------------

class MatrixChangeDurationCommand : public BasicCommand
{
public:
    MatrixChangeDurationCommand(Rosegarden::Segment &segment,
                                timeT newDuration,
                                MatrixStaff*,
                                Rosegarden::Event *event);

protected:
    virtual void modifySegment();


    timeT m_newDuration;

    MatrixStaff* m_staff;
    Rosegarden::Event* m_event;
};

MatrixChangeDurationCommand::MatrixChangeDurationCommand(Rosegarden::Segment &segment,
                                                         timeT newDuration,
                                                         MatrixStaff* staff,
                                                         Rosegarden::Event *event)
    : BasicCommand(i18n("Change Note Duration"),
                   segment,
                   event->getAbsoluteTime(), 
                   event->getAbsoluteTime() + std::max(newDuration, event->getDuration()),
                   true),
    m_newDuration(newDuration),
    m_staff(staff),
    m_event(event)
{
}

void MatrixChangeDurationCommand::modifySegment()
{
    std::string eventType = m_event->getType();

    if (eventType == Note::EventType) {

        timeT oldDuration = m_event->getDuration();

        // We can't change the duration directly

        // Create new event
        Rosegarden::Event *newEvent = new Rosegarden::Event(*m_event,
                                                            m_event->getAbsoluteTime(),
                                                            m_newDuration);
        // Delete old one
        getSegment().eraseSingle(m_event);

        // Insert new one
        getSegment().insert(newEvent);

        // Tidy up
        getSegment().normalizeRests(newEvent->getAbsoluteTime(),
                                    newEvent->getAbsoluteTime() + std::max(m_newDuration, oldDuration));

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

MatrixPainter::MatrixPainter(QString name, MatrixView* parent)
    : MatrixTool(name, parent),
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

    // get staff and attempt to get segment start time
    m_currentStaff = m_mParentView->getStaff(staffNo);
    Rosegarden::timeT barStart = 0;

    if (m_currentStaff->getSegment().getComposition())
    {
        Rosegarden::Composition *comp =
            m_currentStaff->getSegment().getComposition();

        barStart = comp->getBarStart(
                       comp->getBarNumber(
                           m_currentStaff->getSegment().getStartTime()));
    }

    Event* el = newNote.getAsNoteEvent(time, pitch);

    time -= barStart;

    // set a default velocity
    using Rosegarden::BaseProperties::VELOCITY;
    el->set<Rosegarden::Int>(VELOCITY, 100);

    m_currentElement = new MatrixElement(el);

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
                                    int pitch,
                                    QMouseEvent*)
{
    // sanity check
    if (!m_currentElement) return;

    using Rosegarden::BaseProperties::PITCH;

    if (pitch == m_currentElement->event()->get<Rosegarden::Int>(PITCH))
    {
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
    
        m_mParentView->canvas()->update();
    }
    else
    {
        // destroy and recreate event on the same staff as we used above
        // but for a different pitch
        delete m_currentElement;

        Note newNote(m_resolution);

        // Round event time to a multiple of resolution
        timeT noteDuration = newNote.getDuration();
    
        newTime = (newTime / noteDuration) * noteDuration;

        Event* el = newNote.getAsNoteEvent(newTime, pitch);

        // set a default velocity
        using Rosegarden::BaseProperties::VELOCITY;
        el->set<Rosegarden::Int>(VELOCITY, 100);

        m_currentElement = new MatrixElement(el);

        //m_currentStaff = m_mParentView->getStaff(staffNo);

        int y = m_currentStaff->getLayoutYForHeight(pitch) - m_currentStaff->getElementHeight() / 2;

        m_currentElement->setLayoutY(y);
        m_currentElement->setLayoutX(newTime * m_currentStaff->getTimeScaleFactor());
        m_currentElement->setHeight(m_currentStaff->getElementHeight());

        double width = noteDuration * m_currentStaff->getTimeScaleFactor();
        m_currentElement->setWidth(int(width));

        m_currentStaff->positionElement(m_currentElement);
        m_mParentView->update();
    }

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

    Note tmpNote(m_resolution);

    m_basicDuration = tmpNote.getDuration();
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

    // Play the Note if we have one
    if (m_clickedElement && m_clickedElement->event()->
                                           isa(Rosegarden::Note::EventType))
        m_mParentView->playNote(m_clickedElement->event());

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

//------------------------------

MatrixMover::MatrixMover(MatrixView* parent)
    : MatrixTool("MatrixMover", parent),
      m_currentElement(0),
      m_currentStaff(0)
{
}

void MatrixMover::handleLeftButtonPress(Rosegarden::timeT,
                                        int,
                                        int staffNo,
                                        QMouseEvent*,
                                        Rosegarden::ViewElement* el)
{
    kdDebug(KDEBUG_AREA) << "MatrixMover::handleLeftButtonPress() : el = "
                         << el << endl;

    if (!el) return; // nothing to erase

    m_currentElement = dynamic_cast<MatrixElement*>(el);
    m_currentStaff = m_mParentView->getStaff(staffNo);
}

void MatrixMover::handleMouseMove(Rosegarden::timeT newTime,
                                  int pitch,
                                  QMouseEvent*)
{
    kdDebug(KDEBUG_AREA) << "MatrixMover::handleMouseMove() time = "
                         << newTime << endl;

    if (!m_currentElement || !m_currentStaff) return;

    int y = m_currentStaff->getLayoutYForHeight(pitch) - m_currentStaff->getElementHeight() / 2;

    m_currentElement->setLayoutY(y);
    m_currentElement->setLayoutX(newTime * m_currentStaff->getTimeScaleFactor());

    m_currentStaff->positionElement(m_currentElement);
    m_mParentView->canvas()->update();
    
}

void MatrixMover::handleMouseRelease(Rosegarden::timeT newTime,
                                     int newHeight,
                                     QMouseEvent*)
{
    kdDebug(KDEBUG_AREA) << "MatrixMover::handleMouseRelease()\n";

    if (!m_currentElement || !m_currentStaff) return;

    int y = m_currentStaff->getLayoutYForHeight(newHeight) - m_currentStaff->getElementHeight() / 2;

    kdDebug(KDEBUG_AREA) << "MatrixMover::handleMouseRelease() y = " << y << endl;

    m_currentElement->setLayoutY(y);
    m_currentElement->setLayoutX(newTime * m_currentStaff->getTimeScaleFactor());

//     m_currentStaff->positionElement(m_currentElement);
//     m_mParentView->canvas()->update();

    MatrixMoveCommand* command = new MatrixMoveCommand(m_currentStaff->getSegment(),
                                                       newTime, newHeight,
                                                       m_currentStaff,
                                                       m_currentElement->event());


    m_mParentView->addCommandToHistory(command);
    m_mParentView->canvas()->update();
}

//------------------------------
MatrixResizer::MatrixResizer(MatrixView* parent)
    : MatrixPainter("MatrixResizer", parent)
{
}

void MatrixResizer::handleLeftButtonPress(Rosegarden::timeT,
                                          int,
                                          int staffNo,
                                          QMouseEvent*,
                                          Rosegarden::ViewElement* el)
{
    kdDebug(KDEBUG_AREA) << "MatrixResizer::handleLeftButtonPress() : el = "
                         << el << endl;

    if (!el) return; // nothing to erase

    m_currentElement = dynamic_cast<MatrixElement*>(el);
    m_currentStaff = m_mParentView->getStaff(staffNo);
}

void MatrixResizer::handleMouseMove(Rosegarden::timeT newTime,
                                    int,
                                    QMouseEvent*)
{
    if (!m_currentElement || !m_currentStaff) return;

    newTime = (newTime / m_basicDuration) * m_basicDuration;

    // if (newTime == m_currentElement->getAbsoluteTime()) // what to do ? erase element ?

    timeT newDuration = newTime - m_currentElement->getAbsoluteTime();

    double width = newDuration * m_currentStaff->getTimeScaleFactor();
    m_currentElement->setWidth(int(width));

    m_mParentView->canvas()->update();
    
}

void MatrixResizer::handleMouseRelease(Rosegarden::timeT newTime,
                                       int,
                                       QMouseEvent*)
{
    if (!m_currentElement || !m_currentStaff) return;

    newTime = (newTime / m_basicDuration) * m_basicDuration;

    // if (newTime == m_currentElement->getAbsoluteTime()) // what to do ? erase element ?

    timeT newDuration = newTime - m_currentElement->getAbsoluteTime();

    double width = newDuration * m_currentStaff->getTimeScaleFactor();
    m_currentElement->setWidth(int(width));

    MatrixChangeDurationCommand* command =
        new MatrixChangeDurationCommand(m_currentStaff->getSegment(),
                                        newDuration,
                                        m_currentStaff,
                                        m_currentElement->event());


    m_mParentView->addCommandToHistory(command);

    m_mParentView->update();
}

//------------------------------

const QString MatrixPainter::ToolName   = "painter";
const QString MatrixEraser::ToolName    = "eraser";
const QString MatrixSelector::ToolName  = "selector";
const QString MatrixMover::ToolName     = "mover";
const QString MatrixResizer::ToolName   = "resizer";

