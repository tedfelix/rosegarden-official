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

#include <kapp.h>
#include <kconfig.h>
#include <kaction.h>
#include <klocale.h>
#include <kstdaction.h>

#include <qvbox.h>
#include <qhbox.h>

#include "editview.h"
#include "edittool.h"
#include "qcanvasgroupableitem.h"
#include "basiccommand.h"
#include "rosegardenguidoc.h"
#include "multiviewcommandhistory.h"
#include "rosedebug.h"
#include "ktmpstatusmsg.h"
#include "staffruler.h"
#include "barbuttons.h"

//----------------------------------------------------------------------
const unsigned int EditView::ID_STATUS_MSG = 1;

EditView::EditView(RosegardenGUIDoc *doc,
                   std::vector<Rosegarden::Segment *>,
                   QWidget *parent)
    : KMainWindow(parent),
      m_config(kapp->config()),
      m_document(doc),
      m_tool(0),
      m_toolBox(0),
      m_activeItem(0),
      m_canvasView(0)
{
    m_topBox = new QVBox(this);
    setCentralWidget(m_topBox);

    QHBox *topSplit = new QHBox(m_topBox);
    
    m_topBox->setSpacing(0);
    m_topBox->setMargin(0);

    topSplit->setMinimumHeight(25);//!!!
    topSplit->setMaximumHeight(25);//!!!
    topSplit->setSpacing(0);
    topSplit->setMargin(0);
//    topSplit->setFrameStyle(Plain);

    QLabel *label = new QLabel(topSplit);
    label->setMinimumWidth(20);//!!!
    label->setMaximumWidth(20);//!!!
    label->setMinimumHeight(25);//!!!
    label->setMaximumHeight(25);//!!!

    m_barButtonsView = new QScrollView(topSplit);
    m_barButtonsView->setHScrollBarMode(QScrollView::AlwaysOff);
    m_barButtonsView->setVScrollBarMode(QScrollView::AlwaysOff);

//    BarButtons *barButtons = new BarButtons
//	(doc, m_rulerScale, 25, m_barButtonsView);

//    m_barButtonsView->setFrameStyle(Plain);
//    barButtons->setFrameStyle(Plain);

    m_barButtonsView->setMinimumHeight(25);//!!!
    m_barButtonsView->setMaximumHeight(25);//!!!

    // add undo and redo to edit menu and toolbar
    getCommandHistory()->attachView(actionCollection());
    
    QObject::connect
	(getCommandHistory(), SIGNAL(commandExecuted(KCommand *)),
	 this,		      SLOT(slotCommandExecuted(KCommand *)));
}

EditView::~EditView()
{
    getCommandHistory()->detachView(actionCollection());
}

void EditView::setCanvasView(QCanvasView *canvasView)
{
    m_canvasView = canvasView;

    if (m_canvasView) {
	
//	connect(m_trackEditorScrollView, SIGNAL(contentsMoving(int, int)),
//		trackButtonsView,        SLOT(setContentsPos(int, int)));

	connect(m_canvasView,    SIGNAL(contentsMoving(int, int)),
		m_barButtonsView,  SLOT(setContentsPos(int, int)));
    }
}


void EditView::readjustViewSize(QSize requestedSize, bool exact)
{
    if (exact) {
        setViewSize(requestedSize);
        return;
    }

    int requestedWidth  = requestedSize.width(),
        requestedHeight = requestedSize.height(),
	windowWidth     = width(),
	windowHeight	= height();

    QSize newSize;

    newSize.setWidth(((requestedWidth / windowWidth) + 1) * windowWidth);
    newSize.setHeight(((requestedHeight / windowHeight) + 1) * windowHeight);

    setViewSize(newSize);
}

MultiViewCommandHistory *EditView::getCommandHistory()
{
    return getDocument()->getCommandHistory();
}

void EditView::addCommandToHistory(KCommand *command)
{
    getCommandHistory()->addCommand(command);
}

void EditView::setTool(EditTool* tool)
{
    if (m_tool)
        m_tool->stow();

    m_tool = tool;

    if (m_tool)
        m_tool->ready();

}

//////////////////////////////////////////////////////////////////////
//                    Slots
//////////////////////////////////////////////////////////////////////

void EditView::closeWindow()
{
    close();
}

//
// Toolbar and statusbar toggling
//
void EditView::slotToggleToolBar()
{
    KTmpStatusMsg msg(i18n("Toggle the toolbar..."), statusBar());

    if (toolBar()->isVisible())
        toolBar()->hide();
    else
        toolBar()->show();
}

void EditView::slotToggleStatusBar()
{
    KTmpStatusMsg msg(i18n("Toggle the statusbar..."), statusBar());

    if (statusBar()->isVisible())
        statusBar()->hide();
    else
        statusBar()->show();
}

void EditView::slotCommandExecuted(KCommand *command)
{
    BasicCommand *basicCommand = 0;
    if ((basicCommand = dynamic_cast<BasicCommand *>(command)) != 0) {
	refreshSegment
	    (&basicCommand->getSegment(), basicCommand->getBeginTime(),
					  basicCommand->getEndTime());
    } else {
	//!!! deal with other command superclasses from segmentcommands.h
	kdDebug(KDEBUG_AREA)
	    << "Warning: EditView::slotCommandExecuted:\n"
	    << "KCommand is not a BasicCommand, don't know how to refresh"
	    << endl;
    }
}

//
// Status messages
//
void EditView::slotStatusMsg(const QString &text)
{
    ///////////////////////////////////////////////////////////////////
    // change status message permanently
    statusBar()->clear();
    statusBar()->changeItem(text, ID_STATUS_MSG);
}


void EditView::slotStatusHelpMsg(const QString &text)
{
    ///////////////////////////////////////////////////////////////////
    // change status message of whole statusbar temporary (text, msec)
    statusBar()->message(text, 2000);
}

void EditView::activeItemPressed(QMouseEvent* e,
                                 QCanvasItem* item)
{
    if (!item) return;

    // Check if it's a groupable item, if so get its group
    //
    QCanvasGroupableItem *gitem = dynamic_cast<QCanvasGroupableItem*>(item);
    if (gitem) item = gitem->group();
        
    // Check if it's an active item
    //
    ActiveItem *activeItem = dynamic_cast<ActiveItem*>(item);
        
    if (activeItem) {

        setActiveItem(activeItem);
        activeItem->handleMousePress(e);
        update();

    }

}
