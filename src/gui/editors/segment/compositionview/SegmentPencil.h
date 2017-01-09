
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_SEGMENTPENCIL_H
#define RG_SEGMENTPENCIL_H

#include "base/Track.h"
#include "SegmentTool.h"
#include <QString>
#include "base/Event.h"


class QMouseEvent;
class QKeyEvent;


namespace Rosegarden
{

class RosegardenDocument;
class CompositionView;


//////////////////////////////

class SegmentPencil : public SegmentTool
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
    virtual void keyPressEvent(QKeyEvent *);
    virtual void keyReleaseEvent(QKeyEvent *);

    static QString ToolName();

protected:
    SegmentPencil(CompositionView*, RosegardenDocument*);
    void setContextHelpFor(QPoint pos, Qt::KeyboardModifiers modifiers = 0);

    //--------------- Data members ---------------------------------

    bool m_newRect;

    // X coord of the initial mouse button press
    int m_pressX;

    QPoint m_lastMousePos;

    // Start time of the segment as computed by press and move.
    timeT m_startTime;
    // End time of the segment as computed by press and move.
    timeT m_endTime;
};


}

#endif
