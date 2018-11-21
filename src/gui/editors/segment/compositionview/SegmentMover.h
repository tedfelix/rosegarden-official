
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
    void ready() override;
    void stow() override;

    void mousePressEvent(QMouseEvent *) override;
    int mouseMoveEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void keyPressEvent(QKeyEvent *) override;
    void keyReleaseEvent(QKeyEvent *) override;

    static QString ToolName();

protected:
    SegmentMover(CompositionView*, RosegardenDocument*);

    void setContextHelp2(Qt::KeyboardModifiers modifiers = nullptr);

    QPoint m_clickPoint;
    bool m_changeMade;
};


}

#endif
