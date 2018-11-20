/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_MATRIXVELOCITY_H
#define RG_MATRIXVELOCITY_H

#include "MatrixTool.h"

#include "base/Event.h"

#include <QString>

namespace Rosegarden
{

class ViewElement;
class MatrixViewSegment;
class MatrixElement;
class Event;


class MatrixVelocity : public MatrixTool
{
    Q_OBJECT

    friend class MatrixToolBox;

public:
    void handleLeftButtonPress(const MatrixMouseEvent *) override;
    FollowMode handleMouseMove(const MatrixMouseEvent *) override;
    void handleMouseRelease(const MatrixMouseEvent *) override;

    static QString ToolName();

    /**
     * Respond to an event being deleted -- it may be the one the tool
     * is remembering as the current event.
     */
    void handleEventRemoved(Event *event) override;

    void ready() override;
    void stow() override;

protected:
    int m_mouseStartY;
    int m_velocityDelta;
    int m_screenPixelsScale; // Amount of screen pixels used for scale +-127 1:1 scale ratio
    double m_velocityScale;
    MatrixVelocity(MatrixWidget *);

    void setBasicContextHelp();

    MatrixElement *m_currentElement;
    MatrixViewSegment *m_currentViewSegment;

    bool m_start;   // Indicator of the start of a velocity change sequence
};


}

#endif
