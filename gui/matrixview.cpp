// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2001
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
#include <kmessagebox.h>

#include "NotationTypes.h"
#include "Event.h"

#include "BaseProperties.h"
#include "matrixview.h"
#include "rosegardenguidoc.h"
#include "ktmpstatusmsg.h"

MatrixCanvasView::MatrixCanvasView(QCanvas *viewing, QWidget *parent,
                                   const char *name, WFlags f)
    : QCanvasView(viewing, parent, name, f)
{
}

MatrixCanvasView::~MatrixCanvasView()
{
}

//----------------------------------------------------------------------

MatrixVLayout::MatrixVLayout()
    : m_pitchScaleFactor(10.0),
      m_staffIdScaleFactor(100.0)
{
}

MatrixVLayout::~MatrixVLayout()
{
}

void MatrixVLayout::reset()
{
}

void MatrixVLayout::resetStaff(StaffType&)
{
}

void MatrixVLayout::scanStaff(MatrixVLayout::StaffType& staffBase)
{
    MatrixStaff& staff = dynamic_cast<MatrixStaff&>(staffBase);

    using Rosegarden::BaseProperties::PITCH;

    MatrixElementList *notes = staff.getViewElementList();

    MatrixElementList::iterator from = notes->begin();
    MatrixElementList::iterator to = notes->end();
    MatrixElementList::iterator i;

    for (i = from; i != to; ++i) {

        MatrixElement *el = (*i);

        if (!el->isNote()) continue; // notes only
        
        int pitch = el->event()->get<Rosegarden::Int>(PITCH);

        el->setLayoutY(pitch * m_pitchScaleFactor +
                       staff.getId() * m_staffIdScaleFactor);
        el->setHeight(m_pitchScaleFactor);
    }

}

void MatrixVLayout::finishLayout()
{
}

//-----------------------------------

MatrixHLayout::MatrixHLayout(unsigned int durationScaleFactor)
    : m_totalWidth(0),
      m_durationScaleFactor(durationScaleFactor)
{
}

MatrixHLayout::~MatrixHLayout()
{
}

void MatrixHLayout::reset()
{
}

void MatrixHLayout::resetStaff(StaffType&)
{
}

void MatrixHLayout::scanStaff(MatrixHLayout::StaffType& staff)
{
    m_totalWidth = 0;
    double currentX = 0.0;

    MatrixElementList *notes = staff.getViewElementList();

    MatrixElementList::iterator from = notes->begin();
    MatrixElementList::iterator to = notes->end();
    MatrixElementList::iterator i;

    for (i = from; i != to; ++i) {

        MatrixElement *el = (*i);
        Rosegarden::timeT duration = el->event()->getDuration();
        el->setLayoutX(currentX);
        double width = duration * m_durationScaleFactor;
        el->setWidth(width);
        currentX += width;

    }

    m_totalWidth = currentX + 50;
}

double MatrixHLayout::getTotalWidth()
{
    return m_totalWidth;
}

unsigned int MatrixHLayout::getBarLineCount(StaffType &staff)
{
    return 0;
}

double MatrixHLayout::getBarLineX(StaffType &staff, unsigned int barNo)
{
    return 0;
}

void MatrixHLayout::finishLayout()
{
}

//----------------------------------------------------------------------

MatrixElement::MatrixElement(Rosegarden::Event *event)
    : Rosegarden::ViewElement(event),
      m_canvasRect(new QCanvasRectangle(0))
{
}

MatrixElement::~MatrixElement()
{
    m_canvasRect->hide();
    delete m_canvasRect;
}

void MatrixElement::setCanvas(QCanvas* c)
{
    if (!m_canvasRect->canvas()) {
        
        m_canvasRect->setCanvas(c);
        m_canvasRect->setBrush(Qt::blue);
        m_canvasRect->show();

    }
}

bool MatrixElement::isNote() const
{
    return event()->isa(Rosegarden::Note::EventType);
}

//----------------------------------------------------------------------
MatrixStaff::MatrixStaff(QCanvas* c, Rosegarden::Segment* segment,
                         unsigned int id)
    : Rosegarden::Staff<MatrixElement>(*segment),
      m_canvas(c),
      m_id(id)
{
}

void MatrixStaff::renderElements(MatrixElementList::iterator from,
                                 MatrixElementList::iterator to)
{
    for (MatrixElementList::iterator i = from;
         i != to; ++i) {
        
        MatrixElement* el = (*i);

        el->setCanvas(m_canvas);
    }
}

void MatrixStaff::renderElements()
{
    renderElements(getViewElementList()->begin(),
		   getViewElementList()->end());
}

//----------------------------------------------------------------------

MatrixView::MatrixView(RosegardenGUIDoc *doc,
                       std::vector<Rosegarden::Segment *> segments,
                       QWidget *parent)
    : EditView(doc, segments, parent),
      m_canvasView(new MatrixCanvasView(new QCanvas(width() * 2,
                                                    height() * 2),
                                        this)),
      m_hLayout(new MatrixHLayout),
      m_vLayout(new MatrixVLayout)
{
    setCentralWidget(m_canvasView);

    for (unsigned int i = 0; i < segments.size(); ++i)
        m_staffs.push_back(new MatrixStaff(canvas(), segments[i], i));

    bool layoutApplied = applyLayout();
    if (!layoutApplied) KMessageBox::sorry(0, i18n("Couldn't apply piano roll layout"));
    else {
        for (unsigned int i = 0; i < m_staffs.size(); ++i) {
            m_staffs[i]->renderElements();
	    // m_staffs[i]->positionElements();
        }
    }
}

MatrixView::~MatrixView()
{
    delete m_hLayout;
    delete m_vLayout;

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
    KStdAction::close (this, SLOT(closeWindow()),          actionCollection());

    KStdAction::cut     (this, SLOT(slotEditCut()),        actionCollection());
    KStdAction::copy    (this, SLOT(slotEditCopy()),       actionCollection());
    KStdAction::paste   (this, SLOT(slotEditPaste()),      actionCollection());

    createGUI("matrix.rc");
}

void MatrixView::initStatusBar()
{
}


bool MatrixView::applyLayout()
{
    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
        m_hLayout->scanStaff(*m_staffs[i]);
        m_vLayout->scanStaff(*m_staffs[i]);
    }

    return true;
}


//////////////////////////////////////////////////////////////////////
//                    Slots
//////////////////////////////////////////////////////////////////////

//
// Undo, Redo
//
void MatrixView::slotEditUndo()
{
    KTmpStatusMsg msg(i18n("Undo..."), statusBar());
}

void MatrixView::slotEditRedo()
{
    KTmpStatusMsg msg(i18n("Redo..."), statusBar());
}

//
// Cut, Copy, Paste
//
void MatrixView::slotEditCut()
{
}

void MatrixView::slotEditCopy()
{
}

void MatrixView::slotEditPaste()
{
}

