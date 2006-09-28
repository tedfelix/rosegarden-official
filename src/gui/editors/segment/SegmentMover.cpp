/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2006
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


#include "SegmentMover.h"

#include "base/Event.h"
#include <klocale.h>
#include "misc/Debug.h"
#include "base/Composition.h"
#include "base/RealTime.h"
#include "base/Track.h"
#include "commands/segment/SegmentReconfigureCommand.h"
#include "CompositionItemHelper.h"
#include "CompositionModel.h"
#include "CompositionView.h"
#include "document/RosegardenGUIDoc.h"
#include "gui/general/BaseTool.h"
#include "gui/general/RosegardenCanvasView.h"
#include "SegmentTool.h"
#include <kcommand.h>
#include <qcursor.h>
#include <qevent.h>
#include <qpoint.h>
#include <qrect.h>
#include <qstring.h>


namespace Rosegarden
{

SegmentMover::SegmentMover(CompositionView *c, RosegardenGUIDoc *d)
        : SegmentTool(c, d)
{
    RG_DEBUG << "SegmentMover()\n";
}

void SegmentMover::ready()
{
    m_canvas->viewport()->setCursor(Qt::sizeAllCursor);
    connect(m_canvas, SIGNAL(contentsMoving (int, int)),
            this, SLOT(slotCanvasScrolled(int, int)));

}

void SegmentMover::stow()
{
    disconnect(m_canvas, SIGNAL(contentsMoving (int, int)),
               this, SLOT(slotCanvasScrolled(int, int)));
}

void SegmentMover::slotCanvasScrolled(int newX, int newY)
{
    QMouseEvent tmpEvent(QEvent::MouseMove,
                         m_canvas->viewport()->mapFromGlobal(QCursor::pos()) + QPoint(newX, newY),
                         Qt::NoButton, Qt::NoButton);
    handleMouseMove(&tmpEvent);
}

void SegmentMover::handleMouseButtonPress(QMouseEvent *e)
{
    CompositionItem item = m_canvas->getFirstItemAt(e->pos());
    SegmentSelector* selector = dynamic_cast<SegmentSelector*>
                                (getToolBox()->getTool("segmentselector"));

    // #1027303: Segment move issue
    // Clear selection if we're clicking on an item that's not in it
    // and we're not in add mode

    if (selector && item &&
            !m_canvas->getModel()->isSelected(item) && !selector->isSegmentAdding()) {
        m_canvas->getModel()->clearSelected();
        m_canvas->getModel()->signalSelection();
        m_canvas->updateContents();
    }

    if (item) {

        setCurrentItem(item);
        m_clickPoint = e->pos();
        Segment* s = CompositionItemHelper::getSegment(m_currentItem);

        int x = int(m_canvas->grid().getRulerScale()->getXForTime(s->getStartTime()));
        int y = int(m_canvas->grid().getYBinCoordinate(s->getTrack()));

        m_canvas->setGuidesPos(x, y);
        m_canvas->setDrawGuides(true);

        if (m_canvas->getModel()->haveSelection()) {
            RG_DEBUG << "SegmentMover::handleMouseButtonPress() : haveSelection\n";
            // startChange on all selected segments
            m_canvas->getModel()->startChangeSelection(CompositionModel::ChangeMove);


            CompositionModel::itemcontainer& changingItems = m_canvas->getModel()->getChangingItems();
            // set m_currentItem to its "sibling" among selected (now moving) items
            setCurrentItem(CompositionItemHelper::findSiblingCompositionItem(changingItems, m_currentItem));

        } else {
            RG_DEBUG << "SegmentMover::handleMouseButtonPress() : no selection\n";
            m_canvas->getModel()->startChange(item, CompositionModel::ChangeMove);
        }

        m_canvas->updateContents();

        m_passedInertiaEdge = false;

    } else {

        // check for addmode - clear the selection if not
        RG_DEBUG << "SegmentMover::handleMouseButtonPress() : clear selection\n";
        m_canvas->getModel()->clearSelected();
        m_canvas->getModel()->signalSelection();
        m_canvas->updateContents();
    }

}

void SegmentMover::handleMouseButtonRelease(QMouseEvent*)
{
    if (m_currentItem) {

        if (changeMade()) {

            CompositionModel::itemcontainer& changingItems = m_canvas->getModel()->getChangingItems();

            SegmentReconfigureCommand *command =
                new SegmentReconfigureCommand
                (changingItems.size() == 1 ? i18n("Move Segment") : i18n("Move Segments"));


            CompositionModel::itemcontainer::iterator it;

            for (it = changingItems.begin();
                    it != changingItems.end();
                    it++) {

                CompositionItem item = *it;

                Segment* segment = CompositionItemHelper::getSegment(item);
                int trackPos = m_canvas->grid().getYBin(item->rect().y());
                Track* track = m_doc->getComposition().getTrackByPosition(trackPos);
                TrackId itemTrackId = track->getId();
                timeT newStartTime = CompositionItemHelper::getStartTime(item, m_canvas->grid());

                // No -- we absolutely don't want to snap the end time
                // to the grid.  We want it to remain exactly the same
                // as it was, but relative to the new start time. --cc
                //                timeT newEndTime   = CompositionItemHelper::getEndTime(item, m_canvas->grid());
                timeT newEndTime = newStartTime + segment->getEndMarkerTime()
                                   - segment->getStartTime();

                command->addSegment(segment,
                                    newStartTime,
                                    newEndTime,
                                    itemTrackId);
            }

            addCommandToHistory(command);
        }

        m_canvas->hideTextFloat();
        m_canvas->setDrawGuides(false);
        m_canvas->getModel()->endChange();
        m_canvas->slotUpdateSegmentsDrawBuffer();

    }

    setChangeMade(false);
    m_currentItem = CompositionItem();
}

int SegmentMover::handleMouseMove(QMouseEvent *e)
{
    m_canvas->setSnapGrain(true);
    if (m_currentItem) {
        CompositionModel::itemcontainer& changingItems = m_canvas->getModel()->getChangingItems();

        //         RG_DEBUG << "SegmentMover::handleMouseMove : nb changingItems = "
        //                  << changingItems.size() << endl;

        CompositionModel::itemcontainer::iterator it;
        int guideX = 0;
        int guideY = 0;
        QRect updateRect;

        for (it = changingItems.begin();
                it != changingItems.end();
                it++) {
            //             it->second->showRepeatRect(false);

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
            int newY = m_canvas->grid().snapY((*it)->savedRect().y() + dy);
            // Make sure we don't set a non-existing track
            if (newY < 0) {
                newY = 0;
            }
            int trackPos = m_canvas->grid().getYBin(newY);

            //             RG_DEBUG << "SegmentMover::handleMouseMove: orig y "
            //                      << (*it)->savedRect().y()
            //                      << ", dy " << dy << ", newY " << newY
            //                      << ", track " << track << endl;

            // Make sure we don't set a non-existing track (c'td)
            // TODO: make this suck less. Either the tool should
            // not allow it in the first place, or we automatically
            // create new tracks - might make undo very tricky though
            //
            if (trackPos >= m_doc->getComposition().getNbTracks())
                trackPos = m_doc->getComposition().getNbTracks() - 1;

            newY = m_canvas->grid().getYBinCoordinate(trackPos);

            //             RG_DEBUG << "SegmentMover::handleMouseMove: moving to "
            //                      << newX << "," << newY << endl;

            updateRect |= (*it)->rect();
            (*it)->moveTo(newX, newY);
            updateRect |= (*it)->rect();
            setChangeMade(true);
        }

        if (changeMade())
            m_canvas->getModel()->signalContentChange();

        guideX = m_currentItem->rect().x();
        guideY = m_currentItem->rect().y();

        m_canvas->setGuidesPos(guideX, guideY);

        timeT currentItemStartTime = m_canvas->grid().snapX(m_currentItem->rect().x());

        Composition &comp = m_doc->getComposition();
        RealTime time =
            comp.getElapsedRealTime(currentItemStartTime);
        QString ms;
        ms.sprintf("%03d", time.msec());

        int bar, beat, fraction, remainder;
        comp.getMusicalTimeForAbsoluteTime(currentItemStartTime, bar, beat, fraction, remainder);

        QString posString = QString("%1.%2s (%3, %4, %5)")
                            .arg(time.sec).arg(ms)
                            .arg(bar + 1).arg(beat).arg(fraction);

        m_canvas->setTextFloat(guideX + 10, guideY - 30, posString);
        m_canvas->updateContents();

        return RosegardenCanvasView::FollowHorizontal | RosegardenCanvasView::FollowVertical;
    }

    return RosegardenCanvasView::NoFollow;
}

}
