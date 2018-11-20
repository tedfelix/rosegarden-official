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

#ifndef RG_CONTROLPAINTER_H
#define RG_CONTROLPAINTER_H

#include "ControlMover.h"
//#include <QString>
//#include "base/Event.h"


namespace Rosegarden
{

class Event;
class ControlRuler;

class ControlPainter : public ControlMover
{
    Q_OBJECT

    friend class ControlToolBox;

public:
    void handleLeftButtonPress(const ControlMouseEvent *) override;
    ControlTool::FollowMode handleMouseMove(const ControlMouseEvent *) override;

    /**
     * Respond to an event being deleted -- it may be the one the tool
     * is remembering as the current event.
     */
//    virtual void handleEventRemoved(Event *event);

    static QString ToolName();

signals:
//    void hoveredOverNoteChanged(int evPitch, bool haveEvent, timeT evTime);

protected slots:
//    void slotMatrixScrolled(int x, int y); //!!! do we need this? probably not

protected:
    ControlPainter(ControlRuler *);

    /** In the GIMP, you have to click a point with the pencil, then hold shift
     * and click another point to draw a line between the two points.  If you
     * change tools in between, you have to start over.  Whatever the last thing
     * you did was, the last point you drew with the pencil while you did not
     * have shift clicked, that becomes the origin for the line when you do hold
     * shift on a subsequent click.
     *
     * I think we can mimic that behavior by saving the X and Y from the
     * previous mouse click while this tool was active, to use as the point of
     * origin of the line.
     */
    std::pair<float, float> m_controlLineOrigin;
};

}

#endif
