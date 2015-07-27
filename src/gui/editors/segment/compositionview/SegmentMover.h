
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

#ifndef RG_SEGMENTMOVER_H
#define RG_SEGMENTMOVER_H

#include "SegmentTool.h"
#include <QPoint>
#include <QString>


class QMouseEvent;


namespace Rosegarden
{

class RosegardenDocument;
class CompositionView;


class SegmentMover : public SegmentTool
{
    Q_OBJECT

    friend class SegmentToolBox;

public:

    virtual void ready();
    virtual void stow();

    virtual void mousePressEvent(QMouseEvent *);
    virtual void handleMouseButtonRelease(QMouseEvent*);
    virtual int  handleMouseMove(QMouseEvent*);

    static const QString ToolName;

protected slots:
    void slotCanvasScrolled(int newX, int newY);

protected:
    SegmentMover(CompositionView*, RosegardenDocument*);

    void setBasicContextHelp();

    //--------------- Data members ---------------------------------

    QPoint            m_clickPoint;
    bool              m_passedInertiaEdge;
};


}

#endif
