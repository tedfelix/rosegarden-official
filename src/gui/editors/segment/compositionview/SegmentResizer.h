
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

#ifndef RG_SEGMENTRESIZER_H
#define RG_SEGMENTRESIZER_H

#include "SegmentTool.h"
#include <QString>


class QPoint;
class QMouseEvent;
class ChangingSegmentPtr;


namespace Rosegarden
{

class RosegardenDocument;
class CompositionView;


/**
 * Segment Resizer tool. Allows resizing only at the end of the segment part
 */
class SegmentResizer : public SegmentTool
{
    Q_OBJECT

    friend class SegmentToolBox;
    friend class SegmentSelector;

public:

    void ready() override;
    void stow() override;

    void mousePressEvent(QMouseEvent *) override;
    int mouseMoveEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void keyPressEvent(QKeyEvent *) override;
    void keyReleaseEvent(QKeyEvent *) override;

    static QString ToolName();

protected:
    SegmentResizer(CompositionView*, RosegardenDocument*);
    void setContextHelp2(Qt::KeyboardModifiers modifiers = nullptr);

    void resizeAudioSegment(
            Segment *segment,
            double ratio,
            timeT newStartTime,
            timeT newEndTime);

    //--------------- Data members ---------------------------------

    bool m_resizeStart;
};


}

#endif
