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

#include <qiconset.h>

#include <kapp.h>
#include <kconfig.h>
#include <kaction.h>
#include <kstddirs.h>
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
    timeT evTime = 0;
    int evPitch = 0;
    eventTimePitch(e, evTime, evPitch);

    
    kdDebug(KDEBUG_AREA) << "MatrixCanvasView::contentsMousePressEvent() at time "
                         << evTime << ", pitch " << evPitch << endl;

    //     QCanvasItemList itemList = canvas()->collisions(e->pos());
    //     QCanvasItemList::Iterator it;
    //     for (it = itemList.begin(); it != itemList.end(); ++it) {

    //         QCanvasItem *item = *it;
    //         MatrixElement* mel;
        
    //     }

    emit itemPressed(evTime, evPitch, e, 0);
    
}

void MatrixCanvasView::contentsMouseReleaseEvent(QMouseEvent* e)
{
    timeT evTime = 0;
    int evPitch = 0;
    eventTimePitch(e, evTime, evPitch);

    emit itemReleased(evTime, e);
}

void MatrixCanvasView::contentsMouseMoveEvent(QMouseEvent* e)
{
    timeT evTime = 0;
    int evPitch = 0;
    eventTimePitch(e, evTime, evPitch);

    emit itemResized(evTime, e);
}

void MatrixCanvasView::eventTimePitch(QMouseEvent* e,
                                      Rosegarden::timeT& evTime,
                                      int& pitch)
{
    QPoint eventPos = e->pos();

/*!!! implement xToTime in staff or hlayout; use LinedStaff methods
      for yToPitch (getHeightAtCanvasY(), & returned height is MIDI pitch) 

    evTime = m_staff.xToTime(eventPos.x());
    pitch = m_staff.yToPitch(eventPos.y());
*/
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

//!!!        double y = (maxMIDIPitch - pitch) * staff.getPitchScaleFactor();
     
        int y = staff.getLayoutYForHeight(pitch);
   
        kdDebug(KDEBUG_AREA) << "MatrixVLayout::scanStaff : y = "
                             << y << " for pitch " << pitch << endl;

        el->setLayoutY(y);
        el->setHeight(staff.getElementHeight());
    }

}

void MatrixVLayout::finishLayout()
{
}

//!!!const unsigned int MatrixStaff::defaultPitchScaleFactor = 10;
const unsigned int MatrixVLayout::maxMIDIPitch = 127;

//-----------------------------------

MatrixHLayout::MatrixHLayout(double scaleFactor) :
    m_scaleFactor(scaleFactor),
    m_totalWidth(0.0)
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

    for (i = from; i != to; ++i) {

        MatrixElement *el = (*i);

        if (el->isNote()) {

            Rosegarden::timeT duration = el->event()->getDuration();
            Rosegarden::timeT time =  el->event()->getAbsoluteTime();

            el->setLayoutX(time * m_scaleFactor);
            double width = duration * m_scaleFactor;
            el->setWidth(int(width));

            currentX = el->getLayoutX() + width;

        } else { // it's a time sig change

        }
    }

    // margin at the right end of the window
//!!! no, view should manage that, it's not the layout's job
//!!! m_totalWidth = currentX + 50;
    m_totalWidth = currentX;

//     MatrixStaff& mstaff = dynamic_cast<MatrixStaff&>(staff);
//     mstaff.setBarData(m_barData);
}

double MatrixHLayout::getTotalWidth()
{
    return m_totalWidth;
}

unsigned int MatrixHLayout::getBarLineCount(StaffType&)
{
    //!!! This is now essential to ensure the staff resizes itself correctly.
    // Also will need getTimeSignatureInBar so as to place bar subdivisions
    // in the right places

    return 0;
}

double MatrixHLayout::getBarLineX(StaffType&, unsigned int)
{
    //!!! This is now essential to ensure the staff resizes itself correctly.
    return 0;
}

void MatrixHLayout::finishLayout()
{
}

//----------------------------------------------------------------------

MatrixElement::MatrixElement(Rosegarden::Event *event) :
    Rosegarden::ViewElement(event),
    m_canvasRect(new QCanvasRectangle(0)),
    m_layoutX(0.0),
    m_layoutY(0.0)
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


MatrixStaff::MatrixStaff(QCanvas *canvas, Segment *segment,
                         int id, int vResolution) :
    LinedStaff<MatrixElement>(canvas, segment, id, vResolution, 1)
{
    // nothing else yet
}

MatrixStaff::~MatrixStaff()
{
    // nothing
}

int
MatrixStaff::getLineCount() const
{
    return MatrixVLayout::maxMIDIPitch + 2;
}

int
MatrixStaff::getLegerLineCount() const
{
    return 0;
}

int
MatrixStaff::getBottomLineHeight() const
{
    // simply define height as equal to MIDI pitch, for this sort of staff
    return 0; 
}

int
MatrixStaff::getHeightPerLine() const
{
    // simply define height as equal to MIDI pitch, for this sort of staff
    return 1;
}

bool MatrixStaff::wrapEvent(Rosegarden::Event* e)
{
    return
        e->isa(Rosegarden::Note::EventType) || 
        e->isa(Rosegarden::TimeSignature::EventType);
}

void
MatrixStaff::positionElements(timeT from, timeT to)
{
    MatrixElementList *mel = getViewElementList();

    MatrixElementList::iterator beginAt = mel->begin();
    if (from >= 0) beginAt = mel->findTime(from);
    if (beginAt != mel->begin()) --beginAt;

    MatrixElementList::iterator endAt = mel->end();
    if (to >= 0) endAt = mel->findTime(to);

    for (MatrixElementList::iterator i = beginAt; i != endAt; ++i) {
        
        LinedStaffCoords coords = getCanvasCoordsForLayoutCoords((*i)->getLayoutX(),
                                                                 (*i)->getLayoutY());

        (*i)->setCanvas(m_canvas);
        (*i)->setCanvasX(coords.first);
        (*i)->setCanvasY((double)coords.second);
    }
}

//----------------------------------------------------------------------

MatrixView::MatrixView(RosegardenGUIDoc *doc,
                       std::vector<Segment *> segments,
                       QWidget *parent)
    : EditView(doc, segments, parent),
      m_canvasView(0),
      m_hlayout(new MatrixHLayout(0.25)), //!!!
      m_vlayout(new MatrixVLayout)
{
    setupActions();
/*!!!
    QCanvas *tCanvas = new QCanvas(segments[0]->getDuration() / 2,
                                   MatrixStaff::defaultPitchScaleFactor *
                                   MatrixStaff::nbHLines);
*/
    QCanvas *tCanvas = new QCanvas(100, 100);

    kdDebug(KDEBUG_AREA) << "MatrixView : creating staff\n";

    for (unsigned int i = 0; i < segments.size(); ++i)
        m_staffs.push_back(new MatrixStaff(tCanvas, segments[i], i,
                                           10)); //!!!

    kdDebug(KDEBUG_AREA) << "MatrixView : creating canvas view\n";

    m_canvasView = new MatrixCanvasView(*m_staffs[0], tCanvas, this);

    setCentralWidget(m_canvasView);

    QObject::connect
        (m_canvasView, SIGNAL(itemPressed(int, Rosegarden::timeT, QMouseEvent*, MatrixElement*)),
         this,         SLOT  (itemPressed(int, Rosegarden::timeT, QMouseEvent*, MatrixElement*)));



    kdDebug(KDEBUG_AREA) << "MatrixView : applying layout\n";

    bool layoutApplied = applyLayout();
    if (!layoutApplied) KMessageBox::sorry(0, i18n("Couldn't apply piano roll layout"));
    else {
        kdDebug(KDEBUG_AREA) << "MatrixView : rendering elements\n";
        for (unsigned int i = 0; i < m_staffs.size(); ++i) {
            // m_staffs[i]->renderElements();
            m_staffs[i]->positionElements();
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

    //
    // Edition tools (eraser, selector...)
    //
    KRadioAction* toolAction = 0;

    toolAction = new KRadioAction(i18n("Paint"), "pencil", 0,
                                  this, SLOT(slotEraseSelected()),
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
}

void MatrixView::initStatusBar()
{
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

    readjustViewSize(QSize(maxWidth, maxHeight));
    
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

void MatrixView::slotPaintSelected()
{
    MatrixPainter* painter = dynamic_cast<MatrixPainter*>(m_toolBox->getTool(MatrixPainter::ToolName));

    setTool(painter);
}

void MatrixView::slotEraseSelected()
{
}

void MatrixView::slotSelectSelected()
{
}

void MatrixView::itemPressed(int pitch, Rosegarden::timeT time,
                             QMouseEvent*, MatrixElement*)
{
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

MatrixPainter::MatrixPainter(MatrixView* parent)
    : MatrixTool("MatrixPainter", parent)
{
}

void MatrixPainter::handleLeftButtonPress(int pitch,
                                          Rosegarden::timeT time,
                                          int staffNo,
                                          QMouseEvent *event,
                                          Rosegarden::ViewElement*)
{
    kdDebug(KDEBUG_AREA) << "MatrixPainter::handleLeftButtonPress : pitch = "
                         << pitch << ", time : " << time << endl;
}

const QString MatrixPainter::ToolName = "painter";
