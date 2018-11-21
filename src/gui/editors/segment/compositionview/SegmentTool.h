
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

#ifndef RG_SEGMENTTOOL_H
#define RG_SEGMENTTOOL_H

#include "gui/general/RosegardenScrollView.h"
#include "gui/general/BaseTool.h"
#include "gui/general/ActionFileClient.h"
#include "ChangingSegment.h"
#include "CompositionModelImpl.h"
#include "base/TimeT.h"


class QMouseEvent;
class QKeyEvent;


namespace Rosegarden
{


class RosegardenDocument;
class CompositionView;


//////////////////////////////////////////////////////////////////////

/// Baseclass for the Segment canvas tools, e.g. SegmentPencil.
class SegmentTool : public BaseTool, public ActionFileClient
{
    Q_OBJECT

public:
    ~SegmentTool() override;

    /**
     * Is called by the parent View (e.g. CompositionView) when
     * the tool is set as current.
     * Add any setup here
     */
    void ready() override;

    /**
     * Derivers need to call this to make sure the right-click
     * context menu works.  See SegmentPencil::mousePressEvent().
     */
    virtual void mousePressEvent(QMouseEvent *);
    virtual int mouseMoveEvent(QMouseEvent *)
            { return RosegardenScrollView::NoFollow; }
    virtual void mouseReleaseEvent(QMouseEvent *)  { }
    virtual void keyPressEvent(QKeyEvent *)  { }
    virtual void keyReleaseEvent(QKeyEvent *)  { }

protected:
    /// Protected since a SegmentTool isn't very useful on its own.
    SegmentTool(CompositionView *, RosegardenDocument *);

    CompositionView *m_canvas;
    RosegardenDocument *m_doc;

    /// Set the Segment that the tool is working on.
    /**
     * The Segment tools use this to pass the changing Segment
     * between their mouse press, move, and release handlers.
     */
    void setChangingSegment(ChangingSegmentPtr changingSegment);
    ChangingSegmentPtr getChangingSegment()  { return m_changingSegment; }

    /// Sets the SnapGrid snap time based on the Shift key.
    void setSnapTime(QMouseEvent *e, timeT snapTime);

private:
    /// Right-click context menu.
    void createMenu() override;
    bool hasMenu() override { return true; }

    ChangingSegmentPtr m_changingSegment;

private slots:
    // This is just a mess of forwarding functions to RosegardenMainWindow.
    // Is there a better way to get the menu items to appear and to go to
    // RosegardenMainWindow?
    void slotEdit();
    void slotEditInMatrix();
    void slotEditInPercussionMatrix();
    void slotEditAsNotation();
    void slotEditInEventList();
    void slotEditInPitchTracker();
    void slotEditCut();
    void slotEditCopy();
    void slotEditPaste();
    void slotDeleteSelectedSegments();
    void slotJoinSegments();
    void slotQuantizeSelection();
    void slotRepeatQuantizeSelection();
    void slotRelabelSegments();
    void slotTransposeSegments();
    void slotPointerSelected();
    void slotMoveSelected();
    void slotDrawSelected();
    void slotEraseSelected();
    void slotResizeSelected();
    void slotSplitSelected();
};


}

#endif
