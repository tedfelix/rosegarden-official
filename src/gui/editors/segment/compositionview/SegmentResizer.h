
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

    virtual void ready();
    virtual void stow();

    virtual void mousePressEvent(QMouseEvent *);
    virtual int mouseMoveEvent(QMouseEvent *);
    virtual void mouseReleaseEvent(QMouseEvent *);

    static const QString ToolName;

protected:
    SegmentResizer(CompositionView*, RosegardenDocument*);
    void setBasicContextHelp(bool ctrlPressed = false);

    //--------------- Data members ---------------------------------

    bool m_resizeStart;
};


}

#endif
