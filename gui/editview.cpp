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

#include <qlayout.h>

#include <kapp.h>
#include <kconfig.h>
#include <kaction.h>
#include <klocale.h>
#include <kstdaction.h>
#include <kstatusbar.h>

#include "editview.h"
#include "rosestrings.h"
#include "rosegardencanvasview.h"
#include "edittool.h"
#include "qcanvasgroupableitem.h"
#include "basiccommand.h"
#include "rosegardenguidoc.h"
#include "multiviewcommandhistory.h"
#include "ktmpstatusmsg.h"
#include "barbuttons.h"

#include "rosedebug.h"

//----------------------------------------------------------------------
const unsigned int EditView::ID_STATUS_MSG = 1;

EditView::EditView(RosegardenGUIDoc *doc,
                   std::vector<Rosegarden::Segment *> segments,
                   bool hasTwoCols,
                   QWidget *parent, const char *name) :
    KMainWindow(parent, name),
    m_viewNumber(-1),
    m_viewLocalPropertyPrefix(makeViewLocalPropertyPrefix()),
    m_config(kapp->config()),
    m_document(doc),
    m_segments(segments),
    m_tool(0),
    m_toolBox(0),
    m_activeItem(0),
    m_canvasView(0),
    m_centralFrame(new QFrame(this)),
    m_horizontalScrollBar(new QScrollBar(Horizontal, m_centralFrame)),
    m_grid(new QGridLayout(m_centralFrame, 5, hasTwoCols ? 2 : 1)),
    m_rulerBox(new QVBoxLayout), // added to grid later on
    m_topBarButtons(0),
    m_bottomBarButtons(0),
    m_mainCol(hasTwoCols ? 1 : 0),
    m_compositionRefreshStatusId(doc->getComposition().getNewRefreshStatusId())
{
    initSegmentRefreshStatusIds();

    setCentralWidget(m_centralFrame);

    m_centralFrame->setMargin(0);

    // add undo and redo to edit menu and toolbar
    getCommandHistory()->attachView(actionCollection());
    
    QObject::connect
        (getCommandHistory(), SIGNAL(commandExecuted()),
         this,                  SLOT(update()));

    m_grid->addWidget(m_horizontalScrollBar, 4, m_mainCol);
    m_grid->addLayout(m_rulerBox, 0, m_mainCol);

    // create accelerators
    //
    m_accelerators = new QAccel(this);
}

EditView::~EditView()
{
    getCommandHistory()->detachView(actionCollection());
    m_viewNumberPool.erase(m_viewNumber);
}

void EditView::saveOptions()
{
    m_config->writeEntry("Geometry", size());

    // FIXME: it seems this one doesn't exist in KDE3.1
    if (getToggleAction("options_show_toolbar"))
        m_config->writeEntry("Show Toolbar",
                             getToggleAction("options_show_toolbar")->isChecked());

    m_config->writeEntry("Show Statusbar",
                         getToggleAction("options_show_statusbar")->isChecked());

    toolBar()->saveSettings(m_config, m_config->group() + " Toolbar");
}

void EditView::readOptions()
{
    QSize size(m_config->readSizeEntry("Geometry"));

    if (!size.isEmpty()) {
        resize(size);
    }

    // Read toolbars visibility
    bool opt;

    opt = m_config->readBoolEntry("Show Toolbar", true);

    // FIXME: it seems this one doesn't exist in KDE3.1
    if (getToggleAction("options_show_toolbar"))
        getToggleAction("options_show_toolbar")->setChecked(opt);

    if (opt)
        toolBar()->show();
    else
        toolBar()->hide();

    opt = m_config->readBoolEntry("Show Statusbar", true);
    getToggleAction("options_show_statusbar")->setChecked(opt);
    if (opt)
        statusBar()->show();
    else
        statusBar()->hide();

    toolBar()->applySettings(m_config, m_config->group() + " Toolbar");
}


std::set<int>
EditView::m_viewNumberPool;

std::string
EditView::makeViewLocalPropertyPrefix()
{
    static char buffer[100];
    int i = 0;
    while (m_viewNumberPool.find(i) != m_viewNumberPool.end()) ++i;
    m_viewNumber = i;
    m_viewNumberPool.insert(i);
    sprintf(buffer, "View%d::", i);
    return buffer;
}

void EditView::paintEvent(QPaintEvent* e)
{
    if (isCompositionModified()) {
      
        // Check if one of the segments we display has been removed
        // from the composition.
        //
	// For the moment we'll have to close the view if any of the
	// segments we handle has been deleted.

	for (unsigned int i = 0; i < m_segments.size(); ++i) {

	    if (!m_segments[i]->getComposition()) {
		// oops, I think we've been deleted
		close();
		return;
	    }
	}
        setCompositionModified(false);
    }


    bool needUpdate = false;
    
    // Scan all segments and check if they've been modified
    //
    for (unsigned int i = 0; i < m_segments.size(); ++i) {

        Rosegarden::Segment* segment = m_segments[i];
        unsigned int refreshStatusId = m_segmentsRefreshStatusIds[i];
        Rosegarden::SegmentRefreshStatus &refreshStatus =
	    segment->getRefreshStatus(refreshStatusId);
        
        if (refreshStatus.needsRefresh() && isCompositionModified()) {

            // if composition is also modified, relayout everything
            refreshSegment(0);
            break;
            
        } else if (refreshStatus.needsRefresh()) {

            Rosegarden::timeT startTime = refreshStatus.from(),
                endTime = refreshStatus.to();

            refreshSegment(segment, startTime, endTime);
            refreshStatus.setNeedsRefresh(false);
            needUpdate = true;
        }
    }

    KMainWindow::paintEvent(e);

    if (needUpdate)  {
        kdDebug(KDEBUG_AREA) << "EditView::paintEvent() - calling updateView\n";
        updateView();
        getCanvasView()->slotUpdate();
    } else {

        getCanvasView()->slotUpdate();
    }

}

void EditView::setTopBarButtons(QWidget* w)
{
    delete m_topBarButtons;
    m_topBarButtons = w;
    m_grid->addWidget(w, 1, m_mainCol);

    connect(m_horizontalScrollBar, SIGNAL(valueChanged(int)),
            m_topBarButtons, SLOT(slotScrollHoriz(int)));
    connect(m_horizontalScrollBar, SIGNAL(sliderMoved(int)),
            m_topBarButtons, SLOT(slotScrollHoriz(int)));
}

void EditView::setBottomBarButtons(QWidget* w)
{
    delete m_bottomBarButtons;
    m_bottomBarButtons = w;
    m_grid->addWidget(w, 3, m_mainCol);

    connect(m_horizontalScrollBar, SIGNAL(valueChanged(int)),
            m_bottomBarButtons, SLOT(slotScrollHoriz(int)));
    connect(m_horizontalScrollBar, SIGNAL(sliderMoved(int)),
            m_bottomBarButtons, SLOT(slotScrollHoriz(int)));
}

void EditView::addRuler(QWidget* w)
{
    m_rulerBox->addWidget(w);

    connect(m_horizontalScrollBar, SIGNAL(valueChanged(int)),
            w, SLOT(slotScrollHoriz(int)));
    connect(m_horizontalScrollBar, SIGNAL(sliderMoved(int)),
            w, SLOT(slotScrollHoriz(int)));
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
        windowHeight        = height();

    QSize newSize;

    newSize.setWidth(((requestedWidth / windowWidth) + 1) * windowWidth);
    newSize.setHeight(((requestedHeight / windowHeight) + 1) * windowHeight);

    kdDebug(KDEBUG_AREA) << "EditView::readjustViewSize: requested ("
			 << requestedSize.width() << ", " << requestedSize.height() 
			 << "), getting (" << newSize.width() <<", "
			 << newSize.height() << ")" << endl;

    setViewSize(newSize);

    getCanvasView()->slotUpdate();
}

// This scrolling model pages the EditView's canvas across the screen
//
void EditView::slotScrollHoriz(int hpos)
{
    QScrollView* scrollView = getCanvasView();
    QScrollBar* hbar = m_horizontalScrollBar;

    if (hpos == 0) {
	
	// returning to zero
        hbar->setValue(0);

    } else if (hpos > (scrollView->contentsX() +
		       scrollView->visibleWidth() * 1.6) ||
	       hpos < (scrollView->contentsX() -
		       scrollView->visibleWidth() * 0.7)) {
	
	// miles off one side or the other
	hbar->setValue(hpos - int(scrollView->visibleWidth() * 0.4));

    } else if (hpos > (scrollView->contentsX() + 
		       scrollView->visibleWidth() * 0.9)) {

	// moving off the right hand side of the view   
	hbar->setValue(hbar->value() + int(scrollView->visibleWidth() * 0.8));

    } else if (hpos < (scrollView->contentsX() +
		       scrollView->visibleWidth() * 0.1)) {

	// moving off the left
	hbar->setValue(hbar->value() - int(scrollView->visibleWidth() * 0.8));
    }
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

void EditView::setCanvasView(RosegardenCanvasView *canvasView)
{
    delete m_canvasView;
    m_canvasView = canvasView;
    m_grid->addWidget(m_canvasView, 2, m_mainCol);
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

RosegardenCanvasView* EditView::getCanvasView()
{
    return m_canvasView;
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
        updateView();

    }

}

void EditView::initSegmentRefreshStatusIds()
{
    for(unsigned int i = 0; i < m_segments.size(); ++i) {

        unsigned int rid = m_segments[i]->getNewRefreshStatusId();
        m_segmentsRefreshStatusIds.push_back(rid);
    }
}

bool EditView::isCompositionModified()
{
    return m_document->getComposition().getRefreshStatus
	(m_compositionRefreshStatusId).needsRefresh();
}

void EditView::setCompositionModified(bool c)
{
    m_document->getComposition().getRefreshStatus
	(m_compositionRefreshStatusId).setNeedsRefresh(c);
}

KToggleAction* EditView::getToggleAction(const QString& actionName)
{
    return dynamic_cast<KToggleAction*>(actionCollection()->action(actionName));
}
