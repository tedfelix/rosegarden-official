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

#include <qpopupmenu.h>
#include <qwhatsthis.h>

#include <klocale.h>
#include <kmessagebox.h>

#include "segmentcanvas.h"
#include "Segment.h"

#include "rosedebug.h"
#include "colours.h"

using Rosegarden::Segment;

//////////////////////////////////////////////////////////////////////
//                SegmentItem
//////////////////////////////////////////////////////////////////////

SegmentItem::SegmentItem(int x, int y,
                     int nbSteps,
                     QCanvas* canvas)
    : QCanvasRectangle(x, y,
                       nbBarsToWidth(nbSteps), m_itemHeight,
                       canvas),
      m_segment(0),
      m_selected(false)
{
}

int SegmentItem::getItemNbBars() const
{
    kdDebug(KDEBUG_AREA) << "SegmentItem::getItemNbBars() : "
                         << rect().width() / m_widthToDurationRatio
                         << endl;

    return widthToNbBars(width());
}

int SegmentItem::getStartBar() const
{
    return (int)(x() / m_widthToDurationRatio * m_barResolution);
}

int SegmentItem::getTrack() const
{
    return m_segment->getTrack();
}

void SegmentItem::setTrack(int t)
{
    m_segment->setTrack(t);
}

void SegmentItem::setWidthToDurationRatio(unsigned int r)
{
    m_widthToDurationRatio = r;
}

void SegmentItem::setBarResolution(unsigned int r)
{
    m_barResolution = r;
}

unsigned int SegmentItem::getBarResolution()
{
    return m_barResolution;
}

void SegmentItem::setItemHeight(unsigned int h)
{
    m_itemHeight = h;
}

unsigned int SegmentItem::nbBarsToWidth(unsigned int nbBars)
{
    if (nbBars < m_barResolution) nbBars = m_barResolution;

    return nbBars * m_widthToDurationRatio / m_barResolution;
}

unsigned int SegmentItem::widthToNbBars(unsigned int width)
{
    return width * m_barResolution / m_widthToDurationRatio;
}

// Set this SegmentItem as selected/highlighted - we send
// in the QBrush we need at the same time
//
void
SegmentItem::setSelected(const bool &select, const QBrush &brush)
{
    setBrush(brush);
    m_selected = select;
}




unsigned int SegmentItem::m_widthToDurationRatio = 1;
unsigned int SegmentItem::m_barResolution = 1;
unsigned int SegmentItem::m_itemHeight = 10;



//////////////////////////////////////////////////////////////////////
//                SegmentCanvas
//////////////////////////////////////////////////////////////////////


SegmentCanvas::SegmentCanvas(int gridH, int gridV,
                           QCanvas& c, QWidget* parent,
                           const char* name, WFlags f) :
    QCanvasView(&c,parent,name,f),
    m_tool(0),
    m_grid(gridH, gridV),
    m_brush(RosegardenGUIColours::SegmentBlock),
    m_highlightBrush(RosegardenGUIColours::SegmentHighlightBlock),
    m_pen(RosegardenGUIColours::SegmentBorder),
    m_editMenu(new QPopupMenu(this))
{
    QWhatsThis::add(this, i18n("Segments Canvas - Create and manipulate your segments here"));

    SegmentItem::setWidthToDurationRatio(m_grid.hstep());
    SegmentItem::setItemHeight(m_grid.vstep());

}

SegmentCanvas::~SegmentCanvas()
{
}

void
SegmentCanvas::update()
{
    canvas()->update();
}

void
SegmentCanvas::setTool(ToolType t)
{
    kdDebug(KDEBUG_AREA) << "SegmentCanvas::setTool(" << t << ")"
                         << this << "\n";

    if (m_tool)
      delete m_tool;

    m_tool = 0;

    switch(t) {
    case Pencil:
        m_tool = new SegmentPencil(this);
        break;
    case Eraser:
        m_tool = new SegmentEraser(this);
        break;
    case Mover:
        m_tool = new SegmentMover(this);
        break;
    case Resizer:
        m_tool = new SegmentResizer(this);
        break;
    case Selector:
        m_tool = new SegmentSelector(this);
        break;
    default:
        KMessageBox::error(0, QString("SegmentCanvas::setTool() : unknown tool id %1").arg(t));
    }
}

SegmentItem*
SegmentCanvas::findPartClickedOn(QPoint pos)
{
    QCanvasItemList l=canvas()->collisions(pos);

    if (l.count()) {

        for (QCanvasItemList::Iterator it=l.begin(); it!=l.end(); ++it) {
            if (SegmentItem *item = dynamic_cast<SegmentItem*>(*it))
                return item;
        }

    }

    return 0;
}

void SegmentCanvas::contentsMousePressEvent(QMouseEvent* e)
{
    if (e->button() == LeftButton) { // delegate event handling to tool

        // ensure that we have a valid tool
        //
        if (m_tool)
            m_tool->handleMouseButtonPress(e);
        else
            kdDebug(KDEBUG_AREA) << "SegmentCanvas::contentsMousePressEvent() :"
                                 << this << " no tool\n";

    } else if (e->button() == RightButton) { // popup menu if over a part

        SegmentItem *item = findPartClickedOn(e->pos());

        if (item) {
            m_currentItem = item;
            //             kdDebug(KDEBUG_AREA) << "SegmentCanvas::contentsMousePressEvent() : edit m_currentItem = "
            //                                  << m_currentItem << endl;

            if (m_currentItem->getSegment()->getType() == 
                                             Rosegarden::Segment::Audio)
            {
                m_editMenu->clear();
                m_editMenu->insertItem(i18n("Edit Audio"),
                                       this, SLOT(onEditAudio()));

            }
            else
            {
                m_editMenu->clear();
                m_editMenu->insertItem(i18n("Edit as Notation"),
                                       this, SLOT(onEditNotation()));

                m_editMenu->insertItem(i18n("Edit as Matrix"),
                                       this, SLOT(onEditMatrix()));
            }

            m_editMenu->exec(QCursor::pos());
        }
    }
}

void SegmentCanvas::contentsMouseDoubleClickEvent(QMouseEvent* e)
{
    SegmentItem *item = findPartClickedOn(e->pos());

    if (item) {
        m_currentItem = item;
        // TODO : edit style should be user configurable

        if (m_currentItem->getSegment()->getType() == Rosegarden::Segment::Audio)
            emit editSegmentAudio(m_currentItem->getSegment());
        else
            emit editSegmentNotation(m_currentItem->getSegment());
    }
}

void SegmentCanvas::contentsMouseReleaseEvent(QMouseEvent *e)
{
    if (!m_tool) return;

    if (e->button() == LeftButton) m_tool->handleMouseButtonRelease(e);
}

void SegmentCanvas::contentsMouseMoveEvent(QMouseEvent* e)
{
    if (!m_tool) return;

    m_tool->handleMouseMove(e);
}

void
SegmentCanvas::wheelEvent(QWheelEvent *e)
{
    e->ignore();
}


void SegmentCanvas::clear()
{
    QCanvasItemList list = canvas()->allItems();
    QCanvasItemList::Iterator it = list.begin();
    for (; it != list.end(); ++it) {
	if ( *it )
	    delete *it;
    }
}

// Called when reading a music file or after we've finished recording
// a new Segment
//
SegmentItem*
SegmentCanvas::addPartItem(int x, int y, unsigned int nbBars)
{
    SegmentItem* newPartItem = new SegmentItem(x, y, nbBars, canvas());

    newPartItem->setPen(m_pen);
    newPartItem->setBrush(m_brush);
    newPartItem->setVisible(true);     
    newPartItem->setZ(1);           // Segment at Z=1, Pointer at Z=10 [rwb]

    return newPartItem;
}

void
SegmentCanvas::showRecordingSegmentItem(int x, int y,
                                        Rosegarden::Segment *segment)
{
}


void SegmentCanvas::onEditNotation()
{
    emit editSegmentNotation(m_currentItem->getSegment());
}

void SegmentCanvas::onEditMatrix()
{
    emit editSegmentMatrix(m_currentItem->getSegment());
}

void SegmentCanvas::onEditAudio()
{
    emit editSegmentAudio(m_currentItem->getSegment());
}



// Select a SegmentItem on the canvas according to a
// passed Segment pointer
//
//
void
SegmentCanvas::selectSegments(std::list<Rosegarden::Segment*> segments)
{
    SegmentSelector* selTool = dynamic_cast<SegmentSelector*>(m_tool);

    if (!selTool) return;

    std::list<Rosegarden::Segment*>::iterator segIt;
    QCanvasItemList itemList = canvas()->allItems();
    QCanvasItemList::Iterator it;

    // clear any SegmentItems currently selected
    //
    selTool->clearSelected();

    for (it = itemList.begin(); it != itemList.end(); ++it) {
        if ((*it)->rtti() == SegmentItem::SegmentItemRTTI) { 

            for (segIt = segments.begin(); segIt != segments.end(); segIt++) {

                if (dynamic_cast<SegmentItem*>(*it)->getSegment() == (*segIt)) {

                    selTool->selectSegmentItem(dynamic_cast<SegmentItem*>(*it));
                }
            }
        }
    }
}

// enter/exit selection add mode - this means that the SHIFT key
// (or similar) has been depressed and if we're in Select mode we
// can add Selections to the one we currently have
//
//
void
SegmentCanvas::setSelectAdd(const bool &value)
{
    SegmentSelector* selTool = dynamic_cast<SegmentSelector*>(m_tool);

    if (!selTool) return;

    selTool->setSegmentAdd(value);
}


// enter/exit selection copy mode - this means that the CTRL key
// (or similar) has been depressed and if we're in Select mode we
// can copy the current selection with a click and drag (overrides
// the default movement behaviour for selection).
//
//
void
SegmentCanvas::setSelectCopy(const bool &value)
{
    SegmentSelector* selTool = dynamic_cast<SegmentSelector*>(m_tool);

    if (!selTool) return;

    selTool->setSegmentCopy(value);
}


int
SegmentCanvas::SnapGrid::snappedSegmentSizeX(int x) const
{
    //!!! just as for snapX below
    int division = m_hstep;
    int base = x / division * division;
    if (x - base <= division / 2) return base;
    else return base + division;
}

int 
SegmentCanvas::SnapGrid::snapX(int x) const
{
//    return x / m_hdiv * m_hdiv; //!!! the following seems to me more responsive for dragging new segments:
    int division = m_hdiv;
    int base = x / division * division;
    if (x - base <= division / 2) return base;
    else return base + division;
}

int
SegmentCanvas::SnapGrid::snapY(int y) const
{
    return y / m_vstep * m_vstep;
}




//////////////////////////////////////////////////////////////////////
//                 Segment Tools
//////////////////////////////////////////////////////////////////////

SegmentTool::SegmentTool(SegmentCanvas* canvas)
    : m_canvas(canvas),
      m_currentItem(0)
{
    m_canvas->setCursor(Qt::arrowCursor);
}

SegmentTool::~SegmentTool()
{
}


//////////////////////////////
// SegmentPencil
//////////////////////////////

SegmentPencil::SegmentPencil(SegmentCanvas *c)
    : SegmentTool(c),
      m_newRect(false)
{
//    m_canvas->setCursor(Qt::ibeamCursor);

    connect(this, SIGNAL(addSegment(SegmentItem*)),
            c,    SIGNAL(addSegment(SegmentItem*)));
    connect(this, SIGNAL(deleteSegment(Rosegarden::Segment*)),
            c,    SIGNAL(deleteSegment(Rosegarden::Segment*)));
    connect(this, SIGNAL(setSegmentDuration(SegmentItem*)),
            c,    SIGNAL(updateSegmentDuration(SegmentItem*)));

    kdDebug(KDEBUG_AREA) << "SegmentPencil()\n";
}

void SegmentPencil::handleMouseButtonPress(QMouseEvent *e)
{
    m_newRect = false;
    m_currentItem = 0;

    // Check if we're clicking on a rect
    //
    SegmentItem *item = m_canvas->findPartClickedOn(e->pos());

    if (item) {
        // we are, so set currentItem to it
        // m_currentItem = item; // leave it alone
        return;

    } else { // we are not, so create one

        int gx = m_canvas->grid().snappedSegmentSizeX(e->pos().x()),
            gy = m_canvas->grid().snapY(e->pos().y());

        m_currentItem = new SegmentItem(gx, gy,
                                      SegmentItem::getBarResolution(),
                                      m_canvas->canvas());
        
        m_currentItem->setPen(m_canvas->pen());
        m_currentItem->setBrush(m_canvas->brush());
        m_currentItem->setVisible(true);
        m_currentItem->setZ(1);          // Segment at Z=1, Pointer at Z=10 [rwb]

        m_newRect = true;

        m_canvas->update();
    }

}

void SegmentPencil::handleMouseButtonRelease(QMouseEvent*)
{
    if (!m_currentItem) return;

    if (m_currentItem->width() < 0) { // segment was drawn from right to left
        double itemX = m_currentItem->x();
        double itemW = m_currentItem->width();
        kdDebug(KDEBUG_AREA) << "SegmentCanvas::contentsMouseReleaseEvent() : itemX = "
                             << itemX << " - width : " << itemW << endl;

        m_currentItem->setX(itemX + itemW);
        m_currentItem->setSize(int(-itemW), m_currentItem->height());

        kdDebug(KDEBUG_AREA) << "SegmentCanvas::contentsMouseReleaseEvent() after correction : itemX = "
                             << m_currentItem->x()
                             << " - width : " << m_currentItem->width()
                             << endl;
    }
    

    if (m_currentItem->width() == 0 && ! m_newRect) {

        kdDebug(KDEBUG_AREA) << "SegmentCanvas::contentsMouseReleaseEvent() : segment deleted"
                             << endl;
        emit deleteSegment(m_currentItem->getSegment());
        delete m_currentItem;
        m_canvas->canvas()->update();

    } else if (m_newRect && m_currentItem->width() > 0) {

        emit addSegment(m_currentItem);

    } else if (m_currentItem->width() > 0) {

        kdDebug(KDEBUG_AREA) << "SegmentCanvas::contentsMouseReleaseEvent() : shorten m_currentItem = "
                             << m_currentItem << endl;

	emit setSegmentDuration(m_currentItem);
    }

    m_currentItem = 0;
    m_newRect = false;
}

void SegmentPencil::handleMouseMove(QMouseEvent *e)
{
    if (m_currentItem) {

	int width = m_canvas->grid().snapX(e->pos().x()) -
	    m_currentItem->rect().x();
	if ((width >= 0) && (width < SegmentItem::getBarResolution())) {
	    width = SegmentItem::getBarResolution(); // cc
	}

	m_currentItem->setSize(width, m_currentItem->height());
	m_canvas->canvas()->update();
    }
}

//////////////////////////////
// SegmentEraser
//////////////////////////////

SegmentEraser::SegmentEraser(SegmentCanvas *c)
    : SegmentTool(c)
{
    m_canvas->setCursor(Qt::pointingHandCursor);

    connect(this, SIGNAL(deleteSegment(Rosegarden::Segment*)),
            c,    SIGNAL(deleteSegment(Rosegarden::Segment*)));

    kdDebug(KDEBUG_AREA) << "SegmentEraser()\n";
}

void SegmentEraser::handleMouseButtonPress(QMouseEvent *e)
{
    m_currentItem = m_canvas->findPartClickedOn(e->pos());
}

void SegmentEraser::handleMouseButtonRelease(QMouseEvent*)
{
    if (m_currentItem) emit deleteSegment(m_currentItem->getSegment());
    delete m_currentItem;
    m_canvas->canvas()->update();
    
    m_currentItem = 0;
}

void SegmentEraser::handleMouseMove(QMouseEvent*)
{
}

//////////////////////////////
// SegmentMover
//////////////////////////////

SegmentMover::SegmentMover(SegmentCanvas *c)
    : SegmentTool(c)
{
    m_canvas->setCursor(Qt::sizeAllCursor);

    connect(this, SIGNAL(updateSegmentTrackAndStartIndex(SegmentItem*)),
            c,    SIGNAL(updateSegmentTrackAndStartIndex(SegmentItem*)));

    kdDebug(KDEBUG_AREA) << "SegmentMover()\n";
}

void SegmentMover::handleMouseButtonPress(QMouseEvent *e)
{
    SegmentItem *item = m_canvas->findPartClickedOn(e->pos());

    if (item) {
        m_currentItem = item;
        return;
    }
}

void SegmentMover::handleMouseButtonRelease(QMouseEvent*)
{
    if (m_currentItem)
        emit updateSegmentTrackAndStartIndex(m_currentItem);

    m_currentItem = 0;
}

void SegmentMover::handleMouseMove(QMouseEvent *e)
{
    if (m_currentItem) {
        m_currentItem->setX(m_canvas->grid().snapX(e->pos().x()));
        m_currentItem->setY(m_canvas->grid().snapY(e->pos().y()));
        m_canvas->canvas()->update();
    }
}

//////////////////////////////
// SegmentResizer
//////////////////////////////

SegmentResizer::SegmentResizer(SegmentCanvas *c)
    : SegmentTool(c),
      m_edgeThreshold(10)
{
    m_canvas->setCursor(Qt::sizeHorCursor);

    connect(this, SIGNAL(deleteSegment(Rosegarden::Segment*)),
            c,    SIGNAL(deleteSegment(Rosegarden::Segment*)));

    connect(this, SIGNAL(setSegmentDuration(Rosegarden::Segment*)),
            c,    SIGNAL(updateSegmentDuration(Rosegarden::Segment*)));

    kdDebug(KDEBUG_AREA) << "SegmentResizer()\n";
}

void SegmentResizer::handleMouseButtonPress(QMouseEvent *e)
{
    SegmentItem* item = m_canvas->findPartClickedOn(e->pos());

    if (item && cursorIsCloseEnoughToEdge(item, e)) {
        m_currentItem = item;
    }
}

void SegmentResizer::handleMouseButtonRelease(QMouseEvent*)
{
    if (!m_currentItem) return;

    unsigned int newNbBars = m_currentItem->getItemNbBars();

    kdDebug(KDEBUG_AREA) << "SegmentResizer: set segment nb bars to "
                         << newNbBars << endl;
    
    emit setSegmentDuration(m_currentItem);

    m_currentItem = 0;
}

void SegmentResizer::handleMouseMove(QMouseEvent *e)
{
    if (!m_currentItem) return;

    // change width only

    m_currentItem->setSize(m_canvas->grid().snapX(e->pos().x()) - m_currentItem->rect().x(),
                           m_currentItem->rect().height());
    
    m_canvas->canvas()->update();
    
}

bool SegmentResizer::cursorIsCloseEnoughToEdge(SegmentItem* p, QMouseEvent* e)
{
    return ( abs(p->rect().x() + p->rect().width() - e->x()) < int(m_edgeThreshold));
}

//////////////////////////////
// SegmentSelector (bo!)
//////////////////////////////

SegmentSelector::SegmentSelector(SegmentCanvas *c)
    : SegmentTool(c), m_segmentAddMode(false), m_segmentCopyMode(false)

{
    kdDebug(KDEBUG_AREA) << "SegmentSelector()\n";

    connect(this, SIGNAL(updateSegmentTrackAndStartIndex(SegmentItem*)),
            c,    SIGNAL(updateSegmentTrackAndStartIndex(SegmentItem*)));
}

SegmentSelector::~SegmentSelector()
{
    clearSelected();
}

void
SegmentSelector::clearSelected()
{
    // For the moment only clear all selected from the list
    //
    std::list<SegmentItem*>::iterator it;
    for (it = m_selectedItems.begin();
         it != m_selectedItems.end();
         it++)
    {
        (*it)->setSelected(false, m_canvas->getSegmentBrush());
    }

    // now clear the list
    //
    m_selectedItems.clear();

    // clear the current item
    //
    m_currentItem = 0;

    // send update
    //
    m_canvas->canvas()->update();
}

void
SegmentSelector::handleMouseButtonPress(QMouseEvent *e)
{
    SegmentItem *item = m_canvas->findPartClickedOn(e->pos());

    // If we're in segmentAddMode then we don't clear the
    // selection list
    //
    if (!m_segmentAddMode)
       clearSelected();

    if (item)
    {
        m_currentItem = item;
        selectSegmentItem(m_currentItem);
        emit updateSegmentTrackAndStartIndex(m_currentItem);
    }

}

void
SegmentSelector::selectSegmentItem(SegmentItem *selectedItem)
{
    // If we're selecting a Segment through this method
    // then don't set the m_currentItem
    //
    selectedItem->setSelected(true, m_canvas->getHighlightBrush());
    m_selectedItems.push_back(selectedItem);
    m_canvas->canvas()->update();
}


// Don't need to do anything - for the moment we do this
// all on click, not release
//
void
SegmentSelector::handleMouseButtonRelease(QMouseEvent * /*e*/)
{
}

// In Select mode we implement movement on the Segment
// as movement _of_ the Segment - as with SegmentMover
//
void
SegmentSelector::handleMouseMove(QMouseEvent *e)
{
    if (m_currentItem) {

        if (m_segmentCopyMode)
        {
            std::cout << "Segment quick copy mode not implemented" << std::endl;
        }
        else
        {
            if (m_currentItem->isSelected())
            {
                std::list<SegmentItem*>::iterator it;

                for (it = m_selectedItems.begin();
                     it != m_selectedItems.end();
                     it++)
                {

                    (*it)->setX(m_canvas->grid().snapX(e->pos().x()));
                    (*it)->setY(m_canvas->grid().snapY(e->pos().y()));
                    m_canvas->canvas()->update();
                    emit updateSegmentTrackAndStartIndex(*it);
                }
            }
        }
    }
}

