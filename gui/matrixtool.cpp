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

    string eventType = m_event->getType();

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

void MatrixPainter::setResolution(Rosegarden::Note::Type note)
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

const QString MatrixPainter::ToolName = "painter";
const QString MatrixEraser::ToolName  = "eraser";

