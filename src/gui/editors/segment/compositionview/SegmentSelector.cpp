/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.
 
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
#include "gui/general/RosegardenScrollView.h"
#include "SegmentPencil.h"
#include "SegmentResizer.h"
#include "SegmentToolBox.h"

#include <QMouseEvent>
#include <QKeyEvent>

#include <math.h>
#include <stdlib.h>

namespace Rosegarden
{


const QString SegmentSelector::ToolName = "segmentselector";

SegmentSelector::SegmentSelector(CompositionView *c, RosegardenDocument *d) :
    SegmentTool(c, d),
    m_clickPoint(),
    m_lastMousePos(),
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
    setContextHelpFor(QPoint(0,0));
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
        // If clicked on the background, create a new segment.
        if (!item) {
            m_dispatchTool = m_canvas->getToolBox()->getTool(SegmentPencil::ToolName);

            if (m_dispatchTool) {
                m_dispatchTool->ready(); // set mouse cursor
                m_dispatchTool->mousePressEvent(e);
            }
        }

        return;
    }

    // Left button was pressed.
    // ??? Should we split this into a midPress(e) and a leftPress(e)?
    //     Might improve readability a little.

    // *** Adjust Selection

    // Shift key adds to selection.
    m_segmentAddMode = ((e->modifiers() & Qt::ShiftModifier) != 0);

    // if a segment was clicked
    if (item) {
        bool selected = m_canvas->getModel()->isSelected(item->getSegment());
        if (m_segmentAddMode) {
            // toggle item selection
            m_canvas->getModel()->setSelected(item->getSegment(), !selected);
        } else {
            if (!selected) {
                // make the item the selection
                m_canvas->getModel()->clearSelected();
                m_canvas->getModel()->setSelected(item->getSegment());
            }
        }
    } else {  // the background was clicked
        if (!m_segmentAddMode) {
            // clear the selection
            m_canvas->getModel()->clearSelected();
        }
    }

    // *** Perform Functions

    bool ctrl = ((e->modifiers() & Qt::ControlModifier) != 0);

    // if a segment was clicked
    if (item) {

        // * Resize

        // if only one segment is selected and clicked near the edge, resize
        if (m_canvas->getModel()->getSelectedSegments().size() == 1  &&
            isNearEdge(item->rect(), pos)) {

            SegmentResizer *segmentResizer = dynamic_cast<SegmentResizer *>(
                m_canvas->getToolBox()->getTool(SegmentResizer::ToolName));

            // Turn it over to SegmentResizer.
            m_dispatchTool = segmentResizer;
            m_dispatchTool->ready(); // set mouse cursor
            m_dispatchTool->mousePressEvent(e);

            return;
        }

        // * Move

        // We don't dispatch to SegmentMover because SegmentMover
        // doesn't support copying/linking.

        bool alt = ((e->modifiers() & Qt::AltModifier) != 0);

        // Ctrl and Alt+Ctrl are segment copy.
        m_segmentCopyMode = ctrl;
        // Alt+Ctrl is copy as link.
        m_segmentCopyingAsLink = (alt && ctrl);

        // If the segment is selected, put it in move mode.
        if (m_canvas->getModel()->isSelected(item->getSegment())) {
            m_canvas->getModel()->startChange(
                    item, CompositionModelImpl::ChangeMove);
        }

        setChangingSegment(item);

        m_clickPoint = pos;

        int guideX = item->rect().x();
        int guideY = item->rect().y();

        m_canvas->drawGuides(guideX, guideY);

        setSnapTime(e, SnapGrid::SnapToBeat);

    } else {  // the background was clicked
        if (ctrl) {

            // * Create Segment

            m_dispatchTool = m_canvas->getToolBox()->getTool(
                    SegmentPencil::ToolName);

            if (m_dispatchTool) {
                m_dispatchTool->ready(); // set mouse cursor
                m_dispatchTool->mousePressEvent(e);
            }

            return;
        }

        // * Rubber Band

        m_canvas->drawSelectionRectPos1(pos);

    }

    // Make sure the Segment Parameters box is updated.  See
    // RosegardenMainViewWidget::slotSelectedSegments().
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

    // If another tool (SegmentPencil or SegmentResizer) has taken
    // over, delegate.
    if (m_dispatchTool) {
        m_dispatchTool->mouseReleaseEvent(e);
        m_dispatchTool->stow();

        // Forget about the tool.
        // Note that this is not a memory leak.  There is only one instance
        // of each tool stored in BaseToolBox::m_tools.
        m_dispatchTool = 0;

        // Back to this tool.
        ready();

        return;
    }

    // We only handle the left button.  The middle button is handled by
    // the dispatch tool (segment pencil) or ignored.
    if (e->button() != Qt::LeftButton)
        return;

    // The left button has been released.

    m_canvas->hideGuides();
    m_canvas->hideTextFloat();

    // If rubber band mode
    if (!getChangingSegment()) {
        m_canvas->hideSelectionRect();
        m_canvas->getModel()->finalizeSelectionRect();
        m_canvas->getModel()->selectionHasChanged();
        return;
    }

    m_canvas->viewport()->setCursor(Qt::ArrowCursor);

    if (m_canvas->getModel()->isSelected(getChangingSegment()->getSegment())) {

        if (m_changeMade) {

            MacroCommand *macroCommand = 0;

            CompositionModelImpl::ChangingSegmentSet &changingSegments =
                    m_canvas->getModel()->getChangingSegments();

            if (m_segmentCopyMode) {
                if (m_segmentCopyingAsLink) {
                    macroCommand = new MacroCommand(
                            tr("Copy %n Segment(s) as link(s)", "", changingSegments.size()));
                } else {
                    macroCommand = new MacroCommand(
                            tr("Copy %n Segment(s)", "", changingSegments.size()));
                }
            } else {
                macroCommand = new MacroCommand(
                        tr("Move %n Segment(s)", "", changingSegments.size()));
            }

            if (m_segmentCopyMode) {
                // Make copies of the original Segment(s).  These copies will
                // take the place of the originals.

                SegmentSelection selectedItems = m_canvas->getSelectedSegments();

                // for each selected segment
                for (SegmentSelection::iterator it = selectedItems.begin();
                     it != selectedItems.end();
                     ++it) {
                    Segment *segment = *it;

                    Command *command = 0;

                    if (m_segmentCopyingAsLink) {
                        command = new SegmentQuickLinkCommand(segment);
                    } else {
                        // if it's a link, copy as link
                        if (segment->isTrulyLinked())
                            command = new SegmentQuickLinkCommand(segment);
                        else  // copy as a non-link segment
                            command = new SegmentQuickCopyCommand(segment);
                    }

                    macroCommand->addCommand(command);
                }
            }

            const int startDragTrackPos =
                    m_canvas->grid().getYBin(m_clickPoint.y());
            const int currentTrackPos = m_canvas->grid().getYBin(pos.y());
            const int trackDiff = currentTrackPos - startDragTrackPos;

            Composition &comp = m_doc->getComposition();

            SegmentReconfigureCommand *segmentReconfigureCommand =
                    new SegmentReconfigureCommand("", &comp);

            // For each changing segment, add the segment to the
            // SegmentReconfigureCommand.
            for (CompositionModelImpl::ChangingSegmentSet::iterator it =
                         changingSegments.begin();
                 it != changingSegments.end();
                 ++it) {
                ChangingSegmentPtr changingSegment = *it;
                Segment *segment = changingSegment->getSegment();

                TrackId origTrackId = segment->getTrack();
                int newTrackPos =
                        comp.getTrackPositionById(origTrackId) + trackDiff;

                // Limit to [0, comp.getNbTracks()-1]
                if (newTrackPos < 0)
                    newTrackPos = 0;
                if (newTrackPos > static_cast<int>(comp.getNbTracks()) - 1)
                    newTrackPos = comp.getNbTracks() - 1;

                Track *newTrack = comp.getTrackByPosition(newTrackPos);
                int newTrackId = origTrackId;
                if (newTrack)
                    newTrackId = newTrack->getId();

                timeT startTime =
                        changingSegment->getStartTime(m_canvas->grid());

                // endTime = startTime + segment duration
                // We absolutely don't want to snap the end time to
                // the grid.  We want it to remain exactly the same as
                // it was, but relative to the new start time.
                timeT endTime = startTime +
                        segment->getEndMarkerTime(false) -
                        segment->getStartTime();

                segmentReconfigureCommand->addSegment(
                        segment, startTime, endTime, newTrackId);
            }

            macroCommand->addCommand(segmentReconfigureCommand);

            CommandHistory::getInstance()->addCommand(macroCommand);

            m_canvas->update();
        }

        m_canvas->getModel()->endChange();
        m_canvas->slotUpdateAll();
    }

    // Get ready for the next button press.
    m_segmentQuickCopyDone = false;
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
    m_lastMousePos = pos;

    // If no buttons are pressed, update the context help and bail.
    // Note: Mouse tracking must be on for this to work.  See
    //       QWidget::setMouseTracking().
    if (e->buttons() == Qt::NoButton) {
        setContextHelpFor(pos, e->modifiers());
        return RosegardenScrollView::NoFollow;
    }

    // If another tool has taken over, delegate.
    if (m_dispatchTool)
        return m_dispatchTool->mouseMoveEvent(e);

    // We only handle the left button.  The middle button is handled by
    // the dispatch tool (segment pencil) or ignored.
    if (e->buttons() != Qt::LeftButton)
        return RosegardenScrollView::NoFollow;

    // If we aren't moving anything, rubber band.
    if (!getChangingSegment()) {
        m_canvas->drawSelectionRectPos2(pos);
        m_canvas->getModel()->selectionHasChanged();

        return RosegardenScrollView::FollowHorizontal |
               RosegardenScrollView::FollowVertical;
    }

    // Moving

    // If the segment that was clicked on isn't selected, bail.
    if (!m_canvas->getModel()->isSelected(getChangingSegment()->getSegment()))
        return RosegardenScrollView::NoFollow;

    const int dx = pos.x() - m_clickPoint.x();
    const int dy = pos.y() - m_clickPoint.y();
    const int inertiaDistance = 8;

    // If we've not already exceeded the inertia distance, and we
    // still haven't, bail.
    if (!m_passedInertiaEdge  &&
        abs(dx) < inertiaDistance  &&
        abs(dy) < inertiaDistance) {
        return RosegardenScrollView::NoFollow;
    }

    m_passedInertiaEdge = true;

    m_canvas->viewport()->setCursor(Qt::SizeAllCursor);

#if 0
// ??? Moving to mouseReleaseEvent().
    if (m_segmentCopyMode  &&  !m_segmentQuickCopyDone) {
        // Make copies of the original Segment(s).  These copies will
        // take the place of the originals as the user drags the
        // the originals to a new location.

        MacroCommand *macroCommand = 0;
        
        if (m_segmentCopyingAsLink) {
            macroCommand = new MacroCommand(
                    SegmentQuickLinkCommand::getGlobalName());
        } else {
            macroCommand = new MacroCommand(
                    SegmentQuickCopyCommand::getGlobalName());
        }

        SegmentSelection selectedItems = m_canvas->getSelectedSegments();

        // for each selected segment
        for (SegmentSelection::iterator it = selectedItems.begin();
             it != selectedItems.end();
             ++it) {
            Segment *segment = *it;

            Command *command = 0;

            if (m_segmentCopyingAsLink) {
                command = new SegmentQuickLinkCommand(segment);
            } else {
                // if it's a link, copy as link
                if (segment->isTrulyLinked())
                    command = new SegmentQuickLinkCommand(segment);
                else  // copy as a non-link segment
                    command = new SegmentQuickCopyCommand(segment);
            }

            macroCommand->addCommand(command);
        }

        CommandHistory::getInstance()->addCommand(macroCommand);

        m_canvas->update();

        // Make sure we don't do it again.
        m_segmentQuickCopyDone = true;
    }
#endif

    setSnapTime(e, SnapGrid::SnapToBeat);

    // start move on selected items only once
    if (!m_selectionMoveStarted) {
        m_selectionMoveStarted = true;

        m_canvas->getModel()->startChangeSelection(
                m_segmentCopyMode ? CompositionModelImpl::ChangeCopy :
                                    CompositionModelImpl::ChangeMove);

        // The call to startChangeSelection() generates a new changing segment.
        // Get it.
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
    }

    // Display help for the Shift key.
    setContextHelpFor(pos, e->modifiers());

    Composition &comp = m_doc->getComposition();

    const int startDragTrackPos = m_canvas->grid().getYBin(m_clickPoint.y());
    const int currentTrackPos = m_canvas->grid().getYBin(pos.y());
    const int trackDiff = currentTrackPos - startDragTrackPos;

    CompositionModelImpl::ChangingSegmentSet &changingSegments =
            m_canvas->getModel()->getChangingSegments();

    // For each changing segment
    for (CompositionModelImpl::ChangingSegmentSet::iterator it =
                 changingSegments.begin();
         it != changingSegments.end();
         ++it) {
        ChangingSegmentPtr changingSegment = *it;

        const timeT newStartTime = m_canvas->grid().snapX(
                changingSegment->savedRect().x() + dx);

        const int newX = lround(
                m_canvas->grid().getRulerScale()->getXForTime(newStartTime));

        int newTrackPos = m_canvas->grid().getYBin(
                changingSegment->savedRect().y()) + trackDiff;

        // Limit to [0, comp.getNbTracks()-1].
        if (newTrackPos < 0)
            newTrackPos = 0;
        if (newTrackPos > static_cast<int>(comp.getNbTracks()) - 1)
            newTrackPos = comp.getNbTracks() - 1;

        const int newY = m_canvas->grid().getYBinCoordinate(newTrackPos);

        changingSegment->moveTo(newX, newY);
        m_changeMade = true;
    }

    if (m_changeMade) {
        // Make sure the segments are redrawn.
        // Note: The update() call below doesn't cause the segments to be
        //       redrawn.  It just updates from the cache.
        m_canvas->slotUpdateAll();
    }

    // Guides

    const int guideX = getChangingSegment()->rect().x();
    const int guideY = getChangingSegment()->rect().y();

    m_canvas->drawGuides(guideX, guideY);

    // Text Float

    const timeT guideTime = m_canvas->grid().snapX(guideX);

    RealTime time = comp.getElapsedRealTime(guideTime);
    QString msecs;
    msecs.sprintf("%03d", time.msec());

    int bar;
    int beat;
    int fraction;
    int remainder;
    comp.getMusicalTimeForAbsoluteTime(guideTime, bar, beat, fraction, remainder);

    QString posString = QString("%1.%2s (%3, %4, %5)").
            arg(time.sec).arg(msecs).arg(bar + 1).arg(beat).arg(fraction);

    m_canvas->drawTextFloat(guideX + 10, guideY - 30, posString);

    m_canvas->update();

    return RosegardenScrollView::FollowHorizontal |
           RosegardenScrollView::FollowVertical;
}

void SegmentSelector::keyPressEvent(QKeyEvent *e)
{
    // If another tool has taken over, delegate.
    if (m_dispatchTool) {
        m_dispatchTool->keyPressEvent(e);
        return;
    }

    // In case shift or ctrl were pressed, update the context help.
    setContextHelpFor(m_lastMousePos, e->modifiers());
}

void SegmentSelector::keyReleaseEvent(QKeyEvent *e)
{
    // If another tool has taken over, delegate.
    if (m_dispatchTool) {
        m_dispatchTool->keyReleaseEvent(e);
        return;
    }

    // In case shift or ctrl were released, update the context help.
    setContextHelpFor(m_lastMousePos, e->modifiers());
}

void SegmentSelector::setContextHelpFor(QPoint pos,
                                        Qt::KeyboardModifiers modifiers)
{
    // If we are moving something
    if (m_selectionMoveStarted)
    {
        const bool shift = ((modifiers & Qt::ShiftModifier) != 0);

        // If shift isn't being held down
        if (!shift) {
            setContextHelp(tr("Hold Shift to avoid snapping to beat grid"));
        } else {
            clearContextHelp();
        }

        return;
    }

    ChangingSegmentPtr segment = m_canvas->getModel()->getSegmentAt(pos);

    // If the mouse is hovering over the background
    if (!segment) {
        setContextHelp(tr("Click and drag to select segments; middle-click and drag to draw an empty segment"));
        return;
    }

    // The mouse is hovering over a segment.

    const bool ctrl = ((modifiers & Qt::ControlModifier) != 0);

    // If clicking would resize
    if (m_canvas->getModel()->getSelectedSegments().size() <= 1  &&
        isNearEdge(segment->rect(), pos)) {

        if (!ctrl) {
            setContextHelp(tr("Click and drag to resize a segment; hold Ctrl as well to rescale its contents"));
        } else {
            setContextHelp(tr("Click and drag to rescale segment"));
        }
    } else {  // clicking would move
        if (m_canvas->getModel()->haveMultipleSelection()) {
            if (!ctrl) {
                setContextHelp(tr("Click and drag to move segments; hold Ctrl as well to copy them; Ctrl + Alt for linked copies"));
            } else {
                setContextHelp(tr("Click and drag to copy segments"));
            }
        } else {
            if (!ctrl) {
                setContextHelp(tr("Click and drag to move segment; hold Ctrl as well to copy it; Ctrl + Alt for a linked copy; double-click to edit"));
            } else {
                setContextHelp(tr("Click and drag to copy segment"));
            }
        }
    }
}


}
