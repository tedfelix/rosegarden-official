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
#include <cmath>

#include <qiconset.h>
#include <qvbox.h>

#include <kapp.h>
#include <kconfig.h>
#include <kaction.h>
#include <kstddirs.h>
#include <klocale.h>
#include <kstdaction.h>
#include <kmessagebox.h>

#include "Composition.h"
#include "Event.h"

#include "BaseProperties.h"
#include "matrixview.h"
#include "matrixstaff.h"
#include "matrixhlayout.h"
#include "matrixvlayout.h"
#include "matrixtool.h"
#include "rosegardenguidoc.h"
#include "ktmpstatusmsg.h"
#include "barbuttons.h"
#include "staffruler.h" // for ActiveItem

#include "rosedebug.h"

using Rosegarden::Segment;
using Rosegarden::timeT;

//----------------------------------------------------------------------

MatrixView::MatrixView(RosegardenGUIDoc *doc,
                       std::vector<Segment *> segments,
                       QWidget *parent)
    : EditView(doc, segments, parent),
      m_currentEventSelection(0),
      m_hlayout(new MatrixHLayout(&doc->getComposition())),
      m_vlayout(new MatrixVLayout),
      m_hoveredOverAbsoluteTime(0),
      m_hoveredOverNoteName(0)
{
    m_toolBox = new MatrixToolBox(this);

    setupActions();

    initStatusBar();

    QCanvas *tCanvas = new QCanvas(this);
    tCanvas->resize(width() * 2, height() * 2);

    kdDebug(KDEBUG_AREA) << "MatrixView : creating staff\n";

    for (unsigned int i = 0; i < segments.size(); ++i) {
        m_staffs.push_back(new MatrixStaff(tCanvas, segments[i], i,
                                           8)); //!!! so random, so rare
	if (i == 0) m_staffs[i]->setCurrent(true);
    }

    kdDebug(KDEBUG_AREA) << "MatrixView : creating canvas view\n";

    MatrixCanvasView *canvasView =
	new MatrixCanvasView(*m_staffs[0], m_horizontalScrollBar,
                             tCanvas, m_centralFrame);
    setCanvasView(canvasView);

    QObject::connect
        (getCanvasView(), SIGNAL(activeItemPressed(QMouseEvent*, QCanvasItem*)),
         this,         SLOT  (activeItemPressed(QMouseEvent*, QCanvasItem*)));

    QObject::connect
        (getCanvasView(), SIGNAL(mousePressed(Rosegarden::timeT, int, QMouseEvent*, MatrixElement*)),
         this,         SLOT  (slotMousePressed(Rosegarden::timeT, int, QMouseEvent*, MatrixElement*)));

    QObject::connect
        (getCanvasView(), SIGNAL(mouseMoved(Rosegarden::timeT, QMouseEvent*)),
         this,         SLOT  (slotMouseMoved(Rosegarden::timeT, QMouseEvent*)));

    QObject::connect
        (getCanvasView(), SIGNAL(mouseReleased(Rosegarden::timeT, QMouseEvent*)),
         this,         SLOT  (slotMouseReleased(Rosegarden::timeT, QMouseEvent*)));

    QObject::connect
        (getCanvasView(), SIGNAL(hoveredOverNoteChanged(const QString&)),
         this,         SLOT  (slotHoveredOverNoteChanged(const QString&)));

    QObject::connect
        (getCanvasView(), SIGNAL(hoveredOverAbsoluteTimeChanged(unsigned int)),
         this,         SLOT  (slotHoveredOverAbsoluteTimeChanged(unsigned int)));


    kdDebug(KDEBUG_AREA) << "MatrixView : applying layout\n";

    bool layoutApplied = applyLayout();
    if (!layoutApplied) KMessageBox::sorry(0, i18n("Couldn't apply piano roll layout"));
    else {
        kdDebug(KDEBUG_AREA) << "MatrixView : rendering elements\n";
        for (unsigned int i = 0; i < m_staffs.size(); ++i) {
	    m_staffs[i]->positionAllElements();
        }
    }

    BarButtons *topBarButtons = new BarButtons(m_hlayout, 25,
                                               false, m_centralFrame);
    setTopBarButtons(topBarButtons);

    BarButtons *bottomBarButtons = new BarButtons(m_hlayout, 25,
                                                  true, m_centralFrame);
    bottomBarButtons->connectRulerToDocPointer(doc);
    setBottomBarButtons(bottomBarButtons);
}

MatrixView::~MatrixView()
{
    delete m_hlayout;
    delete m_vlayout;

    // Delete remaining canvas items.
    QCanvasItemList allItems = canvas()->allItems();
    QCanvasItemList::Iterator it;

    for (it = allItems.begin(); it != allItems.end(); ++it) delete *it;
}

void MatrixView::saveOptions()
{        
    m_config->setGroup("Matrix Options");
    m_config->writeEntry("Geometry", size());
    m_config->writeEntry("Show Toolbar", toolBar()->isVisible());
    m_config->writeEntry("Show Statusbar",statusBar()->isVisible());
    m_config->writeEntry("ToolBarPos", (int) toolBar()->barPos());
}

void MatrixView::readOptions()
{
    m_config->setGroup("Matrix Options");
        
    QSize size(m_config->readSizeEntry("Geometry"));

    if (!size.isEmpty()) {
        resize(size);
    }
}

void MatrixView::setupActions()
{   
    // File menu
    KStdAction::close   (this, SLOT(closeWindow()),        actionCollection());

    // Edit menu
    KStdAction::cut     (this, SLOT(slotEditCut()),        actionCollection());
    KStdAction::copy    (this, SLOT(slotEditCopy()),       actionCollection());
    KStdAction::paste   (this, SLOT(slotEditPaste()),      actionCollection());

    //
    // Edition tools (eraser, selector...)
    //
    KRadioAction* toolAction = 0;

    toolAction = new KRadioAction(i18n("Paint"), "pencil", 0,
                                  this, SLOT(slotPaintSelected()),
                                  actionCollection(), "paint");
    toolAction->setExclusiveGroup("tools");

    toolAction = new KRadioAction(i18n("Erase"), "eraser", 0,
                                  this, SLOT(slotEraseSelected()),
                                  actionCollection(), "erase");
    toolAction->setExclusiveGroup("tools");

    QString pixmapDir = KGlobal::dirs()->findResource("appdata", "pixmaps/");
    QIconSet icon(QPixmap(pixmapDir + "/toolbar/select.xpm"));

    toolAction = new KRadioAction(i18n("Select"), icon, 0,
                                  this, SLOT(slotSelectSelected()),
                                  actionCollection(), "select");
    toolAction->setExclusiveGroup("tools");

    createGUI("matrix.rc");

    actionCollection()->action("paint")->activate();
}

void MatrixView::initStatusBar()
{
    KStatusBar* sb = statusBar();
    
    m_hoveredOverNoteName      = new QLabel(sb);
    m_hoveredOverAbsoluteTime  = new QLabel(sb);

    m_hoveredOverNoteName->setMinimumWidth(32);
    m_hoveredOverAbsoluteTime->setMinimumWidth(160);

    sb->addWidget(m_hoveredOverAbsoluteTime);
    sb->addWidget(m_hoveredOverNoteName);

    sb->insertItem(KTmpStatusMsg::getDefaultMsg(),
                   KTmpStatusMsg::getDefaultId(), 1);
    sb->setItemAlignment(KTmpStatusMsg::getDefaultId(), 
                         AlignLeft | AlignVCenter);

}


bool MatrixView::applyLayout(int /*staffNo*/)
{
    m_hlayout->reset();
    m_vlayout->reset();
        
    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
        m_hlayout->scanStaff(*m_staffs[i]);
        m_vlayout->scanStaff(*m_staffs[i]);
    }

    m_hlayout->finishLayout();
    m_vlayout->finishLayout();

    double maxWidth = 0.0, maxHeight = 0.0;

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
        m_staffs[i]->sizeStaff(*m_hlayout);

        if (m_staffs[i]->getX() + m_staffs[i]->getTotalWidth() > maxWidth) {
            maxWidth = m_staffs[i]->getX() + m_staffs[i]->getTotalWidth();
        }

        if (m_staffs[i]->getY() + m_staffs[i]->getTotalHeight() > maxHeight) {
            maxHeight = m_staffs[i]->getY() + m_staffs[i]->getTotalHeight();
        }
    }

    readjustViewSize(QSize(int(maxWidth), int(maxHeight)), true);
    
    return true;
}

void MatrixView::refreshSegment(Segment *segment,
				timeT startTime, timeT endTime)
{
    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
	if (!segment || &m_staffs[i]->getSegment() == segment) {
	    applyLayout();
	    m_staffs[i]->positionElements(startTime, endTime);
	    update();
	    return;
	}
    }
}

QSize MatrixView::getViewSize()
{
    return canvas()->size();
}

void MatrixView::setViewSize(QSize s)
{
    canvas()->resize(s.width(), s.height());
}

void MatrixView::update()
{
    canvas()->update();
}

void MatrixView::setCurrentSelection(EventSelection* s)
{
    if (m_currentEventSelection) {
        m_currentEventSelection->removeSelectionFromSegment();
        getStaff(0)->positionElements(m_currentEventSelection->getBeginTime(),
                                      m_currentEventSelection->getEndTime());
    }

    delete m_currentEventSelection;
    m_currentEventSelection = s;

    if (s) {
        s->recordSelectionOnSegment();
        getStaff(0)->positionElements(s->getBeginTime(),
                                      s->getEndTime());
    }

    update();
}


void MatrixView::slotPaintSelected()
{
    EditTool* painter = m_toolBox->getTool(MatrixPainter::ToolName);

    setTool(painter);
}

void MatrixView::slotEraseSelected()
{
    EditTool* eraser = m_toolBox->getTool(MatrixEraser::ToolName);

    setTool(eraser);
}

void MatrixView::slotSelectSelected()
{
    EditTool* selector = m_toolBox->getTool(MatrixSelector::ToolName);

    setTool(selector);
}

void MatrixView::slotMousePressed(Rosegarden::timeT time, int pitch,
                              QMouseEvent* e, MatrixElement* el)
{
    kdDebug(KDEBUG_AREA) << "MatrixView::mousePressed at pitch "
                         << pitch << ", time " << time << endl;

    m_tool->handleMousePress(time, pitch, 0, e, el);
}

void MatrixView::slotMouseMoved(Rosegarden::timeT time, QMouseEvent* e)
{
    if (activeItem()) {
        activeItem()->handleMouseMove(e);
        update();
    }
    else 
        m_tool->handleMouseMove(time, 0, e);
}

void MatrixView::slotMouseReleased(Rosegarden::timeT time, QMouseEvent* e)
{
    if (activeItem()) {
        activeItem()->handleMouseRelease(e);
        setActiveItem(0);
        update();
    }
    m_tool->handleMouseRelease(time, 0, e);
}

void
MatrixView::slotHoveredOverNoteChanged(const QString &noteName)
{
    m_hoveredOverNoteName->setText(QString(" ") + noteName);
}

void
MatrixView::slotHoveredOverAbsoluteTimeChanged(unsigned int time)
{
    timeT t = time;
    Rosegarden::RealTime rt =
	m_document->getComposition().getElapsedRealTime(t);
    long ms = rt.usec / 1000;

    QString message;
    message.sprintf(" Time: %ld (%ld.%03lds)", t, rt.sec, ms);

    m_hoveredOverAbsoluteTime->setText(message);
}



//////////////////////////////////////////////////////////////////////
//                    Slots
//////////////////////////////////////////////////////////////////////

//
// Cut, Copy, Paste
//
void MatrixView::slotEditCut()
{
    kdDebug(KDEBUG_AREA) << "MatrixView::slotEditCut()\n";

    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Cutting selection..."), statusBar());

    kdDebug(KDEBUG_AREA) << "MatrixView::slotEditCut() : cutting selection\n";

    m_currentEventSelection->cut();

    emit usedSelection();

    refreshSegment(&m_currentEventSelection->getSegment(),
		   m_currentEventSelection->getBeginTime(),
		   m_currentEventSelection->getEndTime());

    kdDebug(KDEBUG_AREA) << "MatrixView::slotEditCut() : selection duration = "
                         << m_currentEventSelection->getTotalDuration() << endl;
}

void MatrixView::slotEditCopy()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Copying selection to clipboard..."), statusBar());

    m_currentEventSelection->copy();

    emit usedSelection();
}

void MatrixView::slotEditPaste()
{
}
