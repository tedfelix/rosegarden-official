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

#include <qlayout.h>

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
      m_canvasView(0),
      m_centralFrame(new QFrame(this)),
      m_horizontalScrollBar(new QScrollBar(Horizontal, m_centralFrame)),
      m_grid(new QGridLayout(m_centralFrame, 5, 0)), // 5 rows, 0 cols
      m_topBarButtons(0),
      m_textRuler(0),
      m_bottomBarButtons(0)
{
    setCentralWidget(m_centralFrame);

    m_centralFrame->setMargin(0);

    // add undo and redo to edit menu and toolbar
    getCommandHistory()->attachView(actionCollection());
    
    QObject::connect
	(getCommandHistory(), SIGNAL(commandExecuted(KCommand *)),
	 this,		      SLOT(slotCommandExecuted(KCommand *)));

    m_grid->addWidget(m_horizontalScrollBar, 4, 0);
}

EditView::~EditView()
{
    getCommandHistory()->detachView(actionCollection());
}

void EditView::setCanvasView(QCanvasView *canvasView)
{
    delete m_canvasView;
    m_canvasView = canvasView;
    m_grid->addWidget(m_canvasView, 1, 0);
    m_canvasView->setHScrollBarMode(QScrollView::AlwaysOff);

    m_horizontalScrollBar->setRange(m_canvasView->horizontalScrollBar()->minValue(),
                                    m_canvasView->horizontalScrollBar()->maxValue());

    m_horizontalScrollBar->setSteps(m_canvasView->horizontalScrollBar()->lineStep(),
                                    m_canvasView->horizontalScrollBar()->pageStep());

    connect(m_horizontalScrollBar, SIGNAL(valueChanged(int)),
            m_canvasView->horizontalScrollBar(), SIGNAL(valueChanged(int)));
    connect(m_horizontalScrollBar, SIGNAL(sliderMoved(int)),
            m_canvasView->horizontalScrollBar(), SIGNAL(sliderMoved(int)));

}

void EditView::setTopBarButtons(QWidget* w)
{
    delete m_topBarButtons;
    m_topBarButtons = w;
    m_grid->addWidget(w, 0, 0);

    connect(m_horizontalScrollBar, SIGNAL(valueChanged(int)),
            m_topBarButtons, SLOT(slotScrollHoriz(int)));
    connect(m_horizontalScrollBar, SIGNAL(sliderMoved(int)),
            m_topBarButtons, SLOT(slotScrollHoriz(int)));
}

void EditView::setBottomBarButtons(QWidget* w)
{
    delete m_bottomBarButtons;
    m_bottomBarButtons = w;
    m_grid->addWidget(w, 3, 0);

    connect(m_horizontalScrollBar, SIGNAL(valueChanged(int)),
            m_bottomBarButtons, SLOT(slotScrollHoriz(int)));
    connect(m_horizontalScrollBar, SIGNAL(sliderMoved(int)),
            m_bottomBarButtons, SLOT(slotScrollHoriz(int)));
}

void EditView::setTextRuler(QWidget* w)
{
    delete m_textRuler;
    m_textRuler = w;
    m_grid->addWidget(w, 2, 0);

    connect(m_horizontalScrollBar, SIGNAL(valueChanged(int)),
            m_textRuler, SLOT(slotScrollHoriz(int)));
    connect(m_horizontalScrollBar, SIGNAL(sliderMoved(int)),
            m_textRuler, SLOT(slotScrollHoriz(int)));
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

void EditView::slotCloseWindow()
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
    // might be better done with a visitor pattern or some such

    if (dynamic_cast<PartialSegmentCommand *>(command) != 0) {

	BasicCommand *basicCommand = 0;

	if ((basicCommand = dynamic_cast<BasicCommand *>(command)) != 0) {
	    refreshSegment(&basicCommand->getSegment(),
			   basicCommand->getBeginTime(),
			   basicCommand->getRelayoutEndTime());
	} else {
	    // partial segment command, but not a basic command
	    Rosegarden::Segment *segment = &basicCommand->getSegment();
	    refreshSegment(segment,
			   segment->getStartTime(),
			   segment->getEndTime());
	}

	return;
    }

    SegmentCommand *segmentCommand = dynamic_cast<SegmentCommand *>(command);
    if (segmentCommand) {
	
	SegmentCommand::SegmentSet segments;
	segmentCommand->getSegments(segments);

	for (SegmentCommand::SegmentSet::iterator i = segments.begin();
	     i != segments.end(); ++i) {

	    //!!!
	    // segment-commands -> redraw ruler if only segment, redraw all
	    // otherwise I fear -- but what if the segment's been deleted?

	}

	return;
    }

    TimeAndTempoChangeCommand *timeCommand =
	dynamic_cast<TimeAndTempoChangeCommand *>(command);
    if (timeCommand) {

	//!!!
	// time-and-tempo-commands -> redraw all? hmm

	return;
    }

    kdDebug(KDEBUG_AREA)
	<< "Warning: EditView::slotCommandExecuted:\n"
	<< "Unknown sort of KCommand, don't know how to refresh"
	<< endl;
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

void EditView::slotActiveItemPressed(QMouseEvent* e,
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
