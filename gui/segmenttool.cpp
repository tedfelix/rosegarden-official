// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
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

#include "segmenttool.h"

#include "SnapGrid.h"
#include "NotationTypes.h"

#include "segmentcommands.h" // for SegmentRecSet
#include "segmentcanvas.h"
#include "colours.h"

#include "rosedebug.h"

using Rosegarden::TrackId;
using Rosegarden::timeT;
using Rosegarden::SnapGrid;
using Rosegarden::Note;
using Rosegarden::SegmentSelection;

//////////////////////////////////////////////////////////////////////
//                 Segment Tools
//////////////////////////////////////////////////////////////////////

SegmentTool::SegmentTool(SegmentCanvas* canvas, RosegardenGUIDoc *doc)
    : m_canvas(canvas),
      m_currentItem(0),
      m_doc(doc)
{
    m_canvas->setCursor(Qt::arrowCursor);
}

SegmentTool::~SegmentTool()
{
}

void
SegmentTool::addCommandToHistory(KCommand *command)
{
    m_doc->getCommandHistory()->addCommand(command);
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
    m_canvas->setCursor(Qt::ibeamCursor);
    RG_DEBUG << "SegmentPencil()\n";
}

void SegmentPencil::handleMouseButtonPress(QMouseEvent *e)
{
    m_newRect = false;
    m_currentItem = 0;

    // Check if we're clicking on a rect
    //
    SegmentItem *item = m_canvas->findSegmentClickedOn(e->pos());

    if (item) return;

    m_canvas->setSnapGrain(false);

    TrackId track = m_canvas->grid().getYBin(e->pos().y());

    // Don't do anything if the user clicked beyond the track buttons
    //
    if (track >= TrackId(m_doc->getComposition().getNbTracks())) return;

    timeT time = m_canvas->grid().snapX(e->pos().x(), SnapGrid::SnapLeft);
    timeT duration = m_canvas->grid().getSnapTime(e->pos().x());
    if (duration == 0) duration = Note(Note::Shortest).getDuration();
    
    m_currentItem = m_canvas->addSegmentItem(track, time, time + duration);
    m_newRect = true;
    
    m_canvas->slotUpdate();
}

void SegmentPencil::handleMouseButtonRelease(QMouseEvent*)
{
    if (!m_currentItem) return;
    m_currentItem->normalize();

    if (m_newRect) {
        SegmentInsertCommand *command =
            new SegmentInsertCommand(m_doc,
                                     m_currentItem->getTrack(),
                                     m_currentItem->getStartTime(),
                                     m_currentItem->getEndTime());

        addCommandToHistory(command);
    }

    delete m_currentItem;
    m_currentItem = 0;
    m_newRect = false;
}

int SegmentPencil::handleMouseMove(QMouseEvent *e)
{
    if (!m_currentItem) return NoFollow;

    m_canvas->setSnapGrain(false);

    SnapGrid::SnapDirection direction = SnapGrid::SnapRight;
    if (e->pos().x() < m_currentItem->x()) direction = SnapGrid::SnapLeft;

    timeT snap = m_canvas->grid().getSnapTime(e->pos().x());
    if (snap == 0) snap = Note(Note::Shortest).getDuration();

    timeT time = m_canvas->grid().snapX(e->pos().x(), direction);
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
    m_canvas->setCursor(Qt::pointingHandCursor);

    RG_DEBUG << "SegmentEraser()\n";
}

void SegmentEraser::handleMouseButtonPress(QMouseEvent *e)
{
    m_currentItem = m_canvas->findSegmentClickedOn(e->pos());
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
    m_canvas->setCursor(Qt::sizeAllCursor);

    m_foreGuide->setPen(RosegardenGUIColours::MovementGuide);
    m_foreGuide->setBrush(RosegardenGUIColours::MovementGuide);
    m_foreGuide->hide();

    m_topGuide->setPen(RosegardenGUIColours::MovementGuide);
    m_topGuide->setBrush(RosegardenGUIColours::MovementGuide);
    m_topGuide->hide();

    RG_DEBUG << "SegmentMover()\n";
}

void SegmentMover::handleMouseButtonPress(QMouseEvent *e)
{
    SegmentItem *item = m_canvas->findSegmentClickedOn(e->pos());

    if (item) {
        m_currentItem = item;
	m_currentItemStartX = item->x();
	m_clickPoint = e->pos();
        m_currentItem->showRepeatRect(false);

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

        // Don't update until the move
        //
        //m_canvas->canvas()->update();
    }
}

void SegmentMover::handleMouseButtonRelease(QMouseEvent*)
{
    if (m_currentItem)
    {
        SegmentReconfigureCommand *command =
                new SegmentReconfigureCommand("Move Segment");

        command->addSegment(m_currentItem->getSegment(),
                            m_currentItem->getStartTime(),
                            m_currentItem->getEndTime(),
                            m_currentItem->getTrack());
        addCommandToHistory(command);
        m_currentItem->showRepeatRect(true);

        m_foreGuide->hide();
        m_topGuide->hide();
    }

    m_currentItem = 0;
}

int SegmentMover::handleMouseMove(QMouseEvent *e)
{
    if (m_currentItem) {

	m_canvas->setSnapGrain(true);

	int x = e->x() - m_clickPoint.x();
        if (x < 0) x = 0;
        int y = e->y();
        if (y < 0) y = 0;

	timeT newStartTime = m_canvas->grid().snapX(m_currentItemStartX + x);
	m_currentItem->setEndTime(m_currentItem->getEndTime() + newStartTime -
				  m_currentItem->getStartTime());
	m_currentItem->setStartTime(newStartTime);

	TrackId track = m_canvas->grid().getYBin(y);
        m_currentItem->setTrack(track);

        m_foreGuide->setX(int(m_canvas->grid().getRulerScale()->
                            getXForTime(newStartTime)) - 2);

        m_topGuide->setY(m_canvas->grid().getYBinCoordinate(track));

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
    m_canvas->setCursor(Qt::sizeHorCursor);

    RG_DEBUG << "SegmentResizer()\n";
}

void SegmentResizer::handleMouseButtonPress(QMouseEvent *e)
{
    SegmentItem* item = m_canvas->findSegmentClickedOn(e->pos());

    if (item && cursorIsCloseEnoughToEdge(item, e, m_edgeThreshold)) {
        m_currentItem = item;
    }
}

void SegmentResizer::handleMouseButtonRelease(QMouseEvent*)
{
    if (!m_currentItem) return;
    m_currentItem->normalize();

    // normalisation may mean start time has changed as well as duration
    SegmentReconfigureCommand *command =
                new SegmentReconfigureCommand("Resize Segment");

    command->addSegment(m_currentItem->getSegment(),
                        m_currentItem->getStartTime(),
                        m_currentItem->getEndTime(),
                        m_currentItem->getTrack());
    addCommandToHistory(command);

    // update preview
    m_currentItem->getPreview()->setPreviewCurrent(false);
    m_canvas->canvas()->update();

    m_currentItem = 0;
}

int SegmentResizer::handleMouseMove(QMouseEvent *e)
{
    if (!m_currentItem) return NoFollow;

    m_canvas->setSnapGrain(true);

    timeT time = m_canvas->grid().snapX(e->pos().x());
    timeT duration = time - m_currentItem->getStartTime();

    timeT snap = m_canvas->grid().getSnapTime(e->pos().x());
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
    m_currentItem->getPreview()->setPreviewCurrent(false);

    m_canvas->canvas()->update();
    return FollowHorizontal;
}

bool SegmentResizer::cursorIsCloseEnoughToEdge(SegmentItem* p, QMouseEvent* e,
					       int edgeThreshold)
{
    return (abs(p->rect().x() + p->rect().width() - e->x()) < edgeThreshold);
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

    connect(this, SIGNAL(selectedSegments(const Rosegarden::SegmentSelection &)),
            c,     SIGNAL(selectedSegments(const Rosegarden::SegmentSelection &)));
}

SegmentSelector::~SegmentSelector()
{
    clearSelected();
    delete m_dispatchTool;
}

void
SegmentSelector::removeFromSelection(Rosegarden::Segment *segment)
{
    for (SegmentItemList::iterator i = m_selectedItems.begin();
	 i != m_selectedItems.end(); ++i) {
	if (i->second->getSegment() == segment) {
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
    for (SegmentItemList::iterator i = m_selectedItems.begin();
	 i != m_selectedItems.end(); ++i) {
	if (i->second == item) return;
    }
    m_selectedItems.push_back
	(SegmentItemPair(QPoint(int(item->x()), int(item->y())), item));
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
        it->second->setSelected(false, m_canvas->getSegmentBrush());
    }

    // now clear the selection
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
    SegmentItem *item = m_canvas->findSegmentClickedOn(e->pos());

    // If we're in segmentAddMode then we don't clear the
    // selection vector
    //
    if (!m_segmentAddMode) {
        clearSelected();
    }

    if (item) {
	
        // Ten percent of the width of the SegmentItem
        //
        int threshold = int(float(item->width()) * 0.15);
        if (threshold  == 0) threshold = 1;
        if (threshold > 10) threshold = 10;

	if (!m_segmentAddMode &&
	    SegmentResizer::cursorIsCloseEnoughToEdge(item, e, threshold)) {
	    m_dispatchTool = new SegmentResizer(m_canvas, m_doc, threshold);
	    m_dispatchTool->handleMouseButtonPress(e);
	    return;
	}


        // Moving
        //
        m_currentItem = item;
        m_clickPoint = e->pos();
        slotSelectSegmentItem(m_currentItem);

        m_foreGuide->setX(int(m_canvas->grid().getRulerScale()->
                           getXForTime(item->getSegment()->getStartTime())) -2);
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

    } else {


        // Add on middle button - bounding box on rest
        //
	if (e->button() == MidButton) {
	    m_dispatchTool = new SegmentPencil(m_canvas, m_doc);
	    m_dispatchTool->handleMouseButtonPress(e);
	    return;
	}
        else {
            // do a bounding box
            QCanvasRectangle *rect  = m_canvas->getSelectionRectangle();

            if (rect) {
                rect->show();
                rect->setX(e->x());
                rect->setY(e->y());
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
    // If we're selecting a Segment through this method
    // then don't set the m_currentItem
    //
    selectedItem->setSelected(true, m_canvas->getHighlightBrush());
    m_selectedItems.push_back(SegmentItemPair
                 (QPoint((int)selectedItem->x(), (int)selectedItem->y()),
                  selectedItem));
    m_canvas->canvas()->update();
}

void
SegmentSelector::handleMouseButtonRelease(QMouseEvent *e)
{
    if (m_dispatchTool) {
	m_dispatchTool->handleMouseButtonRelease(e);
	delete m_dispatchTool;
	m_dispatchTool = 0;
	m_canvas->setCursor(Qt::arrowCursor);
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

    m_canvas->setCursor(Qt::arrowCursor);

    if (m_currentItem->isSelected())
    {
	SegmentItemList::iterator it;

	bool haveChange = false;

	SegmentReconfigureCommand *command =
	    new SegmentReconfigureCommand
	    (m_selectedItems.size() == 1 ? "Move Segment" :
	                                   "Move Segments");

	for (it = m_selectedItems.begin();
	     it != m_selectedItems.end();
	     it++)
	{
	    SegmentItem *item = it->second;

	    if (item->getStartTime() != item->getSegment()->getStartTime() ||
		item->getEndTime()   != item->getSegment()->getEndMarkerTime() ||
		item->getTrack()     != item->getSegment()->getTrack()) {

		command->addSegment(item->getSegment(),
				    item->getStartTime(),
				    item->getEndTime(),
				    item->getTrack());

		haveChange = true;
	    }
	}

	if (haveChange) addCommandToHistory(command);

        // Hide guides
        //
        m_foreGuide->hide();
        m_topGuide->hide();

	m_canvas->canvas()->update();
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

    if (!m_currentItem)  {

        // do a bounding box
        QCanvasRectangle *selectionRect  = m_canvas->getSelectionRectangle();
        QRect rect = selectionRect->rect().normalize();

        if (selectionRect) {
            selectionRect->show();

            // same as for notation view
            int w = int(e->x() - selectionRect->x());
            int h = int(e->y() - selectionRect->y());
            if (w > 0) ++w; else --w;
            if (h > 0) ++h; else --h;

            selectionRect->setSize(w, h);
	    m_canvas->canvas()->update();

            // Get collisions and do selection
            //
            QCanvasItemList l = selectionRect->collisions(true); // exact collisions

            // selection management
            SegmentSelection oldSelection = getSelectedSegments();
            SegmentSelection newSelection;

            int segCount = 0;

            if (l.count()) {
                for (QCanvasItemList::Iterator it=l.begin(); it!=l.end(); ++it)
                {
                    if (SegmentItem *item = dynamic_cast<SegmentItem*>(*it))
                    {
                        if (!isGreedy() &&
                            !rect.contains(item->rect())) continue;

                        segCount++;
                        slotSelectSegmentItem(item);
                        newSelection.insert(item->getSegment());
                    }
                }
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
                        setSelected(false, m_canvas->getSegmentBrush());
                }
            }

            if (segCount)
            {
                emit selectedSegments(getSelectedSegments());
            }
        }
        return NoFollow;
    }

    m_canvas->setCursor(Qt::sizeAllCursor);

    if (m_segmentCopyMode && !m_segmentQuickCopyDone)
    {
	SegmentQuickCopyCommand *command =
            new SegmentQuickCopyCommand(m_currentItem->getSegment());
        // addCommand to generate the new Segment
        //
        addCommandToHistory(command);

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
	    TrackId track = m_canvas->grid().getYBin(it->first.y() + y);

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

            // Make sure we don't set a non-existing track
            // TODO: make this suck less. Either the tool should
            // not allow it in the first place, or we automatically
            // create new tracks - might make undo very tricky though
            //
            if (track >= TrackId(m_doc->getComposition().getNbTracks())) 
                track  = TrackId(m_doc->getComposition().getNbTracks() - 1);

	    it->second->setTrack(track);
	}

        m_foreGuide->setX(guideX - 2);
        m_topGuide->setY(guideY - 2);

	m_canvas->canvas()->update();
    }

    return FollowHorizontal | FollowVertical;
}

bool SegmentSelector::m_greedy = true;


//////////////////////////////
//
// SegmentSplitter
//
//////////////////////////////


SegmentSplitter::SegmentSplitter(SegmentCanvas *c, RosegardenGUIDoc *d)
    : SegmentTool(c, d)
{
    RG_DEBUG << "SegmentSplitter()\n";
    m_canvas->setCursor(Qt::splitHCursor);
}

SegmentSplitter::~SegmentSplitter()
{
}

void
SegmentSplitter::handleMouseButtonPress(QMouseEvent *e)
{
    // Remove cursor and replace with line on a SegmentItem
    // at where the cut will be made
    SegmentItem *item = m_canvas->findSegmentClickedOn(e->pos());

    if (item)
    {
        m_canvas->setCursor(Qt::blankCursor);
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
    SegmentItem *item = m_canvas->findSegmentClickedOn(e->pos());

    if (item)
    {
	m_canvas->setSnapGrain(true);

        if (item->getSegment()->getType() == Rosegarden::Segment::Audio)
        {
            AudioSegmentSplitCommand *command =
                new AudioSegmentSplitCommand( item->getSegment(),
                                    m_canvas->grid().snapX(e->pos().x()));
            addCommandToHistory(command);
        }
        else
        {
            SegmentSplitCommand *command =
                new SegmentSplitCommand(item->getSegment(),
                                    m_canvas->grid().snapX(e->pos().x()));
            addCommandToHistory(command);
        }

    }
 
    // Reinstate the cursor
    m_canvas->setCursor(Qt::splitHCursor);
    m_canvas->slotHideSplitLine();
}


int
SegmentSplitter::handleMouseMove(QMouseEvent *e)
{
    SegmentItem *item = m_canvas->findSegmentClickedOn(e->pos());

    if (item)
    {
        m_canvas->setCursor(Qt::blankCursor);
        drawSplitLine(e);
	return FollowHorizontal;
    }
    else
    {
        m_canvas->setCursor(Qt::splitHCursor);
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

    // Turn the real X into a snapped X
    //
    timeT xT = m_canvas->grid().snapX(e->pos().x());
    int x = (int)(m_canvas->grid().getRulerScale()->getXForTime(xT));

    // Need to watch y doesn't leak over the edges of the
    // current Segment.
    //
    int y = m_canvas->grid().snapY(e->pos().y());

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
