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

#include "base/Event.h"
#include "misc/Strings.h"
#include "misc/Debug.h"
#include "base/Composition.h"
#include "base/RealTime.h"
#include "base/SnapGrid.h"
#include "base/Selection.h"
#include "base/Track.h"
#include "commands/segment/SegmentQuickCopyCommand.h"
#include "commands/segment/SegmentQuickLinkCommand.h"
#include "commands/segment/SegmentReconfigureCommand.h"
#include "CompositionItemHelper.h"
#include "CompositionModelImpl.h"
#include "CompositionView.h"
#include "document/RosegardenDocument.h"
#include "misc/ConfigGroups.h"
#include "gui/general/BaseTool.h"
#include "gui/general/RosegardenScrollView.h"
#include "SegmentPencil.h"
#include "SegmentResizer.h"
#include "SegmentTool.h"
#include "SegmentToolBox.h"
#include <QApplication>
#include <QSettings>
#include <QCursor>
#include <QEvent>
#include <QPoint>
#include <QRect>
#include <QString>
#include <QMouseEvent>


namespace Rosegarden
{


const QString SegmentSelector::ToolName = "segmentselector";

SegmentSelector::SegmentSelector(CompositionView *c, RosegardenDocument *d)
        : SegmentTool(c, d),
        m_segmentAddMode(false),
        m_segmentCopyMode(false),
        m_segmentCopyingAsLink(false),
        m_segmentQuickCopyDone(false),
        m_buttonPressed(false),
        m_selectionMoveStarted(false),
        m_dispatchTool(0)
{
    RG_DEBUG << "SegmentSelector()\n";
}

SegmentSelector::~SegmentSelector()
{}

void SegmentSelector::ready()
{
    m_canvas->viewport()->setCursor(Qt::ArrowCursor);
    setContextHelp(tr("Click and drag to select segments"));
}

void SegmentSelector::stow()
{}

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

    m_buttonPressed = true;

    // Shift adds to the selection.
    m_segmentAddMode = ((e->modifiers() & Qt::ShiftModifier) != 0);
    // Ctrl is segment copy.
    m_segmentCopyMode = ((e->modifiers() & Qt::ControlModifier) != 0);
    // Alt+Ctrl is copy as link.
    m_segmentCopyingAsLink = (
            ((e->modifiers() & Qt::AltModifier) != 0) &&
            ((e->modifiers() & Qt::ControlModifier) != 0));

    QPoint pos = m_canvas->viewportToContents(e->pos());

    CompositionItemPtr item = m_canvas->getModel()->getFirstItemAt(pos);

    // If we're in segmentAddMode or not clicking on an item then we don't
    // clear the selection vector.  If we're clicking on an item and it's
    // not in the selection - then also clear the selection.
    //
    if ((!m_segmentAddMode && !item) ||
        (!m_segmentAddMode &&
         !(m_canvas->getModel()->isSelected(item->getSegment())))) {

        m_canvas->getModel()->clearSelected();
    }

    if (item) {

        // *** Resize Segment

        // Fifteen percent of the width of the SegmentItem, up to 10px
        //
        int threshold = int(float(item->rect().width()) * 0.15);
        if (threshold == 0) threshold = 1;
        if (threshold > 10) threshold = 10;

        bool start = false;

        // Resize if we're dragging from the edge, provided we aren't
        // in segment-add mode with at least one segment already
        // selected -- as we aren't able to resize multiple segments
        // at once, we should assume the segment-add aspect takes
        // priority

        if ((!m_segmentAddMode ||
             !m_canvas->getModel()->haveSelection()) &&
            SegmentResizer::cursorIsCloseEnoughToEdge(
                item, pos, threshold, start)) {

            SegmentResizer* resizer = dynamic_cast<SegmentResizer*>(
                getToolBox()->getTool(SegmentResizer::ToolName));

            resizer->setEdgeThreshold(threshold);

            // For the moment we only allow resizing of a single segment
            // at a time.
            //
            m_canvas->getModel()->clearSelected();
            m_canvas->getModel()->setSelected(item->getSegment());

            m_dispatchTool = resizer;

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

        //RG_DEBUG << "SegmentSelector::mousePressEvent - m_currentIndex = " << item << endl;
        m_currentIndex = item;
        m_clickPoint = pos;

        int guideX = item->rect().x();
        int guideY = item->rect().y();

        m_canvas->drawGuides(guideX, guideY);

        setSnapTime(e, SnapGrid::SnapToBeat);

    } else {

        // If middle button or Ctrl+left button
        if (e->button() == Qt::MidButton ||
            ((e->button() == Qt::LeftButton) && (e->modifiers() & Qt::ControlModifier))) {

            // Create a new segment with the SegmentPencil tool.

            m_dispatchTool = getToolBox()->getTool(SegmentPencil::ToolName);

            if (m_dispatchTool) {
                m_dispatchTool->ready(); // set mouse cursor
                m_dispatchTool->mousePressEvent(e);
            }

            return ;

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
    m_canvas->getModel()->signalSelection();

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

    m_buttonPressed = false;

    // Hide guides and stuff
    //
    m_canvas->hideGuides();
    m_canvas->hideTextFloat();

    if (m_dispatchTool) {
        m_dispatchTool->mouseReleaseEvent(e);
        m_dispatchTool = 0;
        m_canvas->viewport()->setCursor(Qt::ArrowCursor);
        return ;
    }

    int startDragTrackPos = m_canvas->grid().getYBin(m_clickPoint.y());
    int currentTrackPos = m_canvas->grid().getYBin(pos.y());
    int trackDiff = currentTrackPos - startDragTrackPos;

    if (!m_currentIndex) {
        m_canvas->hideSelectionRect();
        m_canvas->getModel()->finalizeSelectionRect();
        m_canvas->getModel()->signalSelection();
        return ;
    }

    m_canvas->viewport()->setCursor(Qt::ArrowCursor);

    Composition &comp = m_doc->getComposition();

    if (m_canvas->getModel()->isSelected(m_currentIndex->getSegment())) {

        CompositionModelImpl::ItemContainer& changingItems = m_canvas->getModel()->getChangingItems();
        CompositionModelImpl::ItemContainer::iterator it;

        if (changeMade()) {

            SegmentReconfigureCommand *command =
                new SegmentReconfigureCommand
                (tr("Move Segment(s)", "", m_selectedItems.size()), &comp);

            for (it = changingItems.begin();
                    it != changingItems.end();
                    ++it) {

                CompositionItemPtr item = *it;

                Segment* segment = CompositionItemHelper::getSegment(item);

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

                timeT itemStartTime = CompositionItemHelper::getStartTime
                    (item, m_canvas->grid());

                // We absolutely don't want to snap the end time to
                // the grid.  We want it to remain exactly the same as
                // it was, but relative to the new start time.
                timeT itemEndTime = itemStartTime + segment->getEndMarkerTime(false)
                                    - segment->getStartTime();

//                std::cerr << "releasing segment " << segment << ": mouse started at track " << startDragTrackPos << ", is now at " << currentTrackPos << ", diff is " << trackDiff << ", moving from track pos " << comp.getTrackPositionById(origTrackId) << " to " << trackPos << ", id " << origTrackId << " to " << newTrackId << std::endl;

                command->addSegment(segment,
                                    itemStartTime,
                                    itemEndTime,
                                    newTrackId);
            }

            addCommandToHistory(command);
        }

        m_canvas->getModel()->endChange();
        m_canvas->slotUpdateAll();
    }

    // if we've just finished a quick copy then drop the Z level back
    if (m_segmentQuickCopyDone) {
        m_segmentQuickCopyDone = false;
        //        m_currentIndex->setZ(2); // see SegmentItem::setSelected  --??
    }

    setChangeMade(false);

    m_selectionMoveStarted = false;

    m_currentIndex = CompositionItemPtr();

    setContextHelpFor(pos);
}

int
SegmentSelector::mouseMoveEvent(QMouseEvent *e)
{
    // No need to propagate.
    e->accept();

    QPoint pos = m_canvas->viewportToContents(e->pos());

    // ??? Does QMouseEvent offer a button state we can use instead of this?
    if (!m_buttonPressed) {
        setContextHelpFor(pos, (e->modifiers() & Qt::ControlModifier));
        return RosegardenScrollView::NoFollow;
    }

    if (m_dispatchTool) {
        return m_dispatchTool->mouseMoveEvent(e);
    }

    Composition &comp = m_doc->getComposition();

    if (!m_currentIndex) {

        // 	RG_DEBUG << "SegmentSelector::mouseMoveEvent: no current item\n";

        m_canvas->drawSelectionRectPos2(pos);

        m_canvas->getModel()->signalSelection();
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

        addCommandToHistory(mcommand);

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

    if (m_canvas->getModel()->isSelected(m_currentIndex->getSegment())) {

        // If shift isn't being held down
        if ((e->modifiers() & Qt::ShiftModifier) == 0) {
            setContextHelp(tr("Hold Shift to avoid snapping to beat grid"));
        } else {
            clearContextHelp();
        }

        // 	RG_DEBUG << "SegmentSelector::mouseMoveEvent: current item is selected\n";

        if (!m_selectionMoveStarted) { // start move on selected items only once
            m_canvas->getModel()->startChangeSelection(CompositionModelImpl::ChangeMove);
            m_selectionMoveStarted = true;
        }

        CompositionModelImpl::ItemContainer& changingItems = m_canvas->getModel()->getChangingItems();
        setCurrentIndex(CompositionItemHelper::findSiblingCompositionItem(changingItems, m_currentIndex));

        CompositionModelImpl::ItemContainer::iterator it;
        int guideX = 0;
        int guideY = 0;

        for (it = changingItems.begin();
                it != changingItems.end();
                ++it) {

            //             RG_DEBUG << "SegmentSelector::mouseMoveEvent() : movingItem at "
            //                      << (*it)->rect().x() << "," << (*it)->rect().y() << endl;

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

//            std::cerr << "segment " << *it << ": mouse started at track " << startDragTrackPos << ", is now at " << currentTrackPos << ", trackPos from " << trackPos << " to ";

            trackPos += trackDiff;

//            std::cerr << trackPos << std::endl;

            if (trackPos < 0) {
                trackPos = 0;
            } else if (trackPos >= (int)comp.getNbTracks()) {
                trackPos = comp.getNbTracks() - 1;
            }

            int newY = m_canvas->grid().getYBinCoordinate(trackPos);

            (*it)->moveTo(newX, newY);
            setChangeMade(true);
        }

        if (changeMade())
            m_canvas->getModel()->signalContentChange();

        guideX = m_currentIndex->rect().x();
        guideY = m_currentIndex->rect().y();

        m_canvas->drawGuides(guideX, guideY);

        timeT currentIndexStartTime = m_canvas->grid().snapX(m_currentIndex->rect().x());

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
        // 	RG_DEBUG << "SegmentSelector::mouseMoveEvent: current item not selected\n";
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

    CompositionItemPtr item = m_canvas->getModel()->getFirstItemAt(p);

    if (!item) {
        setContextHelp(tr("Click and drag to select segments; middle-click and drag to draw an empty segment"));

    } else {

        // Same logic as in mousePressEvent to establish
        // whether we'd be moving or resizing

        int threshold = int(float(item->rect().width()) * 0.15);
        if (threshold == 0) threshold = 1;
        if (threshold > 10) threshold = 10;
        bool start = false;

        if ((!m_segmentAddMode ||
             !m_canvas->getModel()->haveSelection()) &&
            SegmentResizer::cursorIsCloseEnoughToEdge(item, p,
                                                      threshold, start)) {
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
#include "SegmentSelector.moc"
