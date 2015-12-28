
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

#ifndef RG_SEGMENTSELECTOR_H
#define RG_SEGMENTSELECTOR_H

#include "SegmentTool.h"

#include <QPoint>
#include <QString>

class QMouseEvent;

namespace Rosegarden
{


class RosegardenDocument;
class CompositionView;

/// The Arrow Tool
class SegmentSelector : public SegmentTool
{
    Q_OBJECT

public:
    static const QString ToolName;

    SegmentSelector(CompositionView *, RosegardenDocument *);
    virtual ~SegmentSelector();

    /// Called when this tool becomes the active tool.
    virtual void ready();
    virtual void stow()  { }

    virtual void mousePressEvent(QMouseEvent *);
    virtual int mouseMoveEvent(QMouseEvent *);
    virtual void mouseReleaseEvent(QMouseEvent *);

    // Unused
    //bool isSegmentAdding() const { return m_segmentAddMode; }
    //bool isSegmentCopying() const { return m_segmentCopyMode; }

private:
    void setContextHelpFor(QPoint p, bool ctrlPressed = false);

    //--------------- Data members ---------------------------------

    /// Recorded by mousePressEvent().
    QPoint m_clickPoint;

    /// Set on press, cleared on release, used by move.
    bool m_buttonPressed;

    /// Shift
    bool m_segmentAddMode;
    /// Ctrl
    bool m_segmentCopyMode;
    /// Alt+Ctrl
    bool m_segmentCopyingAsLink;

    /// The mouse has moved enough that we can start moving segments.
    bool m_passedInertiaEdge;

    /// Set to true after move makes copies.
    bool m_segmentQuickCopyDone;

    /// Whether we've notified CompositionModelImpl that things are changing.
    bool m_selectionMoveStarted;

    /// Set by mouse move when segments have moved.
    bool m_changeMade;

    /// Secondary tool for resizing or creating new segments.
    SegmentTool *m_dispatchTool;
};


}

#endif
