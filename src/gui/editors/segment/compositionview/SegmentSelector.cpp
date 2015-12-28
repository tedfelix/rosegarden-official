/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[SegmentSelector]"

#include "SegmentSelector.h"

#include "misc/Strings.h"
#include "misc/Debug.h"
#include "base/Composition.h"
#include "base/RealTime.h"
#include "base/SnapGrid.h"
#include "base/Track.h"
#include "commands/segment/SegmentQuickCopyCommand.h"
#include "commands/segment/SegmentQuickLinkCommand.h"
#include "commands/segment/SegmentReconfigureCommand.h"
#include "CompositionModelImpl.h"
#include "CompositionView.h"
#include "document/RosegardenDocument.h"
#include "document/CommandHistory.h"
#include "misc/ConfigGroups.h"
#include "gui/general/RosegardenScrollView.h"
#include "SegmentPencil.h"
#include "SegmentResizer.h"
#include "SegmentToolBox.h"

#include <QSettings>
#include <QMouseEvent>

#include <math.h>

namespace Rosegarden
{


const QString SegmentSelector::ToolName = "segmentselector";

SegmentSelector::SegmentSelector(CompositionView *c, RosegardenDocument *d) :
    SegmentTool(c, d),
    m_clickPoint(),
    m_segmentAddMode(false),
    m_segmentCopyMode(false),
    m_segmentCopyingAsLink(false),
    m_passedInertiaEdge(false),
    m_segmentQuickCopyDone(false),
    m_selectionMoveStarted(false),
    m_changeMade(false),
    m_dispatchTool(0)
{
    //RG_DEBUG << "SegmentSelector()";
}

SegmentSelector::~SegmentSelector()
{
}

void SegmentSelector::ready()
{
    m_canvas->viewport()->setCursor(Qt::ArrowCursor);
    setContextHelpFor(QPoint(0,0), false);
}

static bool isNearEdge(const QRect &segmentRect, const QPoint &cursor)
{
    // Fifteen percent of the width of the segment, up to 10px

    int threshold = lround(segmentRect.width() * 0.15);

    if (threshold == 0)
        threshold = 1;
    if (threshold > 10)
        threshold = 10;

    // Near right edge?
    if (segmentRect.right() - cursor.x() < threshold)
        return true;

    // Near left edge?
    if (cursor.x() - segmentRect.left() < threshold)
        return true;

    return false;
}

void
SegmentSelector::mousePressEvent(QMouseEvent *e)
{
    // Let the baseclass have a go.
    SegmentTool::mousePressEvent(e);

    // We only care about the left and middle mouse buttons.
    if (e->button() != Qt::LeftButton  &&
        e->button() != Qt::MidButton)
        return;

    // No need to propagate.
    e->accept();

    QPoint pos = m_canvas->viewportToContents(e->pos());

    // Get the segment that was clicked.
    ChangingSegmentPtr item = m_canvas->getModel()->getSegmentAt(pos);

    // If middle button...
    if (e->button() == Qt::MidButton) {
        // If on the background, create a new segment.
        if (!item) {
            m_dispatchTool = m_canvas->getToolBox()->getTool(SegmentPencil::ToolName);

            if (m_dispatchTool) {
                m_dispatchTool->ready(); // set mouse cursor
                m_dispatchTool->mousePressEvent(e);
            }
        }

        return;
    }

    // Left button.

    bool shift = ((e->modifiers() & Qt::ShiftModifier) != 0);
    bool ctrl = ((e->modifiers() & Qt::ControlModifier) != 0);
    bool alt = ((e->modifiers() & Qt::AltModifier) != 0);

    // Shift adds to the selection.
    m_segmentAddMode = shift;
    // Ctrl and Alt+Ctrl are segment copy.
    m_segmentCopyMode = ctrl;
    // Alt+Ctrl is copy as link.
    // ??? Grouping Alt+Ctrl and whether the segment is linked seems
    //     suspect.
    m_segmentCopyingAsLink = (
        (item  &&  item->getSegment()->isTrulyLinked())  ||
        (alt && ctrl));

    if (!m_segmentAddMode) {
        // If the background is being clicked, or the item being clicked isn't
        // selected, clear the selection.
        if (!item  ||  !m_canvas->getModel()->isSelected(item->getSegment())) {
            m_canvas->getModel()->clearSelected();
        }
    }

    // If a segment was clicked
    if (item) {

        // *** Resize Segment

        // Resize if we're dragging from the edge, provided we aren't
        // in segment-add mode with at least one segment already
        // selected -- as we aren't able to resize multiple segments
        // at once, we should assume the segment-add aspect takes
        // priority

        // We can only resize one segment at a time.
        // ??? This is convoluted, and it results in a bug.  If you try to
        //     resize with the shift key held down (to turn off snap), it
        //     doesn't work if the segment is selected.  We need a more
        //     direct way to determine whether exactly one segment is
        //     or is going to be selected.
        // ??? Would it be possible to adjust selection first, then handle
        //     other behavior?
        bool oneSegmentSelected =
                (!m_segmentAddMode  ||
                 !m_canvas->getModel()->haveSelection());

        if (oneSegmentSelected  &&  isNearEdge(item->rect(), pos)) {

            SegmentResizer *segmentResizer = dynamic_cast<SegmentResizer *>(
                m_canvas->getToolBox()->getTool(SegmentResizer::ToolName));

            // For the moment we only allow resizing of a single segment
            // at a time.
            //
            m_canvas->getModel()->clearSelected();
            m_canvas->getModel()->setSelected(item->getSegment());

            // Turn it over to SegmentResizer.
            m_dispatchTool = segmentResizer;
            m_dispatchTool->ready(); // set mouse cursor
            m_dispatchTool->mousePressEvent(e);

            return;
        }

        // *** Selecting and Moving

        // ??? Why not let SegmentMover take care of moving?  This
        //     code is awfully similar.  Not similar enough?

        bool selecting = true;
        
        if (m_segmentAddMode  &&
            m_canvas->getModel()->isSelected(item->getSegment())) {

            selecting = false;
        } else {
            // put the segment in 'move' mode only if it's being selected
            m_canvas->getModel()->startChange(item, CompositionModelImpl::ChangeMove);
        }

        m_canvas->getModel()->setSelected(item->getSegment(), selecting);

        //RG_DEBUG << "mousePressEvent() - item = " << item;
        // ??? This was a line that appeared to leak memory.  Was it really
        //     leaking memory?
        setChangingSegment(item);
        m_clickPoint = pos;

        int guideX = item->rect().x();
        int guideY = item->rect().y();

        m_canvas->drawGuides(guideX, guideY);

        setSnapTime(e, SnapGrid::SnapToBeat);

    } else {  // The background was clicked

        // If Ctrl+left button, create a new segment.
        if (ctrl) {
            m_dispatchTool = m_canvas->getToolBox()->getTool(SegmentPencil::ToolName);

            if (m_dispatchTool) {
                m_dispatchTool->ready(); // set mouse cursor
                m_dispatchTool->mousePressEvent(e);
            }

            return;

        } else {

            // Selection rubber band

            m_canvas->drawSelectionRectPos1(pos);
            if (!m_segmentAddMode)
                m_canvas->getModel()->clearSelected();

        }
    }

    // Tell the RosegardenMainViewWidget that we've selected some new Segments -
    // when the list is empty we're just unselecting.
    //
    m_canvas->getModel()->selectionHasChanged();

    m_passedInertiaEdge = false;
}

void
SegmentSelector::mouseReleaseEvent(QMouseEvent *e)
{
    // We only care about the left and middle mouse buttons.
    if (e->button() != Qt::LeftButton  &&
        e->button() != Qt::MidButton)
        return;

    // No need to propagate.
    e->accept();

    QPoint pos = m_canvas->viewportToContents(e->pos());

    m_canvas->hideGuides();
    m_canvas->hideTextFloat();

    if (m_dispatchTool) {
        m_dispatchTool->mouseReleaseEvent(e);
        m_dispatchTool = 0;
        m_canvas->viewport()->setCursor(Qt::ArrowCursor);
        return;
    }

    // We only handle the left button.  The middle button is handled by
    // the dispatch tool (segment pencil) or ignored.
    if (e->button() != Qt::LeftButton)
        return;

    int startDragTrackPos = m_canvas->grid().getYBin(m_clickPoint.y());
    int currentTrackPos = m_canvas->grid().getYBin(pos.y());
    int trackDiff = currentTrackPos - startDragTrackPos;

    if (!getChangingSegment()) {
        m_canvas->hideSelectionRect();
        m_canvas->getModel()->finalizeSelectionRect();
        m_canvas->getModel()->selectionHasChanged();
        return ;
    }

    m_canvas->viewport()->setCursor(Qt::ArrowCursor);

    Composition &comp = m_doc->getComposition();

    if (m_canvas->getModel()->isSelected(getChangingSegment()->getSegment())) {

        CompositionModelImpl::ChangingSegmentSet& changingItems =
                m_canvas->getModel()->getChangingSegments();
        CompositionModelImpl::ChangingSegmentSet::iterator it;

        if (m_changeMade) {

            SegmentReconfigureCommand *command =
                new SegmentReconfigureCommand
                (tr("Move %n Segment(s)", "", changingItems.size()), &comp);

            for (it = changingItems.begin();
                    it != changingItems.end();
                    ++it) {

                ChangingSegmentPtr item = *it;

                Segment* segment = item->getSegment();

                TrackId origTrackId = segment->getTrack();
                int trackPos = comp.getTrackPositionById(origTrackId);
                trackPos += trackDiff;

                if (trackPos < 0) {
                    trackPos = 0;
                } else if (trackPos >= (int)comp.getNbTracks()) {
                    trackPos = comp.getNbTracks() - 1;
                }

                Track *newTrack = comp.getTrackByPosition(trackPos);
                int newTrackId = origTrackId;
                if (newTrack) newTrackId = newTrack->getId();

                timeT itemStartTime = item->getStartTime(m_canvas->grid());

                // We absolutely don't want to snap the end time to
                // the grid.  We want it to remain exactly the same as
                // it was, but relative to the new start time.
                timeT itemEndTime = itemStartTime + segment->getEndMarkerTime(false)
                                    - segment->getStartTime();

                //RG_DEBUG << "mouseReleaseEvent(): releasing segment " << segment << ": mouse started at track " << startDragTrackPos << ", is now at " << currentTrackPos << ", diff is " << trackDiff << ", moving from track pos " << comp.getTrackPositionById(origTrackId) << " to " << trackPos << ", id " << origTrackId << " to " << newTrackId;

                command->addSegment(segment,
                                    itemStartTime,
                                    itemEndTime,
                                    newTrackId);
            }

            CommandHistory::getInstance()->addCommand(command);
        }

        m_canvas->getModel()->endChange();
        m_canvas->slotUpdateAll();
    }

    // if we've just finished a quick copy then drop the Z level back
    if (m_segmentQuickCopyDone) {
        m_segmentQuickCopyDone = false;
        //        getChangingSegment()->setZ(2); // see SegmentItem::setSelected  --??
    }

    m_changeMade = false;

    m_selectionMoveStarted = false;

    setChangingSegment(ChangingSegmentPtr());

    setContextHelpFor(pos);
}

int
SegmentSelector::mouseMoveEvent(QMouseEvent *e)
{
    // No need to propagate.
    e->accept();

    QPoint pos = m_canvas->viewportToContents(e->pos());

    // If no buttons are pressed, update the context help and bail.
    // Note: Mouse tracking must be on for this to work.  See
    //       QWidget::setMouseTracking().
    if (e->buttons() == Qt::NoButton) {
        setContextHelpFor(pos, (e->modifiers() & Qt::ControlModifier));
        return RosegardenScrollView::NoFollow;
    }

    if (m_dispatchTool)
        return m_dispatchTool->mouseMoveEvent(e);

    // We only handle the left button.  The middle button is handled by
    // the dispatch tool (segment pencil) or ignored.
    if (e->buttons() != Qt::LeftButton)
        return RosegardenScrollView::NoFollow;

    Composition &comp = m_doc->getComposition();

    if (!getChangingSegment()) {

        //RG_DEBUG << "mouseMoveEvent(): no current item";

        m_canvas->drawSelectionRectPos2(pos);

        m_canvas->getModel()->selectionHasChanged();
        return RosegardenScrollView::FollowHorizontal | RosegardenScrollView::FollowVertical;
    }

    m_canvas->viewport()->setCursor(Qt::SizeAllCursor);

    if (m_segmentCopyMode && !m_segmentQuickCopyDone) {
        MacroCommand *mcommand = 0;
        
        if (m_segmentCopyingAsLink) {
            mcommand = new MacroCommand
                           (SegmentQuickLinkCommand::getGlobalName());
        } else {
            mcommand = new MacroCommand
                           (SegmentQuickCopyCommand::getGlobalName());
        }

        SegmentSelection selectedItems = m_canvas->getSelectedSegments();
        SegmentSelection::iterator it;
        for (it = selectedItems.begin();
                it != selectedItems.end();
                ++it) {
            Command *command = 0;
        
            if (m_segmentCopyingAsLink) {
                command = new SegmentQuickLinkCommand(*it);
            } else {
                command = new SegmentQuickCopyCommand(*it);
            }

            mcommand->addCommand(command);
        }

        CommandHistory::getInstance()->addCommand(mcommand);

        // generate SegmentItem
        //
// 		m_canvas->updateContents();
		m_canvas->update();

		m_segmentQuickCopyDone = true;
    }

    setSnapTime(e, SnapGrid::SnapToBeat);

    int startDragTrackPos = m_canvas->grid().getYBin(m_clickPoint.y());
    int currentTrackPos = m_canvas->grid().getYBin(pos.y());
    int trackDiff = currentTrackPos - startDragTrackPos;

    if (m_canvas->getModel()->isSelected(getChangingSegment()->getSegment())) {

        // If shift isn't being held down
        if ((e->modifiers() & Qt::ShiftModifier) == 0) {
            setContextHelp(tr("Hold Shift to avoid snapping to beat grid"));
        } else {
            clearContextHelp();
        }

        //RG_DEBUG << "mouseMoveEvent(): current item is selected";

        if (!m_selectionMoveStarted) { // start move on selected items only once
            m_canvas->getModel()->startChangeSelection(CompositionModelImpl::ChangeMove);
            m_selectionMoveStarted = true;
        }

        ChangingSegmentPtr newChangingSegment =
                m_canvas->getModel()->findChangingSegment(
                          getChangingSegment()->getSegment());

        if (newChangingSegment) {
            // Toss the local "changing" segment since it isn't going to
            // be moving at all.  Swap it for the same changing segment in
            // CompositionModelImpl.  That one *will* be moving and can be
            // used to drive the guides.
            setChangingSegment(newChangingSegment);
        }

        CompositionModelImpl::ChangingSegmentSet& changingItems =
                m_canvas->getModel()->getChangingSegments();

        CompositionModelImpl::ChangingSegmentSet::iterator it;

        for (it = changingItems.begin();
                it != changingItems.end();
                ++it) {

            //RG_DEBUG << "mouseMoveEvent(): movingItem at " << (*it)->rect().x() << "," << (*it)->rect().y();

            int dx = pos.x() - m_clickPoint.x(),
                dy = pos.y() - m_clickPoint.y();

            const int inertiaDistance = m_canvas->grid().getYSnap() / 3;
            if (!m_passedInertiaEdge &&
                    (dx < inertiaDistance && dx > -inertiaDistance) &&
                    (dy < inertiaDistance && dy > -inertiaDistance)) {
                return RosegardenScrollView::NoFollow;
            } else {
                m_passedInertiaEdge = true;
            }

            timeT newStartTime = m_canvas->grid().snapX((*it)->savedRect().x() + dx);

            int newX = int(m_canvas->grid().getRulerScale()->getXForTime(newStartTime));

            int trackPos = m_canvas->grid().getYBin((*it)->savedRect().y());

            //RG_DEBUG << "mouseMoveEvent(): segment " << *it << ": mouse started at track " << startDragTrackPos << ", is now at " << currentTrackPos << ", trackPos from " << trackPos << " to:";

            trackPos += trackDiff;

            //RG_DEBUG << "  " << trackPos;

            if (trackPos < 0) {
                trackPos = 0;
            } else if (trackPos >= (int)comp.getNbTracks()) {
                trackPos = comp.getNbTracks() - 1;
            }

            int newY = m_canvas->grid().getYBinCoordinate(trackPos);

            (*it)->moveTo(newX, newY);
            m_changeMade = true;
        }

        if (m_changeMade) {
            // Make sure the segments are redrawn.
            m_canvas->slotUpdateAll();
        }

        int guideX = getChangingSegment()->rect().x();
        int guideY = getChangingSegment()->rect().y();

        m_canvas->drawGuides(guideX, guideY);

        timeT currentIndexStartTime = m_canvas->grid().snapX(getChangingSegment()->rect().x());

        RealTime time = comp.getElapsedRealTime(currentIndexStartTime);
        QString ms;
        ms.sprintf("%03d", time.msec());

        int bar, beat, fraction, remainder;
        comp.getMusicalTimeForAbsoluteTime(currentIndexStartTime, bar, beat, fraction, remainder);

        QString posString = QString("%1.%2s (%3, %4, %5)")
                            .arg(time.sec).arg(ms)
                            .arg(bar + 1).arg(beat).arg(fraction);

        m_canvas->drawTextFloat(guideX + 10, guideY - 30, posString);
// 		m_canvas->updateContents();
		m_canvas->update();

    } else {
        //RG_DEBUG << "mouseMoveEvent(): current item not selected";
    }

    return RosegardenScrollView::FollowHorizontal | RosegardenScrollView::FollowVertical;
}

void SegmentSelector::setContextHelpFor(QPoint p, bool ctrlPressed)
{
    QSettings settings;
    settings.beginGroup( GeneralOptionsConfigGroup );

    if (! qStrToBool( settings.value("toolcontexthelp", "true" ) ) ) {
        settings.endGroup();
        return;
    }
    settings.endGroup();

    ChangingSegmentPtr item = m_canvas->getModel()->getSegmentAt(p);

    if (!item) {
        setContextHelp(tr("Click and drag to select segments; middle-click and drag to draw an empty segment"));

    } else {

        // Same logic as in mousePressEvent to establish
        // whether we'd be moving or resizing

        if ((!m_segmentAddMode ||
             !m_canvas->getModel()->haveSelection()) &&
            isNearEdge(item->rect(), p)) {

            if (!ctrlPressed) {
                setContextHelp(tr("Click and drag to resize a segment; hold Ctrl as well to rescale its contents"));
            } else {
                setContextHelp(tr("Click and drag to rescale segment"));
            }
        } else {
            if (m_canvas->getModel()->haveMultipleSelection()) {
                if (!ctrlPressed) {
                    setContextHelp(tr("Click and drag to move segments; hold Ctrl as well to copy them; Ctrl + Alt for linked copies"));
                } else {
                    setContextHelp(tr("Click and drag to copy segments"));
                }
            } else {
                if (!ctrlPressed) {
                    setContextHelp(tr("Click and drag to move segment; hold Ctrl as well to copy it; Ctrl + Alt for a linked copy; double-click to edit"));
                } else {
                    setContextHelp(tr("Click and drag to copy segment"));
                }
            }
        }
    }
}


}
