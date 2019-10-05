
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

#ifndef RG_SEGMENTSPLITTER_H
#define RG_SEGMENTSPLITTER_H

#include "SegmentTool.h"
#include <QString>
#include "base/Event.h"


class QMouseEvent;


namespace Rosegarden
{

class Segment;
class RosegardenDocument;
class CompositionView;


class SegmentSplitter : public SegmentTool
{
    Q_OBJECT

    friend class SegmentToolBox;

public:

    ~SegmentSplitter() override;

    void ready() override;

    void mousePressEvent(QMouseEvent *) override;
    int mouseMoveEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;

    // don't do double clicks
    virtual void contentsMouseDoubleClickEvent(QMouseEvent*);

    static QString ToolName();

protected:
    SegmentSplitter(CompositionView*, RosegardenDocument*);
    
    void drawSplitLine(QMouseEvent*);
    void splitSegment(Segment *segment,
                      timeT &splitTime);

    bool m_enableEditingDuringPlayback;

    // ??? unused
    int m_prevX;
    // ??? unused
    int m_prevY;
};


}

#endif
