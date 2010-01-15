/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2010 the Rosegarden development team.

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
#include "base/Event.h"
#include "base/Segment.h"
#include <QString>

class QWidget;
class QMouseEvent;


namespace Rosegarden
{

class Segment;
class RulerScale;
class Event;
//class EditViewBase;
class ControlParameter;
class ControlItem;


/**
 * ControllerEventsRuler : edit Controller events
 */
class ControllerEventsRuler : public ControlRuler, public SegmentObserver
{
public:
    ControllerEventsRuler(ViewSegment*,
                          RulerScale*,
                          QWidget* parent=0,
                          const ControlParameter *controller = 0,
                          const char* name=0 );	//, WFlags f=0);

    virtual ~ControllerEventsRuler();

    virtual void paintEvent(QPaintEvent *);

    virtual QString getName();
    int getDefaultItemWidth() { return m_defaultItemWidth; }

    // Allow something external to reset the selection of Events
    // that this ruler is displaying
    //
    virtual void setViewSegment(ViewSegment *);
    virtual void setSegment(Segment *);

    // SegmentObserver interface
//    virtual void elementAdded(const ViewSegment *, ViewElement*);
    virtual void eventAdded(const Segment *, Event *);
//    virtual void elementRemoved(const ViewSegment *, ViewElement*);
    virtual void eventRemoved(const Segment *, Event *);
//    virtual void viewSegmentDeleted(const ViewSegment *);
    virtual void segmentDeleted(const Segment *);

    virtual ControlItem* addControlItem(float,float);
    virtual ControlItem* addControlItem(Event *);

    virtual Event * insertEvent(float,float);
    virtual void eraseEvent(Event *event);
    virtual void eraseControllerEvent();
//    virtual void clearControllerEvents();
//    virtual void startControlLine();

    ControlParameter* getControlParameter() { return m_controller; }

public slots:
    virtual void slotSetTool(const QString&);

protected:
    virtual void init();
    virtual bool isOnThisRuler(Event *);

    //--------------- Data members ---------------------------------
    int  m_defaultItemWidth;

    ControlParameter  *m_controller;
    QRectF m_lastDrawnRect;
    bool m_moddingSegment;
};


}

#endif
