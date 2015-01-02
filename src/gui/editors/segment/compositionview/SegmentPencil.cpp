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
#include "document/RosegardenDocument.h"
#include "gui/general/BaseTool.h"
#include "gui/general/GUIPalette.h"
#include "gui/general/RosegardenScrollView.h"
#include "SegmentTool.h"
#include "document/Command.h"
#include <QCursor>
#include <QEvent>
#include <QPoint>
#include <QRect>
#include <QString>
#include <QMouseEvent>

namespace Rosegarden
{

SegmentPencil::SegmentPencil(CompositionView *c, RosegardenDocument *d)
        : SegmentTool(c, d),
        m_newRect(false),
        m_pressX(0),
        m_startTime(0),
        m_endTime(0)
{
//RG_DEBUG << "SegmentPencil()\n";
}

void SegmentPencil::ready()
{
    m_canvas->viewport()->setCursor(Qt::IBeamCursor);
    //connect(m_canvas, SIGNAL(contentsMoving (int, int)),
    //        this, SLOT(slotCanvasScrolled(int, int)));
    setContextHelpFor(QPoint(0, 0));
}

void SegmentPencil::stow()
{
    //disconnect(m_canvas, SIGNAL(contentsMoving (int, int)),
    //           this, SLOT(slotCanvasScrolled(int, int)));
}

void SegmentPencil::slotCanvasScrolled(int newX, int newY)
{
    Qt::MouseButton button = Qt::NoButton;
    Qt::MouseButtons buttons = Qt::NoButton;
    Qt::KeyboardModifiers modifiers = 0;
    QMouseEvent tmpEvent(QEvent::MouseMove,
                         m_canvas->viewport()->mapFromGlobal(QCursor::pos()) + QPoint(newX, newY), button, buttons, modifiers);
    handleMouseMove(&tmpEvent);
}

void SegmentPencil::handleMouseButtonPress(QMouseEvent *e)
{
    if (e->button() != Qt::LeftButton)
        return;

    // is user holding Ctrl+Alt? (ugly, but we are running short on available
    // modifiers; Alt is grabbed by the window manager, and right clicking, my
    // (dmm) original idea, is grabbed by the context menu, so let's see how
    // this goes over
    bool pencilAnyway = (m_canvas->pencilOverExisting());

    m_newRect = false;

    // Check if mouse click was on a rect
    //
    CompositionItemPtr item = m_canvas->getFirstItemAt(e->pos());

    // If user clicked a rect, and pencilAnyway is false, then there's nothing
    // left to do here
    if (item) {
        delete item;
        if (!pencilAnyway) return ;
    }

    // make new item

    // Switch to coarse-grain snap resolution.
    m_canvas->setSnapGrain(false);

    SnapGrid &snapGrid = m_canvas->grid();
    
    int trackPosition = snapGrid.getYBin(e->pos().y());

    // Don't do anything if the user clicked beyond the track buttons
    //
    if (trackPosition >= (int)m_doc->getComposition().getNbTracks())
        return ;

    Track *t = m_doc->getComposition().getTrackByPosition(trackPosition);
    if (!t)
        return ;

    TrackId trackId = t->getId();

    // Save the mouse X as the original Press point
    m_pressX = e->pos().x();

    m_startTime = snapGrid.snapX(m_pressX, SnapGrid::SnapLeft);
    m_endTime = snapGrid.snapX(m_pressX, SnapGrid::SnapRight);

    // Don't allow a length smaller than the smallest note
    if (m_endTime - m_startTime < Note(Note::Shortest).getDuration())
        m_endTime = m_startTime + Note(Note::Shortest).getDuration();

    int multiple = 
        m_doc->getComposition().getMaxContemporaneousSegmentsOnTrack(trackId);
    if (multiple < 1)
        multiple = 1;

    QRect tmpRect;
    tmpRect.setLeft(lround(snapGrid.getRulerScale()->
                               getXForTime(m_startTime)));
    tmpRect.setRight(lround(snapGrid.getRulerScale()->
                                getXForTime(m_endTime)));
    tmpRect.setY(snapGrid.getYBinCoordinate(trackPosition) + 1);
    tmpRect.setHeight(snapGrid.getYSnap() * multiple - 2);

    m_canvas->setTmpRect(tmpRect,
                         GUIPalette::convertColour
                         (m_doc->getComposition().getSegmentColourMap().
                          getColourByIndex(t->getColor())));

    m_newRect = true;

	m_canvas->update(tmpRect);
}

void SegmentPencil::handleMouseButtonRelease(QMouseEvent* e)
{
    if (e->button() != Qt::LeftButton)
        return ;

    setContextHelpFor(e->pos());

    if (m_newRect) {

        QRect tmpRect = m_canvas->getTmpRect();

        int trackPosition = m_canvas->grid().getYBin(tmpRect.y());
        Track *track = 
            m_doc->getComposition().getTrackByPosition(trackPosition);

//RG_DEBUG << "SegmentPencil::handleMouseButtonRelease():";
//RG_DEBUG << "  m_startTime = " << m_startTime;
//RG_DEBUG << "  m_endTime = " << m_endTime;
//RG_DEBUG << "  track id = " << track->getId();

        SegmentInsertCommand *command =
            new SegmentInsertCommand(m_doc, track->getId(),
                                     m_startTime, m_endTime);

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

        //!!! Should not a default key be inserted here equally in order to
        //    have the "hide redundant clefs and keys" mechanism working
        //    on the segments using the default key ?

        segment->setTranspose(track->getTranspose());
        segment->setColourIndex(track->getColor());
        segment->setLowestPlayable(track->getLowestPlayable());
        segment->setHighestPlayable(track->getHighestPlayable());

        std::string label = track->getPresetLabel();
        if (label != "") {
            segment->setLabel( track->getPresetLabel().c_str() );
        }

        CompositionItemPtr item = CompositionItemHelper::makeCompositionItem(segment);
        m_canvas->getModel()->clearSelected();
        m_canvas->getModel()->setSelected(item);
        m_canvas->getModel()->signalSelection();

        m_canvas->setTmpRect(QRect());
        m_canvas->slotUpdateAll();

    }
}

int SegmentPencil::handleMouseMove(QMouseEvent *e)
{
    if (!m_newRect) {
        setContextHelpFor(e->pos());
        return RosegardenScrollView::NoFollow;
    }

    if (!m_canvas->isFineGrain()) {
        setContextHelp(tr("Hold Shift to avoid snapping to bar lines"));
    } else {
        clearContextHelp();
    }

    QRect tmpRect = m_canvas->getTmpRect();

    // Switch to coarse-grain snap resolution.
    m_canvas->setSnapGrain(false);

    SnapGrid &snapGrid = m_canvas->grid();

    int mouseX = e->pos().x();
    
    // if mouse X is to the right of the original Press point
    if (mouseX >= m_pressX) {
        m_startTime = snapGrid.snapX(m_pressX, SnapGrid::SnapLeft);
        m_endTime = snapGrid.snapX(mouseX, SnapGrid::SnapRight);

        // Make sure the segment is never smaller than the smallest note.
        if (m_endTime - m_startTime < Note(Note::Shortest).getDuration())
            m_endTime = m_startTime + Note(Note::Shortest).getDuration();
    } else {  // we are to the left of the original Press point
        m_startTime = snapGrid.snapX(mouseX, SnapGrid::SnapLeft);
        m_endTime = snapGrid.snapX(m_pressX, SnapGrid::SnapRight);

        // Make sure the segment is never smaller than the smallest note.
        if (m_endTime - m_startTime < Note(Note::Shortest).getDuration())
            m_startTime = m_endTime - Note(Note::Shortest).getDuration();
    }

    int leftX = snapGrid.getRulerScale()->getXForTime(m_startTime);
    int rightX = snapGrid.getRulerScale()->getXForTime(m_endTime);

    // Adjust the rectangle to go from leftX to rightX
    tmpRect.setLeft(leftX);
    tmpRect.setRight(rightX);

    m_canvas->setTmpRect(tmpRect);
    return RosegardenScrollView::FollowHorizontal;
}

void SegmentPencil::setContextHelpFor(QPoint p)
{
    int trackPosition = m_canvas->grid().getYBin(p.y());

    if (trackPosition < (int)m_doc->getComposition().getNbTracks()) {
        Track *t = m_doc->getComposition().getTrackByPosition(trackPosition);
        if (t) {
            InstrumentId id = t->getInstrument();
            if (id >= AudioInstrumentBase && id < MidiInstrumentBase) {
                setContextHelp(tr("Record or drop audio here"));
                return;
            }
        }
    }

    setContextHelp(tr("Click and drag to draw an empty segment.  Control+Alt click and drag to draw in overlap mode."));
}

const QString SegmentPencil::ToolName   = "segmentpencil";

}
#include "SegmentPencil.moc"
