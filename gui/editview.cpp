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

EditView::EditView(RosegardenGUIDoc *doc,
                   std::vector<Rosegarden::Segment *> segments,
                   unsigned int cols,
                   QWidget *parent, const char *name) :
    EditViewBase(doc, segments, cols, parent, name),
    m_activeItem(0),
    m_canvasView(0),
    m_horizontalScrollBar(new QScrollBar(Horizontal, m_centralFrame)),
    m_rulerBox(new QVBoxLayout), // added to grid later on
    m_topBarButtons(0),
    m_bottomBarButtons(0)
{
    m_grid->addWidget(m_horizontalScrollBar, 4, m_mainCol);
    m_grid->addLayout(m_rulerBox, 0, m_mainCol);
}

EditView::~EditView()
{
}

void EditView::paintEvent(QPaintEvent* e)
{
    EditViewBase::paintEvent(e);
    
    if (m_needUpdate)  {
        kdDebug(KDEBUG_AREA) << "EditView::paintEvent() - calling updateView\n";
        updateView();
        getCanvasView()->slotUpdate();
    } else {

        getCanvasView()->slotUpdate();
    }

    m_needUpdate = false;
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
	hbar->setValue(hbar->value() + int(scrollView->visibleWidth() * 0.6));

    } else if (hpos < (scrollView->contentsX() +
		       scrollView->visibleWidth() * 0.1)) {

	// moving off the left
	hbar->setValue(hbar->value() - int(scrollView->visibleWidth() * 0.6));
    }
}

void EditView::slotScrollHorizSmallSteps(int hpos)
{
    QScrollView* scrollView = getCanvasView();
    QScrollBar* hbar = m_horizontalScrollBar;

    int diff = 0;

    if (hpos == 0) {
	
	// returning to zero
        hbar->setValue(0);

    } else if ((diff = int(hpos - (scrollView->contentsX() + 
				   scrollView->visibleWidth() * 0.9))) > 0) {

	// moving off the right hand side of the view   
	hbar->setValue(hbar->value() + diff);

    } else if ((diff = int(hpos - (scrollView->contentsX() +
				   scrollView->visibleWidth() * 0.1))) < 0) {

	// moving off the left
	hbar->setValue(hbar->value() + diff);

    }
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
