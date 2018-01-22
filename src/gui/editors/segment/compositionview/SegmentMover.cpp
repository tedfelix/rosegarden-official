/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
 
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
#include "CompositionModelImpl.h"
#include "CompositionView.h"
#include "document/RosegardenDocument.h"
#include "gui/general/BaseTool.h"
#include "gui/general/RosegardenScrollView.h"
#include "SegmentTool.h"
#include "SegmentToolBox.h"
#include "document/Command.h"
#include "document/CommandHistory.h"

#include <QCursor>
#include <QEvent>
#include <QPoint>
#include <QRect>
#include <QString>
#include <QMouseEvent>


namespace Rosegarden
{


QString SegmentMover::ToolName() { return "segmentmover"; }

SegmentMover::SegmentMover(CompositionView *c, RosegardenDocument *d)
        : SegmentTool(c, d),
        m_clickPoint(),
        m_changeMade(false)

{
    RG_DEBUG << "SegmentMover()\n";
}

void SegmentMover::ready()
{
    m_canvas->viewport()->setCursor(Qt::SizeAllCursor);
    setContextHelp2();
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

    CompositionModelImpl *model = m_canvas->getModel();

    // Get the Segment that was clicked.
    ChangingSegmentPtr changingSegment = model->getSegmentAt(pos);

    // If the segment canvas background was clicked, clear the selection.
    if (!changingSegment) {
        model->clearSelected();
        model->selectionHasChanged();
        m_canvas->update();
        return;
    }

    // The original segment in the composition.
    Segment *segment = changingSegment->getSegment();

    // If we're clicking on a segment that isn't in the selection
    if (!model->isSelected(segment)) {

        // Clear the selection.
        model->clearSelected();
        model->selectionHasChanged();
    }

    setChangingSegment(changingSegment);
    m_clickPoint = pos;

    setSnapTime(e, SnapGrid::SnapToBeat);

    int x = int(m_canvas->grid().getRulerScale()->getXForTime(
                    segment->getStartTime()));
    int y = m_canvas->grid().getYBinCoordinate(segment->getTrack());

    m_canvas->drawGuides(x, y);

    // If some segments are already selected
    if (model->haveSelection()) {

        // Start the move on all selected segments
        model->startChangeSelection(CompositionModelImpl::ChangeMove);

        // Find the clicked segment in the model.
        ChangingSegmentPtr newChangingSegment =
                model->findChangingSegment(segment);

        if (newChangingSegment) {
            // Toss the local "changing" segment since it isn't going to
            // be moving at all.  Swap it for the same changing segment in
            // the model.  That one *will* be moving and can be used to
            // drive the guides.
            setChangingSegment(newChangingSegment);
        }

    } else {  // Nothing selected, just move the one that was clicked
        model->startChange(changingSegment, CompositionModelImpl::ChangeMove);
    }

    m_canvas->update();

    setContextHelp2(e->modifiers());
}

void SegmentMover::mouseReleaseEvent(QMouseEvent *e)
{
    // We only care about the left mouse button.
    if (e->button() != Qt::LeftButton)
        return;

    // No need to propagate.
    e->accept();

    // If we weren't moving anything, bail.
    if (!getChangingSegment())
        return;

    if (m_changeMade) {

        QPoint pos = m_canvas->viewportToContents(e->pos());

        // Compute how far we've moved vertically.
        const int startTrackPos = m_canvas->grid().getYBin(m_clickPoint.y());
        const int currentTrackPos = m_canvas->grid().getYBin(pos.y());
        const int deltaTrack = currentTrackPos - startTrackPos;

        CompositionModelImpl::ChangingSegmentSet &changingSegments =
                m_canvas->getModel()->getChangingSegments();

        Composition &comp = m_doc->getComposition();

        SegmentReconfigureCommand *command =
            new SegmentReconfigureCommand(
                    changingSegments.size() == 1 ?
                            tr("Move Segment") :
                            tr("Move Segments"),
                    &comp);

        // For each changing segment
        for (CompositionModelImpl::ChangingSegmentSet::iterator it =
                 changingSegments.begin();
             it != changingSegments.end();
             ++it) {

            ChangingSegmentPtr changingSegment = *it;

            // The original Segment in the Composition.
            Segment *segment = changingSegment->getSegment();

            // New Track ID

            TrackId origTrackId = segment->getTrack();
            int trackPos = comp.getTrackPositionById(origTrackId) + deltaTrack;

            if (trackPos < 0) {
                trackPos = 0;
            } else if (trackPos >= (int)comp.getNbTracks()) {
                trackPos = comp.getNbTracks() - 1;
            }

            Track *newTrack = comp.getTrackByPosition(trackPos);

            int newTrackId = origTrackId;
            if (newTrack)
                newTrackId = newTrack->getId();

            // New start time

            timeT newStartTime = changingSegment->getStartTime(m_canvas->grid());

            // New end time

            // We absolutely don't want to snap the end time
            // to the grid.  We want it to remain exactly the same
            // as it was, but relative to the new start time.
            timeT newEndTime = newStartTime +
                    segment->getEndMarkerTime(false) - segment->getStartTime();

            // Add the changed segment to the command

            command->addSegment(segment,
                                newStartTime,
                                newEndTime,
                                newTrackId);
        }

        CommandHistory::getInstance()->addCommand(command);

        m_changeMade = false;
    }

    m_canvas->hideTextFloat();
    m_canvas->hideGuides();
    m_canvas->getModel()->endChange();
    m_canvas->slotUpdateAll();

    setChangingSegment(ChangingSegmentPtr());

    setContextHelp2();
}

int SegmentMover::mouseMoveEvent(QMouseEvent *e)
{
    // No need to propagate.
    e->accept();

    // If we aren't moving anything, bail.
    if (!getChangingSegment())
        return RosegardenScrollView::NoFollow;

    QPoint pos = m_canvas->viewportToContents(e->pos());

    setSnapTime(e, SnapGrid::SnapToBeat);

    setContextHelp2(e->modifiers());

    const SnapGrid &grid = m_canvas->grid();

    // Compute how far we've moved vertically.
    const int startTrackPos = grid.getYBin(m_clickPoint.y());
    const int currentTrackPos = grid.getYBin(pos.y());
    const int deltaTrack = currentTrackPos - startTrackPos;

    const int dx = pos.x() - m_clickPoint.x();

    Composition &comp = m_doc->getComposition();

    CompositionModelImpl::ChangingSegmentSet &changingSegments =
            m_canvas->getModel()->getChangingSegments();

    // For each changing segment, move it
    for (CompositionModelImpl::ChangingSegmentSet::iterator it =
             changingSegments.begin();
         it != changingSegments.end();
         ++it) {

        ChangingSegmentPtr changingSegment = *it;

        const timeT newStartTime = grid.snapX(changingSegment->savedRect().x() + dx);
        const int newX = int(grid.getRulerScale()->getXForTime(newStartTime));

        int trackPos = grid.getYBin(changingSegment->savedRect().y()) + deltaTrack;
        if (trackPos < 0)
            trackPos = 0;
        if (trackPos >= (int)comp.getNbTracks())
            trackPos = comp.getNbTracks() - 1;
        const int newY = grid.getYBinCoordinate(trackPos);

        changingSegment->moveTo(newX, newY);
        m_changeMade = true;
    }

    if (m_changeMade) {
        // Make sure the segments are redrawn.
        m_canvas->slotUpdateAll();
    }

    // Draw the guides

    int guideX = getChangingSegment()->rect().x();
    int guideY = getChangingSegment()->rect().y();

    m_canvas->drawGuides(guideX, guideY);

    // Format and draw the text float

    timeT guideTime = grid.snapX(guideX);

    RealTime time = comp.getElapsedRealTime(guideTime);

    int bar, beat, fraction, remainder;
    comp.getMusicalTimeForAbsoluteTime(guideTime, bar, beat, fraction, remainder);

    QString timeString = QString("%1.%2s (%3, %4, %5)")
        .arg(time.sec).arg(time.msec(), 3, 10, QChar('0'))
        .arg(bar + 1).arg(beat).arg(fraction);

    m_canvas->drawTextFloat(guideX + 10, guideY - 30, timeString);

	m_canvas->update();

    return RosegardenScrollView::FollowHorizontal |
           RosegardenScrollView::FollowVertical;
}

void SegmentMover::keyPressEvent(QKeyEvent *e)
{
    // In case shift was pressed, update the context help.
    setContextHelp2(e->modifiers());
}

void SegmentMover::keyReleaseEvent(QKeyEvent *e)
{
    // In case shift was released, update the context help.
    setContextHelp2(e->modifiers());
}

void SegmentMover::setContextHelp2(Qt::KeyboardModifiers modifiers)
{
    // If we're moving something
    if (getChangingSegment()) {
        bool shift = ((modifiers & Qt::ShiftModifier) != 0);

        // If shift isn't being held down
        if (!shift) {
            setContextHelp(tr("Hold Shift to avoid snapping to beat grid"));
        } else {
            clearContextHelp();
        }

        return;
    }

    setContextHelp(tr("Click and drag to move a segment"));
}


}
