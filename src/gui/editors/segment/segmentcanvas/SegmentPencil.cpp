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


#include "SegmentPencil.h"

#include "misc/Debug.h"
#include "misc/Strings.h"
#include "gui/general/ClefIndex.h"
#include "base/NotationTypes.h"
#include "base/Segment.h"
#include "base/SnapGrid.h"
#include "base/Track.h"
#include "commands/segment/SegmentInsertCommand.h"
#include "CompositionItemHelper.h"
#include "CompositionView.h"
#include "document/RosegardenGUIDoc.h"
#include "gui/general/BaseTool.h"
#include "gui/general/GUIPalette.h"
#include "gui/general/RosegardenCanvasView.h"
#include "SegmentTool.h"
#include <kcommand.h>
#include <klocale.h>
#include <qcursor.h>
#include <qevent.h>
#include <qpoint.h>
#include <qrect.h>
#include <qstring.h>


namespace Rosegarden
{

SegmentPencil::SegmentPencil(CompositionView *c, RosegardenGUIDoc *d)
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
    connect(m_canvas, SIGNAL(contentsMoving (int, int)),
            this, SLOT(slotCanvasScrolled(int, int)));
    setContextHelpFor(QPoint(0, 0));
}

void SegmentPencil::stow()
{
    disconnect(m_canvas, SIGNAL(contentsMoving (int, int)),
               this, SLOT(slotCanvasScrolled(int, int)));
}

void SegmentPencil::slotCanvasScrolled(int newX, int newY)
{
    QMouseEvent tmpEvent(QEvent::MouseMove,
                         m_canvas->viewport()->mapFromGlobal(QCursor::pos()) + QPoint(newX, newY),
                         Qt::NoButton, Qt::NoButton);
    handleMouseMove(&tmpEvent);
}

void SegmentPencil::handleMouseButtonPress(QMouseEvent *e)
{
    if (e->button() == RightButton)
        return;

    // is user holding Ctrl+Alt? (ugly, but we are running short on available
    // modifiers; Alt is grabbed by the window manager, and right clicking, my
    // (dmm) original idea, is grabbed by the context menu, so let's see how
    // this goes over
    bool pencilAnyway = (m_canvas->pencilOverExisting());

    m_newRect = false;

    // Check if mouse click was on a rect
    //
    CompositionItem item = m_canvas->getFirstItemAt(e->pos());

    // If user clicked a rect, and pencilAnyway is false, then there's nothing
    // left to do here
    if (item) {
        delete item;
        if (!pencilAnyway) return ;
    }

    // make new item
    //
    m_canvas->setSnapGrain(false);

    int trackPosition = m_canvas->grid().getYBin(e->pos().y());

    // Don't do anything if the user clicked beyond the track buttons
    //
    if (trackPosition >= m_doc->getComposition().getNbTracks())
        return ;

    Track *t = m_doc->getComposition().getTrackByPosition(trackPosition);
    if (!t)
        return ;

    TrackId trackId = t->getId();

    timeT time = int(nearbyint(m_canvas->grid().snapX(e->pos().x(), SnapGrid::SnapLeft)));
    timeT duration = int(nearbyint(m_canvas->grid().getSnapTime(double(e->pos().x()))));
    if (duration == 0)
        duration = Note(Note::Shortest).getDuration();

    int multiple = m_doc->getComposition()
        .getMaxContemporaneousSegmentsOnTrack(trackId);
    if (multiple < 1) multiple = 1;

    QRect tmpRect;
    tmpRect.setX(int(nearbyint(m_canvas->grid().getRulerScale()->getXForTime(time))));
    tmpRect.setY(m_canvas->grid().getYBinCoordinate(trackPosition) + 1);
    tmpRect.setHeight(m_canvas->grid().getYSnap() * multiple - 2);
    tmpRect.setWidth(int(nearbyint(m_canvas->grid().getRulerScale()->getWidthForDuration(time, duration))));

    m_canvas->setTmpRect(tmpRect,
                         GUIPalette::convertColour
                         (m_doc->getComposition().getSegmentColourMap().
                          getColourByIndex(t->getColor())));

    m_newRect = true;
    m_origPos = e->pos();

    m_canvas->updateContents(tmpRect);
}

void SegmentPencil::handleMouseButtonRelease(QMouseEvent* e)
{
    if (e->button() == RightButton)
        return ;

    setContextHelpFor(e->pos());

    if (m_newRect) {

        QRect tmpRect = m_canvas->getTmpRect();

        int trackPosition = m_canvas->grid().getYBin(tmpRect.y());
        Track *track = m_doc->getComposition().getTrackByPosition(trackPosition);
        timeT startTime = int(nearbyint(m_canvas->grid().getRulerScale()->getTimeForX(tmpRect.x()))),
                          endTime = int(nearbyint(m_canvas->grid().getRulerScale()->getTimeForX(tmpRect.x() + tmpRect.width())));

        //         RG_DEBUG << "SegmentPencil::handleMouseButtonRelease() : new segment with track id "
        //                  << track->getId() << endl;

        SegmentInsertCommand *command =
            new SegmentInsertCommand(m_doc, track->getId(),
                                     startTime, endTime);

        m_newRect = false;

        addCommandToHistory(command);

        // add the SegmentItem by hand, instead of allowing the usual
        // update mechanism to spot it.  This way we can select the
        // segment as we add it; otherwise we'd have no way to know
        // that the segment was created by this tool rather than by
        // e.g. a simple file load

        Segment *segment = command->getSegment();

        // add a clef to the start of the segment (tracks initialize to a
        // default of 0 for this property, so treble will be the default if it
        // is not specified elsewhere)
        segment->insert(clefIndexToClef(track->getClef()).getAsEvent
                        (segment->getStartTime()));
        segment->setTranspose(track->getTranspose());
        segment->setColourIndex(track->getColor());
        segment->setLowestPlayable(track->getLowestPlayable());
        segment->setHighestPlayable(track->getHighestPlayable());

        std::string label = qstrtostr(track->getPresetLabel());
        if (label != "") {
            segment->setLabel(qstrtostr(track->getPresetLabel()));
        }

        CompositionItem item = CompositionItemHelper::makeCompositionItem(segment);
        m_canvas->getModel()->clearSelected();
        m_canvas->getModel()->setSelected(item);
        m_canvas->getModel()->signalSelection();
        m_canvas->setTmpRect(QRect());
        m_canvas->slotUpdateSegmentsDrawBuffer();

    } else {

        m_newRect = false;
    }
}

int SegmentPencil::handleMouseMove(QMouseEvent *e)
{
    if (!m_newRect) {
        setContextHelpFor(e->pos());
        return RosegardenCanvasView::NoFollow;
    }

    if (!m_canvas->isFineGrain()) {
        setContextHelp(i18n("Hold Shift to avoid snapping to bar lines"));
    } else {
        clearContextHelp();
    }

    QRect tmpRect = m_canvas->getTmpRect();
    QRect oldTmpRect = tmpRect;

    m_canvas->setSnapGrain(false);

    SnapGrid::SnapDirection direction = SnapGrid::SnapRight;
    if (e->pos().x() <= m_origPos.x())
        direction = SnapGrid::SnapLeft;

    timeT snap = int(nearbyint(m_canvas->grid().getSnapTime(double(e->pos().x()))));
    if (snap == 0)
        snap = Note(Note::Shortest).getDuration();

    timeT time = int(nearbyint(m_canvas->grid().snapX(e->pos().x(), direction)));

    timeT startTime = int(nearbyint(m_canvas->grid().getRulerScale()->getTimeForX(tmpRect.x())));
    timeT endTime = int(nearbyint(m_canvas->grid().getRulerScale()->getTimeForX(tmpRect.x() + tmpRect.width())));

    if (direction == SnapGrid::SnapRight) {

        if (time >= startTime) {
            if ((time - startTime) < snap) {
                time = startTime + snap;
            }
        } else {
            if ((startTime - time) < snap) {
                time = startTime - snap;
            }
        }

        int w = int(nearbyint(m_canvas->grid().getRulerScale()->getWidthForDuration(startTime, time - startTime)));
        tmpRect.setWidth(w);

    } else { // SnapGrid::SnapLeft

        //             time += std::max(endTime - startTime, timeT(0));
        tmpRect.setX(int(m_canvas->grid().getRulerScale()->getXForTime(time)));

    }

    m_canvas->setTmpRect(tmpRect);
    return RosegardenCanvasView::FollowHorizontal;
}

void SegmentPencil::setContextHelpFor(QPoint p)
{
    int trackPosition = m_canvas->grid().getYBin(p.y());

    if (trackPosition < m_doc->getComposition().getNbTracks()) {
        Track *t = m_doc->getComposition().getTrackByPosition(trackPosition);
        if (t) {
            InstrumentId id = t->getInstrument();
            if (id >= AudioInstrumentBase && id < MidiInstrumentBase) {
                setContextHelp(i18n("Record or drop audio here"));
                return;
            }
        }
    }

    setContextHelp(i18n("Click and drag to draw an empty segment.  Control+Alt click and drag to draw in overlap mode."));
}

const QString SegmentPencil::ToolName   = "segmentpencil";

}
#include "SegmentPencil.moc"
