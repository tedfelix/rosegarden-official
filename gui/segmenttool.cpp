// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2004
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
#include <qcursor.h>

#include <kmessagebox.h>

#include "segmenttool.h"

#include "Colour.h"
#include "SnapGrid.h"
#include "NotationTypes.h"

#include "segmentcommands.h" // for SegmentRecSet
#include "segmentcanvas.h"
#include "colours.h"

#include "rosegardengui.h"
#include "rosedebug.h"

using Rosegarden::TrackId;
using Rosegarden::timeT;
using Rosegarden::SnapGrid;
using Rosegarden::Note;
using Rosegarden::SegmentSelection;
using RosegardenGUIColours::convertColour;

//////////////////////////////////////////////////////////////////////
//                 Segment Tools
//////////////////////////////////////////////////////////////////////

SegmentToolBox::SegmentToolBox(SegmentCanvas* parent, RosegardenGUIDoc* doc)
    : BaseToolBox(parent),
      m_canvas(parent),
      m_doc(doc)
{
}

SegmentTool* SegmentToolBox::createTool(const QString& toolName)
{
    SegmentTool* tool = 0;

    QString toolNamelc = toolName.lower();
    
    if (toolNamelc == SegmentPencil::ToolName)

        tool = new SegmentPencil(m_canvas, m_doc);

    else if (toolNamelc == SegmentEraser::ToolName)

        tool = new SegmentEraser(m_canvas, m_doc);

    else if (toolNamelc == SegmentMover::ToolName)

        tool = new SegmentMover(m_canvas, m_doc);

    else if (toolNamelc == SegmentResizer::ToolName)

        tool = new SegmentResizer(m_canvas, m_doc);

    else if (toolNamelc == SegmentSelector::ToolName)

        tool = new SegmentSelector(m_canvas, m_doc);

    else if (toolNamelc == SegmentSplitter::ToolName)

        tool = new SegmentSplitter(m_canvas, m_doc);

    else if (toolNamelc == SegmentJoiner::ToolName)

        tool = new SegmentJoiner(m_canvas, m_doc);

    else {
        KMessageBox::error(0, QString("SegmentToolBox::createTool : unrecognised toolname %1 (%2)")
                           .arg(toolName).arg(toolNamelc));
        return 0;
    }

    m_tools.insert(toolName, tool);

    return tool;
}

SegmentTool* SegmentToolBox::getTool(const QString& toolName)
{
    return dynamic_cast<SegmentTool*>(BaseToolBox::getTool(toolName));
}


// TODO : relying on doc->parent being a KMainWindow sucks
SegmentTool::SegmentTool(SegmentCanvas* canvas, RosegardenGUIDoc *doc)
    : BaseTool("segment_tool_menu", dynamic_cast<KMainWindow*>(doc->parent())->factory(), canvas),
      m_canvas(canvas),
      m_currentItem(0),
      m_doc(doc)
{

    connect(this,     SIGNAL(selectedSegments(const Rosegarden::SegmentSelection &)),
            m_canvas, SIGNAL(selectedSegments(const Rosegarden::SegmentSelection &)));
}

SegmentTool::~SegmentTool()
{}

void SegmentTool::ready()
{
    m_canvas->viewport()->setCursor(Qt::arrowCursor);
}

void 
SegmentTool::handleRightButtonPress(QMouseEvent *e)
{
    if (m_currentItem) // mouse button is pressed for some tool
	return;
    
    RG_DEBUG << "SegmentTool::handleRightButtonPress()\n";

    QWMatrix matrix = m_canvas->worldMatrix().invert();
    QPoint tPos = matrix.map(e->pos());

    SegmentItem *item = m_canvas->findSegmentClickedOn(tPos);

    if (item) {
        m_currentItem = item;
        if (!item->isSelected() && (item->getSegment() != 0)) {

            SegmentSelector* selector = dynamic_cast<SegmentSelector*>(getToolBox()->getTool("segmentselector"));
            selector->clearSelected();
            selector->slotSelectSegmentItem(item);
            emit selectedSegments(selector->getSelectedSegments());
            
        }
    }
    
    showMenu();
    m_currentItem = 0;
}

void
SegmentTool::createMenu()
{
    RG_DEBUG << "SegmentTool::createMenu()\n";

    RosegardenGUIApp *app =
        dynamic_cast<RosegardenGUIApp*>(m_doc->parent());

    if (app) {
        m_menu = static_cast<QPopupMenu*>
            //(app->factory()->container("segment_tool_menu", app));
            (m_parentFactory->container("segment_tool_menu", app));

        if (!m_menu) {
            RG_DEBUG << "SegmentTool::createMenu() failed\n";
        }
    } else {
        RG_DEBUG << "SegmentTool::createMenu() failed: !app\n";
    }
}

void
SegmentTool::addCommandToHistory(KCommand *command)
{
    m_doc->getCommandHistory()->addCommand(command);
}

SegmentToolBox* SegmentTool::getToolBox()
{
    return m_canvas->getToolBox();
}

//////////////////////////////
// SegmentPencil
//////////////////////////////

SegmentPencil::SegmentPencil(SegmentCanvas *c, RosegardenGUIDoc *d)
    : SegmentTool(c, d),
      m_newRect(false),
      m_track(0),
      m_startTime(0),
      m_endTime(0)
{
    RG_DEBUG << "SegmentPencil()\n";
}

void SegmentPencil::ready()
{
    m_canvas->viewport()->setCursor(Qt::ibeamCursor);
}

void SegmentPencil::handleMouseButtonPress(QMouseEvent *e)
{
    if (e->button() == RightButton) return;
    
    m_newRect = false;
    m_currentItem = 0;

    QWMatrix matrix = m_canvas->worldMatrix().invert();
    QPoint tPos = matrix.map(e->pos());

    // Check if we're clicking on a rect
    //
    SegmentItem *item = m_canvas->findSegmentClickedOn(tPos);

    if (item) return;

    m_canvas->setSnapGrain(false);

    int trackPosition = m_canvas->grid().getYBin(tPos.y());
    Rosegarden::Track *t = m_doc->getComposition().getTrackByPosition(trackPosition);

    if (!t) return;
    
    TrackId track = t->getId();

    // Don't do anything if the user clicked beyond the track buttons
    //
    if (track >= TrackId(m_doc->getComposition().getNbTracks())) return;

    timeT time = m_canvas->grid().snapX(tPos.x(), SnapGrid::SnapLeft);
    timeT duration = m_canvas->grid().getSnapTime(double(tPos.x()));
    if (duration == 0) duration = Note(Note::Shortest).getDuration();
    
    m_currentItem = m_canvas->
        addSegmentItem(trackPosition, time, time + duration);
    m_newRect = true;
    
    m_canvas->slotUpdate();

}

void SegmentPencil::handleMouseButtonRelease(QMouseEvent* e)
{
    if (e->button() == RightButton) return;

    if (!m_currentItem) return;
    m_currentItem->normalize();

    if (m_newRect) {

        Rosegarden::Track *track = m_doc->getComposition().
            getTrackByPosition(m_currentItem->getTrackPosition());

        SegmentInsertCommand *command =
            new SegmentInsertCommand(m_doc,
                                     track->getId(),
                                     m_currentItem->getStartTime(),
                                     m_currentItem->getEndTime());

	delete m_currentItem;
	m_currentItem = 0;
	m_newRect = false;

        addCommandToHistory(command);

	// add the SegmentItem by hand, instead of allowing the usual
	// update mechanism to spot it.  This way we can select the
	// segment as we add it; otherwise we'd have no way to know
	// that the segment was created by this tool rather than by
	// e.g. a simple file load
	SegmentSelector* selector = dynamic_cast<SegmentSelector*>
	    (getToolBox()->getTool("segmentselector"));
	Rosegarden::Segment *segment = command->getSegment();
	SegmentItem *item = m_canvas->addSegmentItem(segment);

	Rosegarden::SegmentSelection selection;
	selection.insert(segment);
	selector->clearSelected();
	emit selectedSegments(selection);
	selector->slotSelectSegmentItem(item);

    } else {

	delete m_currentItem;
	m_currentItem = 0;
	m_newRect = false;
    }
}

int SegmentPencil::handleMouseMove(QMouseEvent *e)
{
    if (!m_currentItem) return NoFollow;

    m_canvas->setSnapGrain(false);

    QWMatrix matrix = m_canvas->worldMatrix().invert();
    QPoint tPos = matrix.map(e->pos());

    SnapGrid::SnapDirection direction = SnapGrid::SnapRight;
    if (tPos.x() < m_currentItem->x()) direction = SnapGrid::SnapLeft;

    timeT snap = m_canvas->grid().getSnapTime(double(tPos.x()));
    if (snap == 0) snap = Note(Note::Shortest).getDuration();

    timeT time = m_canvas->grid().snapX(tPos.x(), direction);
    timeT startTime = m_currentItem->getStartTime();

    if (time >= startTime) {
	if ((time - startTime) < snap) {
	    time = startTime + snap;
	}
    } else {
	if ((startTime - time) < snap) {
	    time = startTime - snap;
	}
    }

    if (direction == SnapGrid::SnapLeft) {
	time += std::max(m_currentItem->getEndTime() -
			 m_currentItem->getStartTime(), timeT(0));
    }

    m_currentItem->setEndTime(time);

    m_canvas->slotUpdate();

    return FollowHorizontal;
}

//////////////////////////////
// SegmentEraser
//////////////////////////////

SegmentEraser::SegmentEraser(SegmentCanvas *c, RosegardenGUIDoc *d)
    : SegmentTool(c, d)
{
    RG_DEBUG << "SegmentEraser()\n";
}

void SegmentEraser::ready()
{
    m_canvas->viewport()->setCursor(Qt::pointingHandCursor);
}

void SegmentEraser::handleMouseButtonPress(QMouseEvent *e)
{
    QWMatrix matrix = m_canvas->worldMatrix().invert();
    QPoint tPos = matrix.map(e->pos());

    m_currentItem = m_canvas->findSegmentClickedOn(tPos);
}

void SegmentEraser::handleMouseButtonRelease(QMouseEvent*)
{
    if (m_currentItem)
    {
        addCommandToHistory(
                new SegmentEraseCommand(m_currentItem->getSegment()));
    }

    m_canvas->canvas()->update();
    
    m_currentItem = 0;
}

int SegmentEraser::handleMouseMove(QMouseEvent*)
{
    return NoFollow;
}

//////////////////////////////
// SegmentMover
//////////////////////////////

SegmentMover::SegmentMover(SegmentCanvas *c, RosegardenGUIDoc *d)
    : SegmentTool(c, d),
    m_foreGuide(new QCanvasRectangle(m_canvas->canvas())),
    m_topGuide(new QCanvasRectangle(m_canvas->canvas()))
{
    m_foreGuide->setPen(RosegardenGUIColours::MovementGuide);
    m_foreGuide->setBrush(RosegardenGUIColours::MovementGuide);
    m_foreGuide->hide();

    m_topGuide->setPen(RosegardenGUIColours::MovementGuide);
    m_topGuide->setBrush(RosegardenGUIColours::MovementGuide);
    m_topGuide->hide();

    RG_DEBUG << "SegmentMover()\n";
}

void SegmentMover::ready()
{
    m_canvas->viewport()->setCursor(Qt::sizeAllCursor);
}

void SegmentMover::handleMouseButtonPress(QMouseEvent *e)
{
    QWMatrix matrix = m_canvas->worldMatrix().invert();
    QPoint tPos = matrix.map(e->pos());

    SegmentItem *item = m_canvas->findSegmentClickedOn(tPos);
    SegmentSelector* selector = dynamic_cast<SegmentSelector*>
            (getToolBox()->getTool("segmentselector"));

    if (item) {

        m_currentItem = item;
	m_currentItemStartX = item->x();
	m_clickPoint = tPos;

        m_foreGuide->setX(int(m_canvas->grid().getRulerScale()->
                          getXForTime(item->getSegment()->getStartTime())) - 2);
        m_foreGuide->setY(0);
        m_foreGuide->setZ(10);
        m_foreGuide->setSize(2, m_canvas->canvas()->height());

        m_topGuide->setSize(m_canvas->canvas()->width(), 2);
        m_topGuide->setX(0);
        m_topGuide->setY(int(m_canvas->grid().getYBinCoordinate(
                              item->getSegment()->getTrack())));
        m_topGuide->setZ(10);

        m_foreGuide->show();
        m_topGuide->show();

        if (selector)
        {
            selector->slotSelectSegmentItem(item);
        }

	m_passedInertiaEdge = false;
        // Don't update until the move
        //
        //m_canvas->canvas()->update();
    } else {
        // check for addmode - clear the selection if not
        selector->clearSelected();
    }

}

void SegmentMover::handleMouseButtonRelease(QMouseEvent*)
{
    if (m_currentItem)
    {
        /*
        Rosegarden::Composition &comp = m_doc->getComposition();

        Rosegarden::Track *track = comp.getTrackByPosition(
                m_currentItem->getTrackPosition());
                */

	bool haveChange = false;

        SegmentSelector* 
            selector = dynamic_cast<SegmentSelector*>
            (getToolBox()->getTool("segmentselector"));

        if (selector)
            m_selectedItems = selector->getSegmentItemList();
        else
           return;

	SegmentReconfigureCommand *command =
	    new SegmentReconfigureCommand
	    (m_selectedItems->size() == 1 ? i18n("Move Segment") :
	                                   i18n("Move Segments"));

	SegmentItemList::iterator it;
        SegmentSelection newSelection;
	for (it = m_selectedItems->begin();
	     it != m_selectedItems->end();
	     it++)
	{

	    SegmentItem *item = it->second;

            Rosegarden::Composition &comp = m_doc->getComposition();
            Rosegarden::Track *track = 
                comp.getTrackByPosition(item->getTrackPosition());

            Rosegarden::TrackId trackId = track->getId();

	    if (item->getStartTime() != item->getSegment()->getStartTime() ||
		item->getEndTime()   != item->getSegment()->getEndMarkerTime() 
                || trackId              != item->getSegment()->getTrack()) {

		command->addSegment(item->getSegment(),
				    item->getStartTime(),
				    item->getEndTime(),
				    trackId);

                newSelection.insert(item->getSegment());
		haveChange = true;
	    }
	}

        addCommandToHistory(command);
        //m_currentItem->showRepeatRect(true);

        m_foreGuide->hide();
        m_topGuide->hide();

        selector->clearSelected();
        for (SegmentSelection::const_iterator nIt = newSelection.begin();
                nIt != newSelection.end(); ++nIt)
        {
            selector->addToSelection(*nIt);
            selector->slotSelectSegmentItem(m_canvas->getSegmentItem(*nIt));
        }
    }

    m_currentItem = 0;
}

int SegmentMover::handleMouseMove(QMouseEvent *e)
{
    /*
    if (m_currentItem) {

	m_canvas->setSnapGrain(true);

        QWMatrix matrix = m_canvas->worldMatrix().invert();
        QPoint tPos = matrix.map(e->pos());

        // prevent item from being dragged out of screen.
        //
	int newX = tPos.x() - m_clickPoint.x() + int(m_currentItemStartX);

        RG_DEBUG << "SegmentMover::handleMouseMove : newX = "
                 << newX
                 << " - canvas width : " << m_canvas->canvas()->width()
                 << endl;

        if ((newX + m_currentItem->width()) < 20) {
            newX = 20 - m_currentItem->width();
        }
        
        if (newX >= m_canvas->canvas()->width() - 20)
            newX = m_canvas->canvas()->width() - 20;

        int newY = tPos.y();
        if (newY < 0) newY = 0;
        if ((newY + m_currentItem->height()) >= m_canvas->canvas()->height())
            newY = m_canvas->canvas()->height() - m_currentItem->height();

	timeT newStartTime = m_canvas->grid().snapX(newX);

        // We shouldn't have the initial ratio reset around these modifiers
        // otherwise the rulers refresh to the wrong Ratio
        //
	m_currentItem->setEndTime(m_currentItem->getEndTime() + newStartTime -
				  m_currentItem->getStartTime());
	m_currentItem->setStartTime(newStartTime);

	TrackId track = m_canvas->grid().getYBin(newY);

        int nbTracks = m_doc->getComposition().getNbTracks();

        if (track >= ((unsigned int)nbTracks)) {
            // Make sure the item isn't dragged to below the last track
            track = nbTracks - 1;
        }

        // Don't use proper TrackPosition yet - just visual position
        // until the release.
        //
        m_currentItem->setTrackPosition(track);

        m_foreGuide->setX(int(m_canvas->grid().getRulerScale()->
                            getXForTime(newStartTime)) - 2);

        m_topGuide->setY(m_canvas->grid().getYBinCoordinate(track));

        m_canvas->canvas()->update();

	return FollowHorizontal | FollowVertical;
    }
    */

    m_canvas->setSnapGrain(true);

    if (m_currentItem && m_currentItem->isSelected())
    {
	SegmentItemList::iterator it;
        int guideX = 0;
        int guideY = 0;

        SegmentSelector* 
            selector = dynamic_cast<SegmentSelector*>
            (getToolBox()->getTool("segmentselector"));

        if (selector)
            m_selectedItems = selector->getSegmentItemList();
        else
            return NoFollow;
	
	for (it = m_selectedItems->begin();
	     it != m_selectedItems->end();
	     it++)
	{
            it->second->showRepeatRect(false);

	    int x = e->pos().x() - m_clickPoint.x(),
		y = e->pos().y() - m_clickPoint.y();

	    const int inertiaDistance = m_canvas->grid().getYSnap() / 3;
	    if (!m_passedInertiaEdge &&
		(x < inertiaDistance && x > -inertiaDistance) &&
		(y < inertiaDistance && y > -inertiaDistance)) {
		return false;
	    } else {
		m_passedInertiaEdge = true;
	    }


	    timeT newStartTime = m_canvas->grid().snapX(it->first.x() + x);
	    it->second->setEndTime(it->second->getEndTime() + newStartTime -
				   it->second->getStartTime());
	    it->second->setStartTime(newStartTime);

	    TrackId track;
            int newY=it->first.y() + y;
            // Make sure we don't set a non-existing track
            if (newY < 0) { newY = 0; }
            track = m_canvas->grid().getYBin(newY);

	    RG_DEBUG << "SegmentMover::handleMouseMove: orig y " 
                     << it->first.y()
		     << ", dy " << y << ", newY " << newY 
                     << ", track " << track << endl;

            // Make sure we don't set a non-existing track (c'td)
            // TODO: make this suck less. Either the tool should
            // not allow it in the first place, or we automatically
            // create new tracks - might make undo very tricky though
            //
            if (track >= TrackId(m_doc->getComposition().getNbTracks()))
                track  = TrackId(m_doc->getComposition().getNbTracks() - 1);

            /*
            if (it == m_selectedItems->begin())
            {
                guideX = int(m_canvas->grid().getRulerScale()->
                    getXForTime(newStartTime));

                guideY = m_canvas->grid().getYBinCoordinate(track);
            }
            else
            {
                if (x < guideX)
                    guideX = int(m_canvas->grid().getRulerScale()->
                        getXForTime(newStartTime));

                if (y < guideY)
                    guideY = m_canvas->grid().getYBinCoordinate(track);
            }
            */

            // This is during a "mover" so don't use the normalised (i.e.
            // proper) TrackPosition value yet.
            //
	    it->second->setTrackPosition(track);
	}

        guideX = int(m_currentItem->x());
        guideY = int(m_currentItem->y());

        m_foreGuide->setX(guideX - 2);
        m_topGuide->setY(guideY - 2);
	m_canvas->canvas()->update();

	return FollowHorizontal | FollowVertical;
    }

    return NoFollow;
}

//////////////////////////////
// SegmentResizer
//////////////////////////////

SegmentResizer::SegmentResizer(SegmentCanvas *c, RosegardenGUIDoc *d,
			       int edgeThreshold)
    : SegmentTool(c, d),
      m_edgeThreshold(edgeThreshold)
{
    RG_DEBUG << "SegmentResizer()\n";
}

void SegmentResizer::ready()
{
    m_canvas->viewport()->setCursor(Qt::sizeHorCursor);
}

void SegmentResizer::handleMouseButtonPress(QMouseEvent *e)
{
    RG_DEBUG << "SegmentResizer::handleMouseButtonPress" << endl;
    SegmentSelector* selector = dynamic_cast<SegmentSelector*>
            (getToolBox()->getTool("segmentselector"));
    if (selector) selector->clearSelected();

    QWMatrix matrix = m_canvas->worldMatrix().invert();
    QPoint tPos = matrix.map(e->pos());

    SegmentItem* item = m_canvas->findSegmentClickedOn(tPos);

    if (item) {
        RG_DEBUG << "SegmentResizer::handleMouseButtonPress - got item" << endl;
        m_currentItem = item;
        if (selector) selector->slotSelectSegmentItem(item);
    }
}

void SegmentResizer::handleMouseButtonRelease(QMouseEvent*)
{
    if (!m_currentItem) return;
    m_currentItem->normalize();

    // normalisation may mean start time has changed as well as duration
    SegmentReconfigureCommand *command =
                new SegmentReconfigureCommand("Resize Segment");

    Rosegarden::Composition &comp = m_doc->getComposition();
    Rosegarden::Track *track =
        comp.getTrackByPosition(m_currentItem->getTrackPosition());

    command->addSegment(m_currentItem->getSegment(),
                        m_currentItem->getStartTime(),
                        m_currentItem->getEndTime(),
                        track->getId());
    addCommandToHistory(command);

    // update preview
    if (m_currentItem->getPreview())
        m_currentItem->getPreview()->setPreviewCurrent(false);

    m_canvas->canvas()->update();

    m_currentItem = 0;
}

int SegmentResizer::handleMouseMove(QMouseEvent *e)
{
    if (!m_currentItem) return NoFollow;

    m_canvas->setSnapGrain(true);

    QWMatrix matrix = m_canvas->worldMatrix().invert();
    QPoint tPos = matrix.map(e->pos());

    timeT time = m_canvas->grid().snapX(tPos.x());
    timeT duration = time - m_currentItem->getStartTime();

    timeT snap = m_canvas->grid().getSnapTime(double(tPos.x()));

    if (snap == 0) snap = Note(Note::Shortest).getDuration();

    if ((duration > 0 && duration <  snap) ||
	(duration < 0 && duration > -snap)) {
	m_currentItem->setEndTime((duration < 0 ? -snap : snap) +
				  m_currentItem->getStartTime());
    } else {
	m_currentItem->setEndTime(duration +
				  m_currentItem->getStartTime());
    }

    // update preview
    if (m_currentItem->getPreview())
        m_currentItem->getPreview()->setPreviewCurrent(false);

    m_canvas->canvas()->update();

    return FollowHorizontal;
}

bool SegmentResizer::cursorIsCloseEnoughToEdge(SegmentItem* p, const QPoint &coord,
					       int edgeThreshold)
{
    return (abs(p->rect().x() + p->rect().width() - coord.x()) < edgeThreshold);
}

//////////////////////////////
// SegmentSelector (bo!)
//////////////////////////////

SegmentSelector::SegmentSelector(SegmentCanvas *c, RosegardenGUIDoc *d)
    : SegmentTool(c, d),
      m_segmentAddMode(false),
      m_segmentCopyMode(false),
      m_segmentQuickCopyDone(false),
      m_dispatchTool(0),
      m_foreGuide(new QCanvasRectangle(m_canvas->canvas())),
      m_topGuide(new QCanvasRectangle(m_canvas->canvas()))
{
    RG_DEBUG << "SegmentSelector()\n";

    m_foreGuide->setPen(RosegardenGUIColours::MovementGuide);
    m_foreGuide->setBrush(RosegardenGUIColours::MovementGuide);
    m_foreGuide->hide();

    m_topGuide->setPen(RosegardenGUIColours::MovementGuide);
    m_topGuide->setBrush(RosegardenGUIColours::MovementGuide);
    m_topGuide->hide();
}

SegmentSelector::~SegmentSelector()
{
    //delete m_dispatchTool;
}

void SegmentSelector::stow()
{
    // don't clear selection, it's nice to still have it when you
    // switch back to the selector tool
    //
    // clearSelected();
}

void
SegmentSelector::removeFromSelection(Rosegarden::Segment *segment)
{
    for (SegmentItemList::iterator i = m_selectedItems.begin();
	 i != m_selectedItems.end(); ++i) {
	if (i->second->getSegment() == segment) {

//             RG_DEBUG << "SegmentSelector::removeFromSelection() SegmentItem = "
//                      << i->second << endl;

            i->second->disconnect(this); // disconnect the item's 'destroyed' signal
	    m_selectedItems.erase(i);
	    return;
	}
    }
}

void
SegmentSelector::addToSelection(Rosegarden::Segment *segment)
{
    SegmentItem *item = m_canvas->getSegmentItem(segment);
    if (!item) return;

    addToSelection(item); 
}

void
SegmentSelector::addToSelection(SegmentItem* item)
{
    RG_DEBUG << "SegmentSelector::addToSelection() SegmentItem = "
             << item << endl;

    // Check that the segment isn't already selected
    for (SegmentItemList::iterator i = m_selectedItems.begin();
	 i != m_selectedItems.end(); ++i) {
//         RG_DEBUG << "SegmentSelector::addToSelection() SegmentItem already in selection\n";
	if (i->second == item) {
	    i->first = QPoint(int(item->x()), int(item->y()));
	    return;
	}
    }

    m_selectedItems.push_back
	(SegmentItemPair(QPoint(int(item->x()), int(item->y())), item));

    connect(item, SIGNAL(destroyed(QObject*)),
            this, SLOT(slotDestroyedSegmentItem(QObject*)));
}

void
SegmentSelector::clearSelected()
{
    // For the moment only clear all selected
    //
    SegmentItemList::iterator it;
    for (it = m_selectedItems.begin();
         it != m_selectedItems.end();
         it++)
    {
        it->second->disconnect(this);
        it->second->setSelected(false, 
          convertColour(m_doc->getComposition().getSegmentColourMap()
                        .getColourByIndex(it->second->getSegment()->getColourIndex())));
    }

    // now clear the selection
    //
    m_selectedItems.clear();

    RG_DEBUG << "SegmentSelector::clearSelected()" << endl;

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
    RG_DEBUG << "SegmentSelector::handleMouseButtonPress" << endl;
    QWMatrix matrix = m_canvas->worldMatrix().invert();
    QPoint tPos = matrix.map(e->pos());
    SegmentItem *item = m_canvas->findSegmentClickedOn(tPos);

    // If we're in segmentAddMode or not clicking on an item then we don't 
    // clear the selection vector.  If we're clicking on an item and it's 
    // not in the selection - then also clear the selection.
    //
    if ((!m_segmentAddMode && !item) || 
        (!m_segmentAddMode && !(item->isSelected()))) {
    //if (!item || (!m_segmentAddMode && !(item->isSelected()))) {
        clearSelected();
    }

    if (item) {
	
        // Ten percent of the width of the SegmentItem
        //
        int threshold = int(float(item->width()) * 0.15);
        if (threshold  == 0) threshold = 1;
        if (threshold > 10) threshold = 10;

	if (!m_segmentAddMode &&
	    SegmentResizer::cursorIsCloseEnoughToEdge(item, tPos, threshold)) {
            SegmentResizer* resizer = 
                dynamic_cast<SegmentResizer*>(getToolBox()->
                    getTool(SegmentResizer::ToolName));

            resizer->setEdgeThreshold(threshold);

            // For the moment we only allow resizing of a single segment
            // at a time.
            //
            clearSelected();
            slotSelectSegmentItem(item);

	    m_dispatchTool = resizer;
            
	    m_dispatchTool->handleMouseButtonPress(e);
	    return;
	}

        if (item->getSegment()) {
            // Moving
            //
            m_currentItem = item;
            m_clickPoint = e->pos();
            m_clickPoint = tPos;

            slotSelectSegmentItem(m_currentItem);

            m_foreGuide->setX(int(m_canvas->grid().getRulerScale()->
                                  getXForTime(item->getSegment()->
                                      getStartTime())) -2);
            m_foreGuide->setY(0);
            m_foreGuide->setZ(10);
            m_foreGuide->setSize(2, m_canvas->canvas()->height());

            m_topGuide->setX(0);
            m_topGuide->setY(int(m_canvas->grid().getYBinCoordinate(
                            item->getSegment()->getTrack())));
            m_topGuide->setZ(10);
            m_topGuide->setSize(m_canvas->canvas()->width(), 2);

            m_foreGuide->show();
            m_topGuide->show();

            // Don't update until the move - lazy way of making sure the
            // guides don't flash on while we're double clicking
            //
            //m_canvas->canvas()->update();
        }
        
    } else {


        // Add on middle button - bounding box on rest
        //
	if (e->button() == MidButton) {
	    m_dispatchTool =  getToolBox()->getTool(SegmentPencil::ToolName);

            if (m_dispatchTool)
                m_dispatchTool->handleMouseButtonPress(e);

	    return;
	}
        else {
            // do a bounding box
            QCanvasRectangle *rect  = m_canvas->getSelectionRectangle();

            if (rect) {

                rect->show();
                rect->setX(tPos.x());
                rect->setY(tPos.y());
                rect->setSize(0, 0);
            }
        }
    }
 
    // Tell the RosegardenGUIView that we've selected some new Segments -
    // when the list is empty we're just unselecting.
    //
    emit selectedSegments(getSelectedSegments());

    m_passedInertiaEdge = false;
}

SegmentSelection
SegmentSelector::getSelectedSegments()
{
    SegmentSelection segments;
    SegmentItemList::iterator it;

    for (it = m_selectedItems.begin();
         it != m_selectedItems.end();
         ++it)
    {
        segments.insert(it->second->getSegment());
    }

    return segments;
}


void
SegmentSelector::slotSelectSegmentItem(SegmentItem *selectedItem)
{
    if (!selectedItem || !selectedItem->getSegment()) return; // yes, this can happen
    // if the user draws a segment with the middle button (using the
    // selector tool) and left-clicks while maintaining the middle
    // button down - reported by Vladimir Savic <vlada@rockforums.net>
    // who really deserves credit for finding this one :-)
    
    // If we're selecting a Segment through this method
    // then don't set the m_currentItem
    //
    selectedItem->setSelected(true, 
        convertColour(m_doc->getComposition().getSegmentColourMap()
        .getColourByIndex(selectedItem->getSegment()->getColourIndex())).dark(200));
    addToSelection(selectedItem);
    m_canvas->canvas()->update();
}

void
SegmentSelector::slotDestroyedSegmentItem(QObject *destroyedObject)
{
    // doesn't work, because the signal is emitted from the QObject's dtor
    //
//  SegmentItem* destroyedItem = dynamic_cast<SegmentItem*>(destroyedObject);

    RG_DEBUG << "SegmentSelector::slotDestroyedSegmentItem : "
             << "destroyedObject : " << destroyedObject << endl;

    for (SegmentItemList::iterator i = m_selectedItems.begin();
         i != m_selectedItems.end(); ++i) {
        
        if (i->second == destroyedObject) {
            RG_DEBUG << "SegmentSelector::slotDestroyedSegmentItem : "
                     << "found destroyedObject\n";
            m_selectedItems.erase(i);
            return;
        }
    }
    
    RG_DEBUG << "SegmentSelector::slotDestroyedSegmentItem : WARNING - "
             << "destroyedObject not found - this is probably a bug\n";
}


void
SegmentSelector::handleMouseButtonRelease(QMouseEvent *e)
{
    // Hide guides
    //
    m_foreGuide->hide();
    m_topGuide->hide();
    m_canvas->canvas()->update();

    if (m_dispatchTool) {
	m_dispatchTool->handleMouseButtonRelease(e);
	//delete m_dispatchTool;
	m_dispatchTool = 0;
	m_canvas->viewport()->setCursor(Qt::arrowCursor);
	return;
    }

    if (!m_currentItem) {
        QCanvasRectangle *rect  = m_canvas->getSelectionRectangle();

        if (rect) {
            rect->hide();
	    m_canvas->canvas()->update();
        }
        return;
    }

    /*
    RG_DEBUG << "SegmentSelector::handleMouseButtonRelease - "
             << "selection size = " << m_selectedItems.size()
             << endl;
             */

    m_canvas->viewport()->setCursor(Qt::arrowCursor);

    if (m_currentItem->isSelected())
    {
	SegmentItemList::iterator it;

	bool haveChange = false;

	SegmentReconfigureCommand *command =
	    new SegmentReconfigureCommand
	    (m_selectedItems.size() == 1 ? i18n("Move Segment") :
	                                   i18n("Move Segments"));

        SegmentSelection newSelection;

	for (it = m_selectedItems.begin();
	     it != m_selectedItems.end();
	     it++)
	{

	    SegmentItem *item = it->second;

/*
            if (item->getSegment()->isRepeating())
                item->showRepeatRect(true);
*/

            Rosegarden::Composition &comp = m_doc->getComposition();
            Rosegarden::Track *track = 
                comp.getTrackByPosition(item->getTrackPosition());

            Rosegarden::TrackId trackId = track->getId();

	    if (item->getStartTime() != item->getSegment()->getStartTime() ||
		item->getEndTime()   != item->getSegment()->getEndMarkerTime() 
                || trackId              != item->getSegment()->getTrack()) {

		command->addSegment(item->getSegment(),
				    item->getStartTime(),
				    item->getEndTime(),
				    trackId);
		haveChange = true;
	    }
            newSelection.insert(item->getSegment());
	}

	if (haveChange) addCommandToHistory(command);

        /*
        RG_DEBUG << "SegmentSelector::handleMouseButtonRelease - "
                 << "clearing selection" << endl;
                 */

        // Update selection
        clearSelected();

        for (SegmentSelection::const_iterator nIt = newSelection.begin();
                nIt != newSelection.end(); ++nIt)
        {
            addToSelection(*nIt);
            slotSelectSegmentItem(m_canvas->getSegmentItem(*nIt));
        }

	m_canvas->canvas()->update();

	emit selectedSegments(newSelection);
    }
    
    // if we've just finished a quick copy then drop the Z level back
    if (m_segmentQuickCopyDone)
    {
        m_segmentQuickCopyDone = false;
//        m_currentItem->setZ(2); // see SegmentItem::setSelected  --??
    }

    m_currentItem = 0;
}

// In Select mode we implement movement on the Segment
// as movement _of_ the Segment - as with SegmentMover
//
int
SegmentSelector::handleMouseMove(QMouseEvent *e)
{
    if (m_dispatchTool) {
	return m_dispatchTool->handleMouseMove(e);
    }

    QWMatrix matrix = m_canvas->worldMatrix().invert();
    QPoint tPos = matrix.map(e->pos());

    if (!m_currentItem)  {

	RG_DEBUG << "SegmentSelector::handleMouseMove: no current item" << endl;

        // do a bounding box
        QCanvasRectangle *selectionRect  = m_canvas->getSelectionRectangle();

        if (selectionRect) {
            selectionRect->show();

            // same as for notation view
            int w = int(tPos.x() - selectionRect->x());
            int h = int(tPos.y() - selectionRect->y());
            if (w > 0) ++w; else --w;
            if (h > 0) ++h; else --h;

            // Translate these points
            //
            selectionRect->setSize(w, h);

	    m_canvas->canvas()->update();

            // Get collisions and do selection (true for exact collisions)
            //
            QCanvasItemList l = selectionRect->collisions(true);

            // selection management
            SegmentSelection oldSelection = getSelectedSegments();
            SegmentSelection newSelection;

            int segCount = 0;

            if (l.count()) {
                for (QCanvasItemList::Iterator it=l.begin(); it!=l.end(); ++it)
                {
                    if (SegmentItem *item = dynamic_cast<SegmentItem*>(*it))
                    {
                        if (m_segmentAddMode)
                        {
                            slotSelectSegmentItem(item);
                            addToSelection(item);
                        }
                        else
                        {
                            segCount++;
                            slotSelectSegmentItem(item);
                            newSelection.insert(item->getSegment());
                        }
                    }
                }
            }

            if (m_segmentAddMode)
            { 
                emit selectedSegments(getSelectedSegments());
                return FollowHorizontal | FollowVertical;
            }

            // Check for unselected items with this piece of crap
            //
            bool found = false;

            for (SegmentSelection::const_iterator oIt = oldSelection.begin();
                  oIt != oldSelection.end(); oIt++)
            {
                found = false;
                for (SegmentSelection::const_iterator nIt = newSelection.begin();
                     nIt != newSelection.end(); nIt++)
                {
                    if (*oIt == *nIt)
                    {
                        found = true;
                        break;
                    }
                }
                if (found == false)
                {
                    removeFromSelection(*oIt);
                    m_canvas->getSegmentItem(*oIt)->
                        setSelected(false, convertColour(
                            m_doc->getComposition().getSegmentColourMap().
                            getColourByIndex(m_canvas->getSegmentItem(*oIt)
                            ->getSegment()->getColourIndex())));
                }
            }

            if (segCount)
            {
                emit selectedSegments(getSelectedSegments());
            }
        }
        return FollowHorizontal | FollowVertical;
    }

    m_canvas->viewport()->setCursor(Qt::sizeAllCursor);

    if (m_segmentCopyMode && !m_segmentQuickCopyDone)
    {
	KMacroCommand *mcommand = new KMacroCommand
	    (SegmentQuickCopyCommand::getGlobalName());

	SegmentItemList::iterator it;
	for (it = m_selectedItems.begin();
	     it != m_selectedItems.end();
	     it++)
	{
	    SegmentQuickCopyCommand *command =
		new SegmentQuickCopyCommand(it->second->getSegment());

	    mcommand->addCommand(command);
	}

        addCommandToHistory(mcommand);

//        Rosegarden::Segment *newSegment = command->getCopy();

        // generate SegmentItem
        //
	m_canvas->updateAllSegmentItems();
        m_segmentQuickCopyDone = true;

        // Don't understand why swapping selected item is causing
        // problem hereafter - so leaving this commented out.
        //
        //SegmentItem *newItem = m_canvas->getSegmentItem(newSegment);
        //clearSelected();
        //m_currentItem = newItem;
        //m_currentItem->setZ(3); // bring it to the top
        //slotSelectSegmentItem(newItem);
    }

    m_canvas->setSnapGrain(true);

    if (m_currentItem->isSelected())
    {
	SegmentItemList::iterator it;
        int guideX = 0;
        int guideY = 0;

	for (it = m_selectedItems.begin();
	     it != m_selectedItems.end();
	     it++)
	{
	    int x = tPos.x() - m_clickPoint.x(),
		y = tPos.y() - m_clickPoint.y();

	    const int inertiaDistance = m_canvas->grid().getYSnap() / 3;
	    if (!m_passedInertiaEdge &&
		(x < inertiaDistance && x > -inertiaDistance) &&
		(y < inertiaDistance && y > -inertiaDistance)) {
		return false;
	    } else {
		m_passedInertiaEdge = true;
	    }

	    timeT newStartTime = m_canvas->grid().snapX(it->first.x() + x);

	    it->second->setEndTime(it->second->getEndTime() + newStartTime -
				   it->second->getStartTime());
	    it->second->setStartTime(newStartTime);
            it->second->showRepeatRect(false);


	    TrackId track;
            int newY=it->first.y() + y;
            // Make sure we don't set a non-existing track
            if (newY < 0) { newY = 0; }
            track = m_canvas->grid().getYBin(newY);

	    RG_DEBUG << "SegmentSelector::handleMouseMove: orig y " << it->first.y()
		     << ", dy " << y << ", newY " << newY << ", track " << track << endl;

            // Make sure we don't set a non-existing track (c'td)
            // TODO: make this suck less. Either the tool should
            // not allow it in the first place, or we automatically
            // create new tracks - might make undo very tricky though
            //
            if (track >= TrackId(m_doc->getComposition().getNbTracks()))
                track  = TrackId(m_doc->getComposition().getNbTracks() - 1);

            /*
            if (it == m_selectedItems.begin())
            {
                guideX = int(m_canvas->grid().getRulerScale()->
                    getXForTime(newStartTime));

                guideY = m_canvas->grid().getYBinCoordinate(track);
            }
            else
            {
                if (x < guideX)
                    guideX = int(m_canvas->grid().getRulerScale()->
                        getXForTime(newStartTime));

                if (y < guideY)
                    guideY = m_canvas->grid().getYBinCoordinate(track);
            }
            */


            // This is during a "mover" so don't use the normalised (i.e.
            // proper) TrackPosition value yet.
            //
	    it->second->setTrackPosition(track);

	}

        guideX = int(m_currentItem->x());
        guideY = int(m_currentItem->y());

        m_foreGuide->setX(guideX - 2);
        m_topGuide->setY(guideY - 2);

	m_canvas->canvas()->update();
    }

    return FollowHorizontal | FollowVertical;
}

//////////////////////////////
//
// SegmentSplitter
//
//////////////////////////////


SegmentSplitter::SegmentSplitter(SegmentCanvas *c, RosegardenGUIDoc *d)
    : SegmentTool(c, d)
{
    RG_DEBUG << "SegmentSplitter()\n";
}

SegmentSplitter::~SegmentSplitter()
{
}

void SegmentSplitter::ready()
{
    m_canvas->viewport()->setCursor(Qt::splitHCursor);
}

void
SegmentSplitter::handleMouseButtonPress(QMouseEvent *e)
{
    QWMatrix matrix = m_canvas->worldMatrix().invert();
    QPoint tPos = matrix.map(e->pos());

    // Remove cursor and replace with line on a SegmentItem
    // at where the cut will be made
    SegmentItem *item = m_canvas->findSegmentClickedOn(tPos);

    if (item)
    {
        m_canvas->viewport()->setCursor(Qt::blankCursor);
        drawSplitLine(e);
    }

}

// Actually perform a split if we're on a Segment.
// Return the Segment pointer and the desired split
// time to the Document level.
//
void
SegmentSplitter::handleMouseButtonRelease(QMouseEvent *e)
{
    QWMatrix matrix = m_canvas->worldMatrix().invert();
    QPoint tPos = matrix.map(e->pos());

    SegmentItem *item = m_canvas->findSegmentClickedOn(tPos);

    if (item)
    {
	m_canvas->setSnapGrain(true);

        if (item->getSegment()->getType() == Rosegarden::Segment::Audio)
        {
            AudioSegmentSplitCommand *command =
                new AudioSegmentSplitCommand( item->getSegment(),
                                    m_canvas->grid().snapX(tPos.x()));
            addCommandToHistory(command);
        }
        else
        {
            SegmentSplitCommand *command =
                new SegmentSplitCommand(item->getSegment(),
                                    m_canvas->grid().snapX(tPos.x()));
            addCommandToHistory(command);
        }

    }
 
    // Reinstate the cursor
    m_canvas->viewport()->setCursor(Qt::splitHCursor);
    m_canvas->slotHideSplitLine();
}


int
SegmentSplitter::handleMouseMove(QMouseEvent *e)
{
    QWMatrix matrix = m_canvas->worldMatrix().invert();
    QPoint tPos = matrix.map(e->pos());

    SegmentItem *item = m_canvas->findSegmentClickedOn(tPos);

    if (item)
    {
        m_canvas->viewport()->setCursor(Qt::blankCursor);
        drawSplitLine(e);
	return FollowHorizontal;
    }
    else
    {
        m_canvas->viewport()->setCursor(Qt::splitHCursor);
        m_canvas->slotHideSplitLine();
	return NoFollow;
    }
}

// Draw the splitting line
//
void
SegmentSplitter::drawSplitLine(QMouseEvent *e)
{ 
    m_canvas->setSnapGrain(true);

    QWMatrix matrix = m_canvas->worldMatrix().invert();
    QPoint tPos = matrix.map(e->pos());

    // Turn the real X into a snapped X
    //
    timeT xT = m_canvas->grid().snapX(tPos.x());
    int x = (int)(m_canvas->grid().getRulerScale()->getXForTime(xT));

    // Need to watch y doesn't leak over the edges of the
    // current Segment.
    //
    int y = m_canvas->grid().snapY(tPos.y());

    m_canvas->slotShowSplitLine(x, y);

}


void
SegmentSplitter::contentsMouseDoubleClickEvent(QMouseEvent*)
{
}


//////////////////////////////
//
// SegmentJoiner
//
//////////////////////////////
SegmentJoiner::SegmentJoiner(SegmentCanvas *c, RosegardenGUIDoc *d)
    : SegmentTool(c, d)
{
    RG_DEBUG << "SegmentJoiner()\n";
}

SegmentJoiner::~SegmentJoiner()
{
}

void
SegmentJoiner::handleMouseButtonPress(QMouseEvent*)
{
}

void
SegmentJoiner::handleMouseButtonRelease(QMouseEvent*)
{
}


int
SegmentJoiner::handleMouseMove(QMouseEvent*)
{
    return NoFollow;
}

void
SegmentJoiner::contentsMouseDoubleClickEvent(QMouseEvent*)
{
}

//------------------------------

const QString SegmentPencil::ToolName   = "segmentpencil";
const QString SegmentEraser::ToolName   = "segmenteraser";
const QString SegmentMover::ToolName    = "segmentmover";
const QString SegmentResizer::ToolName  = "segmentresizer";
const QString SegmentSelector::ToolName = "segmentselector";
const QString SegmentSplitter::ToolName = "segmentsplitter";
const QString SegmentJoiner::ToolName   = "segmentjoiner";
