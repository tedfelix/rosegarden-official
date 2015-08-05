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

#define RG_MODULE_STRING "[SegmentMover]"

#include "SegmentMover.h"

#include "base/Event.h"
#include "misc/Debug.h"
#include "base/Composition.h"
#include "base/RealTime.h"
#include "base/Track.h"
#include "base/SnapGrid.h"
#include "commands/segment/SegmentReconfigureCommand.h"
#include "CompositionItemHelper.h"
#include "CompositionModelImpl.h"
#include "CompositionView.h"
#include "document/RosegardenDocument.h"
#include "gui/general/BaseTool.h"
#include "gui/general/RosegardenScrollView.h"
#include "SegmentTool.h"
#include "SegmentToolBox.h"
#include "SegmentSelector.h"
#include "document/Command.h"
#include <QCursor>
#include <QEvent>
#include <QPoint>
#include <QRect>
#include <QString>
#include <QMouseEvent>


namespace Rosegarden
{


const QString SegmentMover::ToolName = "segmentmover";

SegmentMover::SegmentMover(CompositionView *c, RosegardenDocument *d)
        : SegmentTool(c, d)
{
    RG_DEBUG << "SegmentMover()\n";
}

void SegmentMover::ready()
{
    m_canvas->viewport()->setCursor(Qt::SizeAllCursor);
    setBasicContextHelp();
}

void SegmentMover::stow()
{
}

void SegmentMover::mousePressEvent(QMouseEvent *e)
{
    // Let the baseclass have a go.
    SegmentTool::mousePressEvent(e);

    // We only care about the left mouse button.
    if (e->button() != Qt::LeftButton)
        return;

    // No need to propagate.
    e->accept();

    QPoint pos = m_canvas->viewportToContents(e->pos());

    CompositionItemPtr item = m_canvas->getModel()->getFirstItemAt(pos);
    SegmentSelector *selector =
            dynamic_cast<SegmentSelector *>(getToolBox()->getTool(SegmentSelector::ToolName));

    // #1027303: Segment move issue
    // Clear selection if we're clicking on an item that's not in it
    // and we're not in add mode

    if (selector &&
        item &&
        !m_canvas->getModel()->isSelected(item->getSegment()) &&
        !selector->isSegmentAdding()) {

        m_canvas->getModel()->clearSelected();
        m_canvas->getModel()->selectionHasChanged();
// 		m_canvas->updateContents();
		m_canvas->update();
	}

    if (item) {

        setCurrentIndex(item);
        m_clickPoint = pos;

        setSnapTime(e, SnapGrid::SnapToBeat);

        Segment* s = CompositionItemHelper::getSegment(m_currentIndex);

        int x = int(m_canvas->grid().getRulerScale()->getXForTime(s->getStartTime()));
        int y = int(m_canvas->grid().getYBinCoordinate(s->getTrack()));

        m_canvas->drawGuides(x, y);

        if (m_canvas->getModel()->haveSelection()) {
            RG_DEBUG << "SegmentMover::mousePressEvent() : haveSelection\n";
            // startChange on all selected segments
            m_canvas->getModel()->startChangeSelection(CompositionModelImpl::ChangeMove);


            CompositionModelImpl::ItemContainer& changingItems = m_canvas->getModel()->getChangingItems();
            // set m_currentIndex to its "sibling" among selected (now moving) items
            setCurrentIndex(CompositionItemHelper::findSiblingCompositionItem(changingItems, m_currentIndex));

        } else {
            RG_DEBUG << "SegmentMover::mousePressEvent() : no selection\n";
            m_canvas->getModel()->startChange(item, CompositionModelImpl::ChangeMove);
        }

// 		m_canvas->updateContents();
		m_canvas->update();

        m_passedInertiaEdge = false;

    } else {

        // check for addmode - clear the selection if not
        RG_DEBUG << "SegmentMover::mousePressEvent() : clear selection\n";
        m_canvas->getModel()->clearSelected();
        m_canvas->getModel()->selectionHasChanged();
// 		m_canvas->updateContents();
		m_canvas->update();
	}
}

void SegmentMover::mouseReleaseEvent(QMouseEvent *e)
{
    // We only care about the left mouse button.
    if (e->button() != Qt::LeftButton)
        return;

    // No need to propagate.
    e->accept();

    QPoint pos = m_canvas->viewportToContents(e->pos());

    Composition &comp = m_doc->getComposition();

    int startDragTrackPos = m_canvas->grid().getYBin(m_clickPoint.y());
    int currentTrackPos = m_canvas->grid().getYBin(pos.y());
    int trackDiff = currentTrackPos - startDragTrackPos;

    if (m_currentIndex) {

        if (changeMade()) {

            CompositionModelImpl::ItemContainer& changingItems = m_canvas->getModel()->getChangingItems();

            SegmentReconfigureCommand *command =
                new SegmentReconfigureCommand
                (changingItems.size() == 1 ? tr("Move Segment") : tr("Move Segments"), &comp);


            CompositionModelImpl::ItemContainer::iterator it;

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

                timeT newStartTime = CompositionItemHelper::getStartTime(item, m_canvas->grid());

                // We absolutely don't want to snap the end time
                // to the grid.  We want it to remain exactly the same
                // as it was, but relative to the new start time.
                timeT newEndTime = newStartTime + segment->getEndMarkerTime(false)
                                   - segment->getStartTime();

                command->addSegment(segment,
                                    newStartTime,
                                    newEndTime,
                                    newTrackId);
            }

            addCommandToHistory(command);
        }

        m_canvas->hideTextFloat();
        m_canvas->hideGuides();
        m_canvas->getModel()->endChange();
        m_canvas->slotUpdateAll();

    }

    setChangeMade(false);
    m_currentIndex = CompositionItemPtr();

    setBasicContextHelp();
}

int SegmentMover::mouseMoveEvent(QMouseEvent *e)
{
    // No need to propagate.
    e->accept();

    QPoint pos = m_canvas->viewportToContents(e->pos());

    setSnapTime(e, SnapGrid::SnapToBeat);

    Composition &comp = m_doc->getComposition();

    if (!m_currentIndex) {
        setBasicContextHelp();
        return RosegardenScrollView::NoFollow;
    }

    // If shift isn't being held down
    if ((e->modifiers() & Qt::ShiftModifier) == 0) {
        setContextHelp(tr("Hold Shift to avoid snapping to beat grid"));
    } else {
        clearContextHelp();
    }

    CompositionModelImpl::ItemContainer& changingItems = m_canvas->getModel()->getChangingItems();

    //         RG_DEBUG << "SegmentMover::mouseMoveEvent : nb changingItems = "
    //                  << changingItems.size() << endl;

    CompositionModelImpl::ItemContainer::iterator it;
    int guideX = 0;
    int guideY = 0;
    QRect updateRect;

    for (it = changingItems.begin();
         it != changingItems.end();
         ++it) {
        //             it->second->showRepeatRect(false);

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

        int startDragTrackPos = m_canvas->grid().getYBin(m_clickPoint.y());
        int currentTrackPos = m_canvas->grid().getYBin(pos.y());
        int trackDiff = currentTrackPos - startDragTrackPos;
        int trackPos = m_canvas->grid().getYBin((*it)->savedRect().y());

//        std::cerr << "segment " << *it << ": mouse started at track " << startDragTrackPos << ", is now at " << currentTrackPos << ", trackPos from " << trackPos << " to ";

        trackPos += trackDiff;

//        std::cerr << trackPos << std::endl;

        if (trackPos < 0) {
            trackPos = 0;
        } else if (trackPos >= (int)comp.getNbTracks()) {
            trackPos = comp.getNbTracks() - 1;
        }
/*!!!
        int newY = m_canvas->grid().snapY((*it)->savedRect().y() + dy);
        // Make sure we don't set a non-existing track
        if (newY < 0) {
            newY = 0;
        }
        int trackPos = m_canvas->grid().getYBin(newY);

        //             RG_DEBUG << "SegmentMover::mouseMoveEvent: orig y "
        //                      << (*it)->savedRect().y()
        //                      << ", dy " << dy << ", newY " << newY
        //                      << ", track " << track << endl;

        // Make sure we don't set a non-existing track (c'td)
        // TODO: make this suck less. Either the tool should
        // not allow it in the first place, or we automatically
        // create new tracks - might make undo very tricky though
        //
        if (trackPos >= comp.getNbTracks())
            trackPos = comp.getNbTracks() - 1;
*/
        int newY = m_canvas->grid().getYBinCoordinate(trackPos);

        //             RG_DEBUG << "SegmentMover::mouseMoveEvent: moving to "
        //                      << newX << "," << newY << endl;

        updateRect |= (*it)->rect();
        (*it)->moveTo(newX, newY);
        updateRect |= (*it)->rect();
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
// 	m_canvas->updateContents();
	m_canvas->update();

    return RosegardenScrollView::FollowHorizontal | RosegardenScrollView::FollowVertical;
}

void SegmentMover::setBasicContextHelp()
{
    setContextHelp(tr("Click and drag to move a segment"));
}


}
#include "SegmentMover.moc"
