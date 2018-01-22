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

#ifndef RG_MATRIXPAINTER_H
#define RG_MATRIXPAINTER_H

#include "MatrixTool.h"
#include <QString>
#include "base/Event.h"


namespace Rosegarden
{

class ViewElement;
class MatrixViewSegment;
class MatrixElement;
class Event;


class MatrixPainter : public MatrixTool
{
    Q_OBJECT

    friend class MatrixToolBox;

public:
    virtual void handleLeftButtonPress(const MatrixMouseEvent *);
    virtual void handleMouseDoubleClick(const MatrixMouseEvent *);
    virtual void handleMidButtonPress(const MatrixMouseEvent *);
    virtual FollowMode handleMouseMove(const MatrixMouseEvent *);
    virtual void handleMouseRelease(const MatrixMouseEvent *);

    virtual void ready();
    virtual void stow();

    static QString ToolName();

public slots:
    /**
     * Respond to an event being deleted -- it may be the one the tool
     * is remembering as the current event.
     */
    virtual void handleEventRemoved(Event *event);

protected slots:
    void slotMatrixScrolled(int x, int y); //!!! do we need this? probably not

protected:
    MatrixPainter(MatrixWidget *);

    void setBasicContextHelp();

    timeT m_clickTime;
    MatrixElement *m_currentElement;
    MatrixViewSegment *m_currentViewSegment;
};


}

#endif
