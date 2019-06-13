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


#include "SegmentPencil.h"

#include "misc/Debug.h"
#include "misc/Strings.h"
#include "gui/general/ClefIndex.h"
#include "base/NotationTypes.h"
#include "base/Segment.h"
#include "base/SnapGrid.h"
#include "base/Track.h"
#include "commands/segment/SegmentInsertCommand.h"
#include "CompositionView.h"
#include "document/RosegardenDocument.h"
#include "gui/general/BaseTool.h"
#include "gui/general/GUIPalette.h"
#include "gui/general/RosegardenScrollView.h"
#include "SegmentTool.h"
#include "document/Command.h"
#include "document/CommandHistory.h"

#include <QCursor>
#include <QEvent>
#include <QPoint>
#include <QRect>
#include <QString>
#include <QMouseEvent>
#include <QKeyEvent>

namespace Rosegarden
{

QString SegmentPencil::ToolName() { return "segmentpencil"; }

SegmentPencil::SegmentPencil(CompositionView *c, RosegardenDocument *d)
        : SegmentTool(c, d),
        m_newRect(false),
        m_pressX(0),
        m_lastMousePos(),
        m_startTime(0),
        m_endTime(0)
{
//RG_DEBUG << "SegmentPencil()\n";
}

void SegmentPencil::ready()
{
    m_canvas->viewport()->setCursor(Qt::IBeamCursor);
    setContextHelpFor(QPoint(0, 0));
}

void SegmentPencil::stow()
{
}

void SegmentPencil::mousePressEvent(QMouseEvent *e)
{
    // Let the baseclass have a go.
    SegmentTool::mousePressEvent(e);

    // We only care about the left and middle mouse buttons.
    // SegmentSelector might send us a middle press.
    if (e->button() != Qt::LeftButton  &&
        e->button() != Qt::MidButton)
        return;

    // No need to propagate.
    e->accept();

    // is user holding Ctrl+Alt? (ugly, but we are running short on available
    // modifiers; Alt is grabbed by the window manager, and right clicking, my
    // (dmm) original idea, is grabbed by the context menu, so let's see how
    // this goes over
    // ??? Why not just set this to true?  The use case is starting a
    //     pencil click/drag on top of an existing segment.  If the
    //     user wants to draw a segment on top of a segment, just let
    //     them.  Maybe this was an issue when segments could overlap?
    bool pencilAnyway = ((e->modifiers() & Qt::AltModifier) != 0  &&
                         (e->modifiers() & Qt::ControlModifier) != 0);

    m_newRect = false;

    QPoint pos = m_canvas->viewportToContents(e->pos());

    // Check if mouse click was on a rect
    //
    ChangingSegmentPtr item = m_canvas->getModel()->getSegmentAt(pos);

    // If user clicked a rect, and pencilAnyway is false, then there's nothing
    // left to do here
    if (item) {
        if (!pencilAnyway) return ;
    }

    // make new item

    SnapGrid &snapGrid = m_canvas->grid();
    
    setSnapTime(e, SnapGrid::SnapToBar);

    int trackPosition = snapGrid.getYBin(pos.y());

    // Don't do anything if the user clicked beyond the track buttons
    //
    if (trackPosition >= (int)m_doc->getComposition().getNbTracks())
        return ;

    Track *t = m_doc->getComposition().getTrackByPosition(trackPosition);
    if (!t)
        return ;

    TrackId trackId = t->getId();

    // Save the mouse X as the original Press point
    m_pressX = pos.x();

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

    m_canvas->setNewSegmentColor(GUIPalette::convertColour(
            m_doc->getComposition().getSegmentColourMap().
            getColourByIndex(t->getColor())));
    m_canvas->drawNewSegment(tmpRect);

    m_newRect = true;

    setContextHelpFor(pos, e->modifiers());

    m_canvas->update(tmpRect);
}

void SegmentPencil::mouseReleaseEvent(QMouseEvent *e)
{
    // Have to allow middle button for SegmentSelector's middle
    // button feature to work.
    if (e->button() != Qt::LeftButton  &&
        e->button() != Qt::MidButton)
        return;

    // No need to propagate.
    e->accept();

    QPoint pos = m_canvas->viewportToContents(e->pos());

    if (m_newRect) {

        QRect tmpRect = m_canvas->getNewSegmentRect();

        int trackPosition = m_canvas->grid().getYBin(tmpRect.y());
        Track *track = 
            m_doc->getComposition().getTrackByPosition(trackPosition);

        SegmentInsertCommand *command =
            new SegmentInsertCommand(m_doc, track->getId(),
                                     m_startTime, m_endTime);

        m_newRect = false;

        CommandHistory::getInstance()->addCommand(command);

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

        m_canvas->getModel()->clearSelected();
        m_canvas->getModel()->setSelected(segment);
        m_canvas->getModel()->selectionHasChanged();

        m_canvas->hideNewSegment();
        m_canvas->slotUpdateAll();
    }

    setContextHelpFor(pos, e->modifiers());
}

int SegmentPencil::mouseMoveEvent(QMouseEvent *e)
{
    // No need to propagate.
    e->accept();

    QPoint pos = m_canvas->viewportToContents(e->pos());
    m_lastMousePos = pos;

    if (!m_newRect) {
        setContextHelpFor(pos);
        return NO_FOLLOW;
    }

    // Display help for the Shift key.
    setContextHelpFor(pos, e->modifiers());

    QRect tmpRect = m_canvas->getNewSegmentRect();

    SnapGrid &snapGrid = m_canvas->grid();

    setSnapTime(e, SnapGrid::SnapToBar);

    int mouseX = pos.x();
    
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

    m_canvas->drawNewSegment(tmpRect);
    return FOLLOW_HORIZONTAL;
}

void SegmentPencil::keyPressEvent(QKeyEvent *e)
{
    // In case shift was pressed, update the context help.
    setContextHelpFor(m_lastMousePos, e->modifiers());
}

void SegmentPencil::keyReleaseEvent(QKeyEvent *e)
{
    // In case shift was released, update the context help.
    setContextHelpFor(m_lastMousePos, e->modifiers());
}

void SegmentPencil::setContextHelpFor(QPoint pos,
                                      Qt::KeyboardModifiers modifiers)
{
    // if we're drawing a segment...
    if (m_newRect)
    {
        const bool shift = ((modifiers & Qt::ShiftModifier) != 0);

        // If shift isn't being held down
        if (!shift)
            setContextHelp(tr("Hold Shift to avoid snapping to bar lines"));
        else
            clearContextHelp();

        return;
    }

    // Mouse is hovering.

    // Audio Track

    int trackPosition = m_canvas->grid().getYBin(pos.y());

    if (trackPosition < (int)m_doc->getComposition().getNbTracks()) {
        Track *track = m_doc->getComposition().getTrackByPosition(trackPosition);
        if (track) {
            InstrumentId id = track->getInstrument();
            // If this is an audio instrument
            if (id >= AudioInstrumentBase  &&  id < MidiInstrumentBase) {
                setContextHelp(tr("Record or drop audio here"));
                return;
            }
        }
    }

    // MIDI Track

    setContextHelp(tr("Click and drag to draw an empty segment.  Control+Alt click and drag to draw in overlap mode."));
}

}
