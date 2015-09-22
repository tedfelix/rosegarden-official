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


#include "MatrixVelocity.h"

#include "base/BaseProperties.h"
#include "base/Event.h"
#include "base/Segment.h"
#include "base/Selection.h"
#include "base/SnapGrid.h"
#include "base/ViewElement.h"
#include "document/CommandHistory.h"
#include "commands/edit/ChangeVelocityCommand.h"
#include "MatrixElement.h"
#include "MatrixViewSegment.h"
#include "MatrixMouseEvent.h"
#include "MatrixTool.h"
#include "MatrixScene.h"
#include "MatrixWidget.h"
#include "misc/Debug.h"


namespace Rosegarden
{

MatrixVelocity::MatrixVelocity(MatrixWidget *widget) :
    MatrixTool("matrixvelocity.rc", "MatrixVelocity", widget),
    m_mouseStartY(0),
    m_velocityDelta(0),
    m_screenPixelsScale(100),
    m_velocityScale(0),
    m_currentElement(0),
    m_currentViewSegment(0)
{
    createAction("select", SLOT(slotSelectSelected()));
    createAction("draw", SLOT(slotDrawSelected()));
    createAction("erase", SLOT(slotEraseSelected()));
    createAction("move", SLOT(slotMoveSelected()));
    createAction("resize", SLOT(slotResizeSelected()));

    createMenu();
}

void
MatrixVelocity::handleEventRemoved(Event *event)
{
    if (m_currentElement && m_currentElement->event() == event) {
        m_currentElement = 0;
    }
}

void
MatrixVelocity::handleLeftButtonPress(const MatrixMouseEvent *e)
{
    if (!e->element) return;

    m_currentViewSegment = e->viewSegment;
    m_currentElement = e->element;

    // Get mouse pointer
    m_mouseStartY = e->sceneY;

    // Add this element and allow velocity change
    EventSelection *selection = m_scene->getSelection();

    if (selection) {
        EventSelection *newSelection;

        if ((e->modifiers & Qt::ShiftModifier) ||
            selection->contains(m_currentElement->event())) {
            newSelection = new EventSelection(*selection);
        } else {
            newSelection = new EventSelection(m_currentViewSegment->getSegment());
        }

        newSelection->addEvent(m_currentElement->event());
        m_scene->setSelection(newSelection, true);

    } else {
        m_scene->setSingleSelectedEvent(m_currentViewSegment,
                                        m_currentElement,
                                        true);
    }
}

MatrixVelocity::FollowMode
MatrixVelocity::handleMouseMove(const MatrixMouseEvent *e)
{
    setBasicContextHelp();

    if (!e || !m_currentElement || !m_currentViewSegment) {
        m_mouseStartY = 0;
        return NoFollow;
    }

    // Check if left mousebutton is down
    if (!(e->buttons & Qt::LeftButton)) {
        m_mouseStartY = 0;
        return NoFollow;
    }

    // Calculate velocity scale factor
    if ((m_mouseStartY - e->sceneY) > m_screenPixelsScale) {
        m_velocityScale = 1.0;
    } else if ((m_mouseStartY - e->sceneY) < -m_screenPixelsScale) {
        m_velocityScale = -1.0;
    } else {
        m_velocityScale =
            (double)(m_mouseStartY - e->sceneY) /
//            (double)(m_screenPixelsScale * 2);
            (double)(m_screenPixelsScale);
    }

    m_velocityDelta = 128 * m_velocityScale;

    /*m_velocityDelta=(m_mouseStartY-(e->pos()).y());

        if (m_velocityDelta > m_screenPixelsScale)
        m_velocityDelta=m_screenPixelsScale;
    else if (m_velocityDelta < -m_screenPixelsScale)
        m_velocityDelta=-m_screenPixelsScale;

    m_velocityScale=1.0+(double)m_velocityDelta/(double)m_screenPixelsScale;

    m_velocityDelta*=2.0;
    */

    // Preview velocity delta in contexthelp
    setContextHelp(tr("Velocity change: %1").arg(m_velocityDelta));

	// Preview calculated velocity info on element
	// Dupe from MatrixMover
    EventSelection* selection = m_scene->getSelection();

//    MatrixElement *element = 0;
//    int maxY = m_currentViewSegment->getCanvasYForHeight(0);

    for (EventSelection::eventcontainer::iterator it =
             selection->getSegmentEvents().begin();
         it != selection->getSegmentEvents().end(); ++it) {

//        MatrixElement *element = m_currentViewSegment->getElement(*it);
//        if (!element) continue;

        MatrixElement *element = 0;
        ViewElementList::iterator vi = m_currentViewSegment->findEvent(*it);
        if (vi != m_currentViewSegment->getViewElementList()->end()) {
            element = static_cast<MatrixElement *>(*vi);
        }
        if (!element) continue;

//        timeT diffTime = element->getViewAbsoluteTime() -
//            m_currentElement->getViewAbsoluteTime();

//        int epitch = 0;
//        if (element->event()->has(PITCH)) {
//            epitch = element->event()->get<Int>(PITCH);
//        }

        int velocity = 64;
        if (element->event()->has(BaseProperties::VELOCITY)) {
            velocity = element->event()->get<Int>(BaseProperties::VELOCITY);
        }

//        element->reconfigure(newTime + diffTime,
//                             element->getViewDuration(),
//                             epitch + diffPitch);
        element->reconfigure(velocity+m_velocityDelta);
        element->setSelected(true);
    }

    emit hoveredOverNoteChanged();

	/** Might be something for the feature
	EventSelection* selection = m_mParentView->getCurrentSelection();
	EventSelection::eventcontainer::iterator it = selection->getSegmentEvents().begin();
	MatrixElement *element = 0;
	for (; it != selection->getSegmentEvents().end(); it++) {
	    element = m_currentViewSegment->getElement(*it);
	    if (element) {
		// Somehow show the calculated velocity for each selected element
		// char label[16];
		// sprintf(label,"%d",(*it->getVelocity())*m_velocityScale);
		// element->label(label) /// DOES NOT EXISTS
	    }
	}
	*/

    return NoFollow;
}

void
MatrixVelocity::handleMouseRelease(const MatrixMouseEvent *e)
{
    if (!e || !m_currentElement || !m_currentViewSegment) {
        m_mouseStartY = 0;
        return;
    }

    EventSelection *selection = m_scene->getSelection();
    if (selection) selection = new EventSelection(*selection);
    else selection = new EventSelection(m_currentViewSegment->getSegment());

    if (selection->getAddedEvents() == 0 || m_velocityDelta == 0) {
        delete selection;
        return;
    } else {
        QString commandLabel = tr("Change Velocity");

        if (selection->getAddedEvents() > 1) {
            commandLabel = tr("Change Velocities");
        }

        m_scene->setSelection(0, false);

        CommandHistory::getInstance()->addCommand
            (new ChangeVelocityCommand(m_velocityDelta, *selection, false));

        m_scene->setSelection(selection, false);
    }

    // Reset the start of mousemove
    m_velocityDelta = m_mouseStartY = 0;
    m_currentElement = 0;
    setBasicContextHelp();
}

void
MatrixVelocity::ready()
{
    setBasicContextHelp();
    m_widget->setCanvasCursor(Qt::SizeVerCursor);
}

void
MatrixVelocity::stow()
{
}

void
MatrixVelocity::setBasicContextHelp()
{
    EventSelection *selection = m_scene->getSelection();
    if (selection && selection->getAddedEvents() > 1) {
        setContextHelp(tr("Click and drag to scale velocity of selected notes"));
    } else {
        setContextHelp(tr("Click and drag to scale velocity of note"));
    }
}

const QString MatrixVelocity::ToolName = "velocity";

}
#include "MatrixVelocity.moc"
