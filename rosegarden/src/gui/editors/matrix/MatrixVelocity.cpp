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

#include <klocale.h>
#include <kstddirs.h>
#include "base/Event.h"
#include "base/Segment.h"
#include "base/Selection.h"
#include "base/SnapGrid.h"
#include "base/ViewElement.h"
#include "commands/edit/ChangeVelocityCommand.h"
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
#include "misc/Debug.h"


namespace Rosegarden
{

MatrixVelocity::MatrixVelocity(MatrixView* parent)
        : MatrixTool("MatrixVelocity", parent),
	m_mouseStartY(0),
	m_velocityDelta(0),
	m_screenPixelsScale(100),
	m_velocityScale(0),
        m_currentElement(0),
        m_currentStaff(0)
{
    QString pixmapDir = KGlobal::dirs()->findResource("appdata", "pixmaps/");
    QCanvasPixmap pixmap(pixmapDir + "/toolbar/select.xpm");
    QIconSet icon = QIconSet(pixmap);

    new KAction(i18n("Switch to Select Tool"), icon, Key_F2, this,
                SLOT(slotSelectSelected()), actionCollection(),
                "select");

    new KAction(i18n("Switch to Draw Tool"), "pencil", Key_F3, this,
                SLOT(slotDrawSelected()), actionCollection(),
                "draw");

    new KAction(i18n("Switch to Erase Tool"), "eraser", Key_F4, this,
                SLOT(slotEraseSelected()), actionCollection(),
                "erase");

    new KAction(i18n("Switch to Move Tool"), "move", Key_F5, this,
                SLOT(slotMoveSelected()), actionCollection(),
                "move");

    new KAction(i18n("Switch to Resize Tool"), "resize", Key_F6, this,
                SLOT(slotResizeSelected()), actionCollection(),
                "resize");

    createMenu("matrixvelocity.rc");
}

void MatrixVelocity::handleEventRemoved(Event *event)
{
    if (m_currentElement && m_currentElement->event() == event) {
        m_currentElement = 0;
    }
}

void MatrixVelocity::handleLeftButtonPress(timeT,
        int,
        int staffNo,
        QMouseEvent* e,
        ViewElement* el)
{
    MATRIX_DEBUG << "MatrixVelocity::handleLeftButtonPress() : el = "
    << el << endl;

    if (!el)
        return ; // nothing to erase
    
    m_currentElement = dynamic_cast<MatrixElement*>(el);
    m_currentStaff = m_mParentView->getStaff(staffNo);
    
    // Get mouse pointer
    m_mouseStartY=e->pos().y();
    
    if (m_currentElement) {
        // Add this element and allow velocity change
        EventSelection* selection = m_mParentView->getCurrentSelection();

        if (selection) {
            EventSelection *newSelection;

            if ((e->state() & Qt::ShiftButton) || selection->contains(m_currentElement->event()))
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



int MatrixVelocity::handleMouseMove(timeT newTime,
                                   int,
                                   QMouseEvent *e)
{
    setBasicContextHelp();
    
    if (!m_currentElement || !m_currentStaff)
        return RosegardenCanvasView::NoFollow;

    
    if (e && m_mouseStartY!=0 ) {
	
	// Check if left mousebutton is down
	if(!(e->state() & Qt::LeftButton)) {
	    m_mouseStartY=0;
	    return RosegardenCanvasView::NoFollow;
	}
        
	// Calculate velocity scale factor
	if((m_mouseStartY-(e->pos()).y())>m_screenPixelsScale)
	    m_velocityScale=1.0;
	else if((m_mouseStartY-(e->pos()).y())<-m_screenPixelsScale)
	    m_velocityScale=-1.0;
	else
	    m_velocityScale=(double)(m_mouseStartY-(e->pos()).y())/(double)(m_screenPixelsScale*2);
	
	m_velocityDelta=128*m_velocityScale;
	
	/*m_velocityDelta=(m_mouseStartY-(e->pos()).y());
     
        if (m_velocityDelta > m_screenPixelsScale) 
		m_velocityDelta=m_screenPixelsScale;
	else if (m_velocityDelta < -m_screenPixelsScale) 
		m_velocityDelta=-m_screenPixelsScale;
	
	m_velocityScale=1.0+(double)m_velocityDelta/(double)m_screenPixelsScale;
	
	m_velocityDelta*=2.0;
	*/
	
	// Preview velocity delta in contexthelp
	setContextHelp(i18n("Velocity change: %1").arg(m_velocityDelta));
	
	// Preview calculated velocity info on element
	/** Might be something for the feature
	EventSelection* selection = m_mParentView->getCurrentSelection();
	EventSelection::eventcontainer::iterator it = selection->getSegmentEvents().begin();
	MatrixElement *element = 0;
	for (; it != selection->getSegmentEvents().end(); it++) {
	    element = m_currentStaff->getElement(*it);
	    if (element) {
		// Somehow show the calculated velocity for each selected element
		// char label[16];
		// sprintf(label,"%d",(*it->getVelocity())*m_velocityScale);
		// element->label(label) /// DOES NOT EXISTS
	    }
	}
	*/
    }
    
    m_mParentView->canvas()->update();
    return RosegardenCanvasView::NoFollow;
}

void MatrixVelocity::handleMouseRelease(timeT newTime,
                                       int,
                                       QMouseEvent *e)
{
    
    if (!m_currentElement || !m_currentStaff)
        return ;

    EventSelection *selection = new EventSelection(*m_mParentView->getCurrentSelection());
   
    if (selection->getAddedEvents() == 0 || m_velocityDelta==0)
        return ;
    else {
        QString commandLabel = i18n("Change Velocity");

        if (selection->getAddedEvents() > 1)
            commandLabel = i18n("Change Velocities");

        KMacroCommand *macro = new KMacroCommand(commandLabel);
	macro->addCommand(new ChangeVelocityCommand(m_velocityDelta,*selection,false));
	
	// Clear selection??? nooo
        //m_mParentView->setCurrentSelection(0, false, false);
	
        m_mParentView->addCommandToHistory(macro);
  
    }

    // Reset the start of mousemove
    m_velocityDelta=m_mouseStartY=0;
    

    m_mParentView->update();
    m_currentElement = 0;
    setBasicContextHelp();
    delete selection;
}

void MatrixVelocity::ready()
{
    /*connect(m_parentView->getCanvasView(), SIGNAL(contentsMoving (int, int)),
            this, SLOT(slotMatrixScrolled(int, int)));
    */
    setBasicContextHelp();
    m_mParentView->setCanvasCursor(Qt::sizeVerCursor);
}

void MatrixVelocity::stow()
{
    /*disconnect(m_parentView->getCanvasView(), SIGNAL(contentsMoving (int, int)),
               this, SLOT(slotMatrixScrolled(int, int)));*/
}

void MatrixVelocity::slotMatrixScrolled(int newX, int newY)
{
    /*QPoint newP1(newX, newY), oldP1(m_parentView->getCanvasView()->contentsX(),
                                    m_parentView->getCanvasView()->contentsY());

    QPoint p(newX, newY);

    if (newP1.x() > oldP1.x()) {
        p.setX(newX + m_parentView->getCanvasView()->visibleWidth());
    }

    p = m_mParentView->inverseMapPoint(p);
    int newTime = getSnapGrid().snapX(p.x());
    handleMouseMove(newTime, 0, 0);*/
}

void MatrixVelocity::setBasicContextHelp()
{
    EventSelection *selection = m_mParentView->getCurrentSelection();
    if (selection && selection->getAddedEvents() > 1) {
        setContextHelp(i18n("Click and drag to scale velocity of selected notes"));
    } else {
        setContextHelp(i18n("Click and drag to scale velocity of note"));
    }
}

const QString MatrixVelocity::ToolName   = "velocity";

}
#include "MatrixVelocity.moc"
