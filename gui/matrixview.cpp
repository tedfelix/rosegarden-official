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
#include <cmath>

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

#include "rosedebug.h"

using Rosegarden::Segment;
using Rosegarden::timeT;

MatrixCanvasView::MatrixCanvasView(MatrixStaff& staff,
                                   QCanvas *viewing, QWidget *parent,
                                   const char *name, WFlags f)
    : QCanvasView(viewing, parent, name, f),
      m_staff(staff)
{
}

MatrixCanvasView::~MatrixCanvasView()
{
}

void MatrixCanvasView::contentsMousePressEvent(QMouseEvent* e)
{
    QPoint eventPos = e->pos();
    
    timeT evTime = m_staff.xToTime(eventPos.x());
    int evPitch = m_staff.yToPitch(eventPos.y());

    kdDebug(KDEBUG_AREA) << "MatrixCanvasView::contentsMousePressEvent() at time "
                         << evTime << ", pitch " << evPitch << endl;

}

void MatrixCanvasView::contentsMouseReleaseEvent(QMouseEvent*)
{
}

void MatrixCanvasView::contentsMouseMoveEvent(QMouseEvent*)
{
}

//----------------------------------------------------------------------

MatrixVLayout::MatrixVLayout()
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

    kdDebug(KDEBUG_AREA) << "MatrixVLayout::scanStaff : id = "
                         << staff.getId() << endl;


    for (i = from; i != to; ++i) {

        MatrixElement *el = (*i);

        if (!el->isNote()) continue; // notes only
        
        int pitch = el->event()->get<Rosegarden::Int>(PITCH);

        double y = (maxMIDIPitch - pitch) * staff.getPitchScaleFactor();
        
        kdDebug(KDEBUG_AREA) << "MatrixVLayout::scanStaff : y = "
                             << y << " for pitch " << pitch << endl;

        el->setLayoutY(y);
        el->setHeight(staff.getPitchScaleFactor());
    }

}

void MatrixVLayout::finishLayout()
{
}

const unsigned int MatrixStaff::defaultPitchScaleFactor = 10;
const unsigned int MatrixVLayout::maxMIDIPitch = 127;

//-----------------------------------

MatrixHLayout::MatrixHLayout()
    : m_totalWidth(0)
{
}

MatrixHLayout::~MatrixHLayout()
{
}

void MatrixHLayout::reset()
{
    m_barData.clear();
}

void MatrixHLayout::resetStaff(StaffType&)
{
}

void MatrixHLayout::scanStaff(MatrixHLayout::StaffType& staffBase)
{
    MatrixStaff& staff = dynamic_cast<MatrixStaff&>(staffBase);

    m_totalWidth = 0;
    double currentX = 0.0;

    MatrixElementList *notes = staff.getViewElementList();

    MatrixElementList::iterator from = notes->begin();
    MatrixElementList::iterator to = notes->end();
    MatrixElementList::iterator i;

    float timeScaleFactor = staff.getTimeScaleFactor();

    for (i = from; i != to; ++i) {

        MatrixElement *el = (*i);

        if (el->isNote()) {

            Rosegarden::timeT duration = el->event()->getDuration();
            Rosegarden::timeT time =  el->event()->getAbsoluteTime();

            el->setLayoutX(time * timeScaleFactor);
            double width = duration * timeScaleFactor;
            el->setWidth(int(width));

            currentX = el->getLayoutX() + width;

        } else { // it's a time sig change

        }
    }

    // margin at the right end of the window
    m_totalWidth = currentX + 50;

//     MatrixStaff& mstaff = dynamic_cast<MatrixStaff&>(staff);
//     mstaff.setBarData(m_barData);
}

double MatrixHLayout::getTotalWidth()
{
    return m_totalWidth;
}

unsigned int MatrixHLayout::getBarLineCount(StaffType&)
{
    return 0;
}

double MatrixHLayout::getBarLineX(StaffType&, unsigned int)
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
        m_canvasRect->setPen(Qt::blue);
        m_canvasRect->show();

    }
}

bool MatrixElement::isNote() const
{
    return event()->isa(Rosegarden::Note::EventType);
}

//----------------------------------------------------------------------
MatrixStaff::MatrixStaff(QCanvas* c, Segment* segment,
                         unsigned int id, unsigned int pitchScaleFactor)
    : Rosegarden::Staff<MatrixElement>(*segment),
      m_canvas(c),
      m_id(id),
      m_pitchScaleFactor(pitchScaleFactor),
      m_timeScaleFactor(0.25),
      m_timeResolution(96.0),
      m_currentBarLength(4)
{
    createLines();
}

MatrixStaff::~MatrixStaff()
{
    // delete all lines
    for(StaffLineList::iterator i = m_staffHLines.begin();
        i != m_staffHLines.end(); ++i) {
        (*i)->hide();
        delete (*i);
    }

    for(StaffLineList::iterator i = m_staffVLines.begin();
        i != m_staffVLines.end(); ++i) {
        (*i)->hide();
        delete (*i);
    }
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

void MatrixStaff::resizeStaffHLines()
{
    for(StaffLineList::iterator i = m_staffHLines.begin();
        i != m_staffHLines.end(); ++i) {
        QCanvasLine* line = (*i);
        int y = line->startPoint().y();
        line->setPoints(0, y, m_canvas->size().width(), y);
    }
}

timeT MatrixStaff::xToTime(double x)
{
    double t = std::floor(x / (m_timeResolution * m_timeScaleFactor)) * m_timeResolution;
    return timeT(t);
}

int MatrixStaff::yToPitch(double y)
{
    double t = std::floor(y / m_pitchScaleFactor);

    int pitch = 127 - int(t);

    return pitch;
}

bool MatrixStaff::wrapEvent(Rosegarden::Event* e)
{
    return
        e->isa(Rosegarden::Note::EventType) || 
        e->isa(Rosegarden::TimeSignature::EventType);
}

void MatrixStaff::createLines()
{
    for(unsigned int i = 0; i <= nbHLines; ++i) {
        QCanvasLine* line = new QCanvasLine(m_canvas);
        int y = i * m_pitchScaleFactor;
        line->setPoints(0, y, m_canvas->size().width(), y);
        line->show();
        m_staffHLines.push_back(line);
    }

    unsigned int nbVLines = int(getSegment().getDuration() / m_timeResolution);

    for (unsigned int i = 0; i <= nbVLines; ++i) {
        QCanvasLine* line = new QCanvasLine(m_canvas);
        double x = i * m_timeResolution * m_timeScaleFactor;
        line->setPoints(0, 0, 0, m_canvas->size().height());
        line->setX(x);
        if ((i % m_currentBarLength) != 0)
            line->setPen(Qt::lightGray);
        line->show();
        m_staffVLines.push_back(line);
    }
}

const unsigned int MatrixStaff::nbHLines = 127;
    
//----------------------------------------------------------------------

MatrixView::MatrixView(RosegardenGUIDoc *doc,
                       std::vector<Segment *> segments,
                       QWidget *parent)
    : EditView(doc, segments, parent),
      m_canvasView(0),
      m_hlayout(new MatrixHLayout),
      m_vlayout(new MatrixVLayout)
{
    setupActions();

    QCanvas *tCanvas = new QCanvas(segments[0]->getDuration() / 2,
                                   MatrixStaff::defaultPitchScaleFactor *
                                   MatrixStaff::nbHLines);

    kdDebug(KDEBUG_AREA) << "MatrixView : creating staff\n";

    for (unsigned int i = 0; i < segments.size(); ++i)
        m_staffs.push_back(new MatrixStaff(tCanvas, segments[i], i));

    kdDebug(KDEBUG_AREA) << "MatrixView : creating canvas view\n";

    m_canvasView = new MatrixCanvasView(*m_staffs[0], tCanvas, this);

    setCentralWidget(m_canvasView);

    kdDebug(KDEBUG_AREA) << "MatrixView : applying layout\n";

    bool layoutApplied = applyLayout();
    if (!layoutApplied) KMessageBox::sorry(0, i18n("Couldn't apply piano roll layout"));
    else {
        kdDebug(KDEBUG_AREA) << "MatrixView : rendering elements\n";
        for (unsigned int i = 0; i < m_staffs.size(); ++i) {
            m_staffs[i]->renderElements();
	    // m_staffs[i]->positionElements();
        }
    }
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

    createGUI("matrix.rc");
}

void MatrixView::initStatusBar()
{
}


bool MatrixView::applyLayout(int staffNo)
{
    m_hlayout->reset();
    m_vlayout->reset();
        
    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
        m_hlayout->scanStaff(*m_staffs[i]);
        m_vlayout->scanStaff(*m_staffs[i]);
    }

    m_hlayout->finishLayout();
    m_vlayout->finishLayout();

    readjustViewSize(QSize(int(m_hlayout->getTotalWidth()),
                           getViewSize().height()));
    
    return true;
}

QSize MatrixView::getViewSize()
{
    return canvas()->size();
}

void MatrixView::setViewSize(QSize s)
{
    canvas()->resize(s.width(), s.height());
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

