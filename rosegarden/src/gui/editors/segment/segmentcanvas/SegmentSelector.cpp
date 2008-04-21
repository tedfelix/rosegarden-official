/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2008
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>
 
    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "SegmentSelector.h"

#include "base/Event.h"
#include <klocale.h>
#include "misc/Debug.h"
#include "base/Composition.h"
#include "base/RealTime.h"
#include "base/SnapGrid.h"
#include "base/Selection.h"
#include "base/Track.h"
#include "commands/segment/SegmentQuickCopyCommand.h"
#include "commands/segment/SegmentReconfigureCommand.h"
#include "CompositionItemHelper.h"
#include "CompositionModel.h"
#include "CompositionView.h"
#include "document/RosegardenGUIDoc.h"
#include "document/ConfigGroups.h"
#include "gui/general/BaseTool.h"
#include "gui/general/RosegardenCanvasView.h"
#include "SegmentPencil.h"
#include "SegmentResizer.h"
#include "SegmentTool.h"
#include "SegmentToolBox.h"
#include <kapplication.h>
#include <kconfig.h>
#include <qcursor.h>
#include <qevent.h>
#include <qpoint.h>
#include <qrect.h>
#include <qstring.h>


namespace Rosegarden
{

SegmentSelector::SegmentSelector(CompositionView *c, RosegardenGUIDoc *d)
        : SegmentTool(c, d),
        m_segmentAddMode(false),
        m_segmentCopyMode(false),
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
    m_canvas->viewport()->setCursor(Qt::arrowCursor);
    connect(m_canvas, SIGNAL(contentsMoving (int, int)),
            this, SLOT(slotCanvasScrolled(int, int)));
    setContextHelp(i18n("Click and drag to select segments"));
}

void SegmentSelector::stow()
{}

void SegmentSelector::slotCanvasScrolled(int newX, int newY)
{
    QMouseEvent tmpEvent(QEvent::MouseMove,
                         m_canvas->viewport()->mapFromGlobal(QCursor::pos()) + QPoint(newX, newY),
                         Qt::NoButton, Qt::NoButton);
    handleMouseMove(&tmpEvent);
}

void
SegmentSelector::handleMouseButtonPress(QMouseEvent *e)
{
    RG_DEBUG << "SegmentSelector::handleMouseButtonPress\n";
    m_buttonPressed = true;

    CompositionItem item = m_canvas->getFirstItemAt(e->pos());

    // If we're in segmentAddMode or not clicking on an item then we don't
    // clear the selection vector.  If we're clicking on an item and it's
    // not in the selection - then also clear the selection.
    //
    if ((!m_segmentAddMode && !item) ||
            (!m_segmentAddMode && !(m_canvas->getModel()->isSelected(item)))) {
        m_canvas->getModel()->clearSelected();
    }

    if (item) {

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
            SegmentResizer::cursorIsCloseEnoughToEdge(item, e->pos(), threshold, start)) {

            SegmentResizer* resizer =
                dynamic_cast<SegmentResizer*>(getToolBox()->getTool(SegmentResizer::ToolName));

            resizer->setEdgeThreshold(threshold);

            // For the moment we only allow resizing of a single segment
            // at a time.
            //
            m_canvas->getModel()->clearSelected();

            m_dispatchTool = resizer;

            m_dispatchTool->ready(); // set mouse cursor
            m_dispatchTool->handleMouseButtonPress(e);
            return ;
        }

        bool selecting = true;
        
        if (m_segmentAddMode && m_canvas->getModel()->isSelected(item)) {
            selecting = false;
        } else {
            // put the segment in 'move' mode only if it's being selected
            m_canvas->getModel()->startChange(item, CompositionModel::ChangeMove);                 
        }

        m_canvas->getModel()->setSelected(item, selecting);

        // Moving
        //
        //         RG_DEBUG << "SegmentSelector::handleMouseButtonPress - m_currentItem = " << item << endl;
        m_currentItem = item;
        m_clickPoint = e->pos();

        m_canvas->setGuidesPos(item->rect().topLeft());

        m_canvas->setDrawGuides(true);

    } else {


        // Add on middle button - bounding box on rest
        //
        if (e->button() == MidButton) {
            m_dispatchTool = getToolBox()->getTool(SegmentPencil::ToolName);

            if (m_dispatchTool) {
                m_dispatchTool->ready(); // set mouse cursor
                m_dispatchTool->handleMouseButtonPress(e);
            }

            return ;

        } else {

            m_canvas->setSelectionRectPos(e->pos());
            m_canvas->setDrawSelectionRect(true);
            if (!m_segmentAddMode)
                m_canvas->getModel()->clearSelected();

        }
    }

    // Tell the RosegardenGUIView that we've selected some new Segments -
    // when the list is empty we're just unselecting.
    //
    m_canvas->getModel()->signalSelection();

    m_passedInertiaEdge = false;
}

void
SegmentSelector::handleMouseButtonRelease(QMouseEvent *e)
{
    m_buttonPressed = false;

    // Hide guides and stuff
    //
    m_canvas->setDrawGuides(false);
    m_canvas->hideTextFloat();

    if (m_dispatchTool) {
        m_dispatchTool->handleMouseButtonRelease(e);
        m_dispatchTool = 0;
        m_canvas->viewport()->setCursor(Qt::arrowCursor);
        return ;
    }

    int startDragTrackPos = m_canvas->grid().getYBin(m_clickPoint.y());
    int currentTrackPos = m_canvas->grid().getYBin(e->pos().y());
    int trackDiff = currentTrackPos - startDragTrackPos;

    if (!m_currentItem) {
        m_canvas->setDrawSelectionRect(false);
        m_canvas->getModel()->finalizeSelectionRect();
        m_canvas->getModel()->signalSelection();
        return ;
    }

    m_canvas->viewport()->setCursor(Qt::arrowCursor);

    Composition &comp = m_doc->getComposition();

    if (m_canvas->getModel()->isSelected(m_currentItem)) {

        CompositionModel::itemcontainer& changingItems = m_canvas->getModel()->getChangingItems();
        CompositionModel::itemcontainer::iterator it;

        if (changeMade()) {

            SegmentReconfigureCommand *command =
                new SegmentReconfigureCommand
                (m_selectedItems.size() == 1 ? i18n("Move Segment") :
                 i18n("Move Segments"));

            for (it = changingItems.begin();
                    it != changingItems.end();
                    it++) {

                CompositionItem item = *it;

                Segment* segment = CompositionItemHelper::getSegment(item);

                TrackId origTrackId = segment->getTrack();
                int trackPos = comp.getTrackPositionById(origTrackId);
                trackPos += trackDiff;

                if (trackPos < 0) {
                    trackPos = 0;
                } else if (trackPos >= comp.getNbTracks()) {
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
                timeT itemEndTime = itemStartTime + segment->getEndMarkerTime()
                                    - segment->getStartTime();

                std::cerr << "releasing segment " << segment << ": mouse started at track " << startDragTrackPos << ", is now at " << currentTrackPos << ", diff is " << trackDiff << ", moving from track pos " << comp.getTrackPositionById(origTrackId) << " to " << trackPos << ", id " << origTrackId << " to " << newTrackId << std::endl;

                command->addSegment(segment,
                                    itemStartTime,
                                    itemEndTime,
                                    newTrackId);
            }

            addCommandToHistory(command);
        }

        m_canvas->getModel()->endChange();
        m_canvas->slotUpdateSegmentsDrawBuffer();
    }

    // if we've just finished a quick copy then drop the Z level back
    if (m_segmentQuickCopyDone) {
        m_segmentQuickCopyDone = false;
        //        m_currentItem->setZ(2); // see SegmentItem::setSelected  --??
    }

    setChangeMade(false);

    m_selectionMoveStarted = false;

    m_currentItem = CompositionItem();

    setContextHelpFor(e->pos());
}

int
SegmentSelector::handleMouseMove(QMouseEvent *e)
{
    if (!m_buttonPressed) {
        setContextHelpFor(e->pos(), (e->state() & Qt::ControlButton));
        return RosegardenCanvasView::NoFollow;
    }

    if (m_dispatchTool) {
        return m_dispatchTool->handleMouseMove(e);
    }

    Composition &comp = m_doc->getComposition();

    if (!m_currentItem) {

        // 	RG_DEBUG << "SegmentSelector::handleMouseMove: no current item\n";

        // do a bounding box
        QRect selectionRect = m_canvas->getSelectionRect();

        m_canvas->setDrawSelectionRect(true);

        // same as for notation view
        int w = int(e->pos().x() - selectionRect.x());
        int h = int(e->pos().y() - selectionRect.y());
        if (w > 0)
            ++w;
        else
            --w;
        if (h > 0)
            ++h;
        else
            --h;

        // Translate these points
        //
        m_canvas->setSelectionRectSize(w, h);

        m_canvas->getModel()->signalSelection();
        return RosegardenCanvasView::FollowHorizontal | RosegardenCanvasView::FollowVertical;
    }

    m_canvas->viewport()->setCursor(Qt::sizeAllCursor);

    if (m_segmentCopyMode && !m_segmentQuickCopyDone) {
        KMacroCommand *mcommand = new KMacroCommand
                                  (SegmentQuickCopyCommand::getGlobalName());

        SegmentSelection selectedItems = m_canvas->getSelectedSegments();
        SegmentSelection::iterator it;
        for (it = selectedItems.begin();
                it != selectedItems.end();
                it++) {
            SegmentQuickCopyCommand *command =
                new SegmentQuickCopyCommand(*it);

            mcommand->addCommand(command);
        }

        addCommandToHistory(mcommand);

        // generate SegmentItem
        //
        m_canvas->updateContents();
        m_segmentQuickCopyDone = true;
    }

    m_canvas->setSnapGrain(true);

    int startDragTrackPos = m_canvas->grid().getYBin(m_clickPoint.y());
    int currentTrackPos = m_canvas->grid().getYBin(e->pos().y());
    int trackDiff = currentTrackPos - startDragTrackPos;

    if (m_canvas->getModel()->isSelected(m_currentItem)) {

        if (!m_canvas->isFineGrain()) {
            setContextHelp(i18n("Hold Shift to avoid snapping to beat grid"));
        } else {
            clearContextHelp();
        }

        // 	RG_DEBUG << "SegmentSelector::handleMouseMove: current item is selected\n";

        if (!m_selectionMoveStarted) { // start move on selected items only once
            m_canvas->getModel()->startChangeSelection(CompositionModel::ChangeMove);
            m_selectionMoveStarted = true;
        }

        CompositionModel::itemcontainer& changingItems = m_canvas->getModel()->getChangingItems();
        setCurrentItem(CompositionItemHelper::findSiblingCompositionItem(changingItems, m_currentItem));

        CompositionModel::itemcontainer::iterator it;
        int guideX = 0;
        int guideY = 0;

        for (it = changingItems.begin();
                it != changingItems.end();
                ++it) {

            //             RG_DEBUG << "SegmentSelector::handleMouseMove() : movingItem at "
            //                      << (*it)->rect().x() << "," << (*it)->rect().y() << endl;

            int dx = e->pos().x() - m_clickPoint.x(),
                dy = e->pos().y() - m_clickPoint.y();

            const int inertiaDistance = m_canvas->grid().getYSnap() / 3;
            if (!m_passedInertiaEdge &&
                    (dx < inertiaDistance && dx > -inertiaDistance) &&
                    (dy < inertiaDistance && dy > -inertiaDistance)) {
                return RosegardenCanvasView::NoFollow;
            } else {
                m_passedInertiaEdge = true;
            }

            timeT newStartTime = m_canvas->grid().snapX((*it)->savedRect().x() + dx);

            int newX = int(m_canvas->grid().getRulerScale()->getXForTime(newStartTime));

            int trackPos = m_canvas->grid().getYBin((*it)->savedRect().y());

            std::cerr << "segment " << *it << ": mouse started at track " << startDragTrackPos << ", is now at " << currentTrackPos << ", trackPos from " << trackPos << " to ";

            trackPos += trackDiff;

            std::cerr << trackPos << std::endl;

            if (trackPos < 0) {
                trackPos = 0;
            } else if (trackPos >= comp.getNbTracks()) {
                trackPos = comp.getNbTracks() - 1;
            }

            int newY = m_canvas->grid().getYBinCoordinate(trackPos);

            (*it)->moveTo(newX, newY);
            setChangeMade(true);
        }

        if (changeMade())
            m_canvas->getModel()->signalContentChange();

        guideX = m_currentItem->rect().x();
        guideY = m_currentItem->rect().y();

        m_canvas->setGuidesPos(guideX, guideY);

        timeT currentItemStartTime = m_canvas->grid().snapX(m_currentItem->rect().x());

        RealTime time = comp.getElapsedRealTime(currentItemStartTime);
        QString ms;
        ms.sprintf("%03d", time.msec());

        int bar, beat, fraction, remainder;
        comp.getMusicalTimeForAbsoluteTime(currentItemStartTime, bar, beat, fraction, remainder);

        QString posString = QString("%1.%2s (%3, %4, %5)")
                            .arg(time.sec).arg(ms)
                            .arg(bar + 1).arg(beat).arg(fraction);

        m_canvas->setTextFloat(guideX + 10, guideY - 30, posString);
        m_canvas->updateContents();

    } else {
        // 	RG_DEBUG << "SegmentSelector::handleMouseMove: current item not selected\n";
    }

    return RosegardenCanvasView::FollowHorizontal | RosegardenCanvasView::FollowVertical;
}

void SegmentSelector::setContextHelpFor(QPoint p, bool ctrlPressed)
{
    kapp->config()->setGroup(GeneralOptionsConfigGroup);
    if (!kapp->config()->readBoolEntry("toolcontexthelp", true)) return;

    CompositionItem item = m_canvas->getFirstItemAt(p);

    if (!item) {
        setContextHelp(i18n("Click and drag to select segments; middle-click and drag to draw an empty segment"));

    } else {

        // Same logic as in handleMouseButtonPress to establish
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
                setContextHelp(i18n("Click and drag to resize a segment; hold Ctrl as well to rescale its contents"));
            } else {
                setContextHelp(i18n("Click and drag to rescale segment"));
            }
        } else {
            if (m_canvas->getModel()->haveMultipleSelection()) {
                if (!ctrlPressed) {
                    setContextHelp(i18n("Click and drag to move segments; hold Ctrl as well to copy them"));
                } else {
                    setContextHelp(i18n("Click and drag to copy segments"));
                }
            } else {
                if (!ctrlPressed) {
                    setContextHelp(i18n("Click and drag to move segment; hold Ctrl as well to copy it; double-click to edit"));
                } else {
                    setContextHelp(i18n("Click and drag to copy segment"));
                }
            }
        }
    }
}

const QString SegmentSelector::ToolName = "segmentselector";

}
#include "SegmentSelector.moc"
