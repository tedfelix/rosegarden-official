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
#include "Composition.h"
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
    timeT evTime = m_staff.getTimeForCanvasX(e->x());
    int evPitch = m_staff.getHeightAtCanvasY(e->y());

    
    kdDebug(KDEBUG_AREA) << "MatrixCanvasView::contentsMousePressEvent() at pitch "
                         << evPitch << ", time " << evTime << endl;

    //     QCanvasItemList itemList = canvas()->collisions(e->pos());
    //     QCanvasItemList::Iterator it;
    //     for (it = itemList.begin(); it != itemList.end(); ++it) {

    //         QCanvasItem *item = *it;
    //         MatrixElement* mel;
        
    //     }

    emit mousePressed(evTime, evPitch, e, 0);
    
}

void MatrixCanvasView::contentsMouseMoveEvent(QMouseEvent* e)
{
    timeT evTime = m_staff.getTimeForCanvasX(e->x());

    kdDebug(KDEBUG_AREA) << "MatrixCanvasView::contentsMouseMoveEvent() at time "
                         << evTime << endl;

    emit mouseMoved(evTime, e);
}

void MatrixCanvasView::contentsMouseReleaseEvent(QMouseEvent* e)
{
    timeT evTime = m_staff.getTimeForCanvasX(e->x());

    emit mouseReleased(evTime, e);
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

	int y = staff.getLayoutYForHeight(pitch) - staff.getElementHeight();

        el->setLayoutY(y);
        el->setHeight(staff.getElementHeight());
    }

}

void MatrixVLayout::finishLayout()
{
}

const unsigned int MatrixVLayout::maxMIDIPitch = 127;

//-----------------------------------

MatrixHLayout::MatrixHLayout() :
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

void MatrixHLayout::scanStaff(MatrixHLayout::StaffType &staffBase)
{
    // The Matrix layout is not currently designed to be able to lay
    // out more than one staff, because we have no requirement to show
    // more than one at once in the Matrix view.  To make it work for
    // multiple staffs should be straightforward; we just need to bear
    // in mind that they might start and end at different times (hence
    // the total width and bar list can't just be calculated from the
    // last staff scanned as they are now).

    MatrixStaff &staff = dynamic_cast<MatrixStaff &>(staffBase);

    // Do this in two parts: bar lines separately from elements.
    // (We don't need to do all that stuff notationhlayout has to do,
    // scanning the notes bar-by-bar; we can just place the bar lines
    // in the theoretically-correct places and do the same with the
    // notes quite independently.)

    // 1. Bar lines and time signatures

    m_barData.clear();
    Rosegarden::Segment &segment = staff.getSegment();
    Rosegarden::Composition *composition = segment.getComposition();

    timeT from = composition->getBarStart(segment.getStartIndex()),
	    to = composition->getBarEnd  (segment.getEndIndex  ());

    //!!! Deal with time signatures...

    while (from < to) {
	m_barData.push_back(BarData(from * staff.getTimeScaleFactor(), 0));
	from = composition->getBarEnd(from);
    }
    m_barData.push_back(BarData(to * staff.getTimeScaleFactor(), 0));

    // 2. Elements

    m_totalWidth = 0.0;

    MatrixElementList *notes = staff.getViewElementList();
    MatrixElementList::iterator i = notes->begin();

    while (i != notes->end()) {

	(*i)->setLayoutX((*i)->getAbsoluteTime() * staff.getTimeScaleFactor());

	double width = (*i)->getDuration() * staff.getTimeScaleFactor();
	(*i)->setWidth((int)width);
	
	m_totalWidth = (*i)->getLayoutX() + width;
	++i;
    }
}

double MatrixHLayout::getTotalWidth()
{
    return m_totalWidth;
}

unsigned int MatrixHLayout::getBarLineCount(StaffType&)
{
    return m_barData.size();
}

double MatrixHLayout::getBarLineX(StaffType&, unsigned int barNo)
{
    return m_barData[barNo].first;
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
    LinedStaff<MatrixElement>(canvas, segment, id, vResolution, 1),
    m_scaleFactor(0.25) //!!!
{
    // nothing else yet
}

MatrixStaff::~MatrixStaff()
{
    // nothing
}

int  MatrixStaff::getLineCount()        const { return MatrixVLayout::
						      maxMIDIPitch + 1; }
int  MatrixStaff::getLegerLineCount()   const { return 0; }
int  MatrixStaff::getBottomLineHeight() const { return 0; }
int  MatrixStaff::getHeightPerLine()    const { return 1; }
bool MatrixStaff::elementsInSpaces()    const { return true; }

bool MatrixStaff::wrapEvent(Rosegarden::Event* e)
{
    // Changed from "Note or Time signature" to just "Note" because 
    // there should be no time signature events in any ordinary
    // segments, they're only in the composition's ref segment

    return e->isa(Rosegarden::Note::EventType);
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
	positionElement(*i);
    }
}

void MatrixStaff::positionElement(MatrixElement* el)
{
    LinedStaffCoords coords = getCanvasCoordsForLayoutCoords(el->getLayoutX(),
                                                             int(el->getLayoutY()));

    el->setCanvas(m_canvas);
    el->setCanvasX(coords.first);
    el->setCanvasY((double)coords.second);
}


timeT MatrixStaff::getTimeForCanvasX(double x)
{
    double layoutX = x - m_x;
    return (timeT)(layoutX / m_scaleFactor);
}

//----------------------------------------------------------------------

MatrixView::MatrixView(RosegardenGUIDoc *doc,
                       std::vector<Segment *> segments,
                       QWidget *parent)
    : EditView(doc, segments, parent),
      m_canvasView(0),
      m_hlayout(new MatrixHLayout),
      m_vlayout(new MatrixVLayout)
{
    m_toolBox = new MatrixToolBox(this);

    setupActions();

    QCanvas *tCanvas = new QCanvas(100, 100);

    kdDebug(KDEBUG_AREA) << "MatrixView : creating staff\n";

    for (unsigned int i = 0; i < segments.size(); ++i)
        m_staffs.push_back(new MatrixStaff(tCanvas, segments[i], i,
                                           10)); //!!!

    kdDebug(KDEBUG_AREA) << "MatrixView : creating canvas view\n";

    m_canvasView = new MatrixCanvasView(*m_staffs[0], tCanvas, this);

    setCentralWidget(m_canvasView);

    QObject::connect
        (m_canvasView, SIGNAL(mousePressed(Rosegarden::timeT, int, QMouseEvent*, MatrixElement*)),
         this,         SLOT  (mousePressed(Rosegarden::timeT, int, QMouseEvent*, MatrixElement*)));

    QObject::connect
        (m_canvasView, SIGNAL(mouseMoved(Rosegarden::timeT, QMouseEvent*)),
         this,         SLOT  (mouseMoved(Rosegarden::timeT, QMouseEvent*)));

    QObject::connect
        (m_canvasView, SIGNAL(mouseReleased(Rosegarden::timeT, QMouseEvent*)),
         this,         SLOT  (mouseReleased(Rosegarden::timeT, QMouseEvent*)));



    kdDebug(KDEBUG_AREA) << "MatrixView : applying layout\n";

    bool layoutApplied = applyLayout();
    if (!layoutApplied) KMessageBox::sorry(0, i18n("Couldn't apply piano roll layout"));
    else {
        kdDebug(KDEBUG_AREA) << "MatrixView : rendering elements\n";
        for (unsigned int i = 0; i < m_staffs.size(); ++i) {
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

    slotPaintSelected();
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

    readjustViewSize(QSize(int(maxWidth), int(maxHeight)));
    
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

void MatrixView::mousePressed(Rosegarden::timeT time, int pitch,
                              QMouseEvent* e, MatrixElement* el)
{
    kdDebug(KDEBUG_AREA) << "MatrixView::mousePressed at pitch "
                         << pitch << ", time " << time << endl;

    m_tool->handleMousePress(pitch, time, 0, e, el);
}

void MatrixView::mouseMoved(Rosegarden::timeT time, QMouseEvent* e)
{
    m_tool->handleMouseMove(0, time, e);
}

void MatrixView::mouseReleased(Rosegarden::timeT time, QMouseEvent* e)
{
    m_tool->handleMouseRelease(0, time, e);
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
using Rosegarden::Note;

MatrixPainter::MatrixPainter(MatrixView* parent)
    : MatrixTool("MatrixPainter", parent),
      m_currentElement(0),
      m_currentStaff(0),
      m_resolution(Note::QuarterNote),
      m_basicDuration(0)
{
    Note tmpNote(m_resolution);

    m_basicDuration = tmpNote.getDuration();
}

void MatrixPainter::handleLeftButtonPress(Rosegarden::timeT time,
                                          int pitch,
                                          int staffNo,
                                          QMouseEvent*,
                                          Rosegarden::ViewElement*)
{
    kdDebug(KDEBUG_AREA) << "MatrixPainter::handleLeftButtonPress : pitch = "
                         << pitch << ", time : " << time << endl;

    Note newNote(m_resolution);

    // Round event time to a multiple of resolution
    timeT noteDuration = newNote.getDuration();
    
    time = (time / noteDuration) * noteDuration;

    Event* el = newNote.getAsNoteEvent(pitch, time);

    m_currentElement = new MatrixElement(el);

    m_currentStaff = m_mParentView->getStaff(staffNo);

    int y = m_currentStaff->getLayoutYForHeight(pitch) - m_currentStaff->getElementHeight() / 2;

    m_currentElement->setLayoutY(y);
    m_currentElement->setLayoutX(time * m_currentStaff->getTimeScaleFactor());
    m_currentElement->setHeight(m_currentStaff->getElementHeight());

    double width = noteDuration * m_currentStaff->getTimeScaleFactor();
    m_currentElement->setWidth(int(width));

    m_currentStaff->positionElement(m_currentElement);
    m_mParentView->canvas()->update();
}

void MatrixPainter::handleMouseMove(Rosegarden::timeT newTime,
                                    int,
                                    QMouseEvent*)
{
    newTime = (newTime / m_basicDuration) * m_basicDuration;

    if (newTime == m_currentElement->getAbsoluteTime()) return;

    timeT newDuration = newTime - m_currentElement->getAbsoluteTime();

    using Rosegarden::BaseProperties::PITCH;

    kdDebug(KDEBUG_AREA) << "MatrixPainter::handleMouseMove : new time = "
                         << newTime << ", old time = "
                         << m_currentElement->getAbsoluteTime()
                         << ", new duration = "
                         << newDuration
                         << ", pitch = "
                         << m_currentElement->event()->get<Rosegarden::Int>(PITCH)
                         << endl;

    m_currentElement->setDuration(newDuration);

    double width = newDuration * m_currentStaff->getTimeScaleFactor();
    m_currentElement->setWidth(int(width));

    m_mParentView->canvas()->update();
}

void MatrixPainter::handleMouseRelease(Rosegarden::timeT,
                                       int,
                                       QMouseEvent*)
{
    // Insert element if it has a non null duration,
    // discard it otherwise
    //
    if (m_currentElement->getDuration() != 0) {
        
        m_currentStaff->insert(m_currentElement->event(), true);

    } else {

        delete m_currentElement;

    }
    
    m_currentElement = 0;

}

void MatrixPainter::setResolution(Rosegarden::Note::Type note)
{
    m_resolution = note;
}


const QString MatrixPainter::ToolName = "painter";
