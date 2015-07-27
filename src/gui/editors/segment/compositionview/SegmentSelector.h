
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


class SegmentSelector : public SegmentTool
{
    Q_OBJECT

    friend class SegmentToolBox;
    friend class SegmentTool;

public:

    virtual ~SegmentSelector();

    virtual void ready();
    virtual void stow();

    virtual void mousePressEvent(QMouseEvent *);
    virtual int mouseMoveEvent(QMouseEvent *);
    virtual void handleMouseButtonRelease(QMouseEvent*);

    bool isSegmentAdding() const { return m_segmentAddMode; }
    bool isSegmentCopying() const { return m_segmentCopyMode; }

    // Return the SegmentItem list for other tools to use
    //
    SegmentItemList* getSegmentItemList() { return &m_selectedItems; }

    static const QString ToolName;

protected:
    SegmentSelector(CompositionView*, RosegardenDocument*);

    void setContextHelpFor(QPoint p, bool ctrlPressed = false);

    //--------------- Data members ---------------------------------

    SegmentItemList m_selectedItems;

    bool m_segmentAddMode;
    bool m_segmentCopyMode;
    bool m_segmentCopyingAsLink;
    QPoint m_clickPoint;
    bool m_segmentQuickCopyDone;
    bool m_passedInertiaEdge;
    bool m_buttonPressed;
    bool m_selectionMoveStarted;

    SegmentTool *m_dispatchTool;
};



}

#endif
