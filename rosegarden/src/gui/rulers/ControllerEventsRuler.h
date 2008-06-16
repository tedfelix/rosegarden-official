
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

#ifndef _RG_CONTROLLEREVENTSRULER_H_
#define _RG_CONTROLLEREVENTSRULER_H_

#include "ControlRuler.h"
#include <qstring.h>
#include "base/Event.h"


class QWidget;
class QMouseEvent;
class QCanvasLine;
class QCanvas;


namespace Rosegarden
{

class Segment;
class RulerScale;
class Event;
class EditViewBase;
class ControlParameter;
class ControlItem;


/**
 * ControllerEventsRuler : edit Controller events
 */
class ControllerEventsRuler : public ControlRuler
{
public:
    ControllerEventsRuler(Segment*,
                          RulerScale*,
                          EditViewBase* parentView,
                          QCanvas*,
                          QWidget* parent=0,
                          const ControlParameter *controller = 0,
                          const char* name=0, WFlags f=0);

    virtual ~ControllerEventsRuler();

    virtual QString getName();
    int getDefaultItemWidth() { return m_defaultItemWidth; }

    // Allow something external to reset the selection of Events
    // that this ruler is displaying
    //
    void setSegment(Segment *);

    /// SegmentObserver interface
    virtual void eventAdded(const Segment *, Event *);
    virtual void eventRemoved(const Segment *, Event *);

    virtual void insertControllerEvent();
    virtual void eraseControllerEvent();
    virtual void clearControllerEvents();
    virtual void startControlLine();

    ControlParameter* getControlParameter() { return m_controller; }

protected:

    virtual void init();
    virtual void drawBackground();

    // Let's override these again here
    //
    virtual void contentsMousePressEvent(QMouseEvent*);
    virtual void contentsMouseReleaseEvent(QMouseEvent*);
    virtual void contentsMouseMoveEvent(QMouseEvent*);

    virtual void layoutItem(ControlItem*);

    void drawControlLine(timeT startTime,
                         timeT endTime,
                         int startValue,
                         int endValue);

    //--------------- Data members ---------------------------------
    int                           m_defaultItemWidth;

    ControlParameter  *m_controller;
    QCanvasLine                   *m_controlLine;
    
    bool                           m_controlLineShowing;
    int                            m_controlLineX;
    int                            m_controlLineY;
};


}

#endif
