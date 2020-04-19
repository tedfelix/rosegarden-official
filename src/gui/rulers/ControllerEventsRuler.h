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

#ifndef RG_CONTROLLEREVENTSRULER_H
#define RG_CONTROLLEREVENTSRULER_H

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
class ControlParameter;
class ControlItem;

/// Controller Ruler (volume, pan, pitchbend, etc...)
/**
 * ??? rename: ControllerRuler
 */
class ControllerEventsRuler : public ControlRuler, public SegmentObserver
{
public:
    ControllerEventsRuler(ViewSegment*,
                          RulerScale*,
                          QWidget* parent=nullptr,
                          const ControlParameter *controller = nullptr,
                          const char* name=nullptr );

    ~ControllerEventsRuler() override;

    void paintEvent(QPaintEvent *) override;

    QString getName() override;
    int getDefaultItemWidth() { return m_defaultItemWidth; }

    // Allow something external to reset the selection of Events
    // that this ruler is displaying
    //
    void setViewSegment(ViewSegment *) override;
    void setSegment(Segment *) override;

    // SegmentObserver interface
    void eventAdded(const Segment *, Event *) override;
    void eventRemoved(const Segment *, Event *) override;
    void segmentDeleted(const Segment *) override;

    virtual QSharedPointer<ControlItem> addControlItem2(float, float);
    virtual QSharedPointer<ControlItem> addControlItem2(Event *);

    /** Draw a line of controllers from (x1, y1) to (x2, y2).  If
     * eraseExistingControllers is true, any existing controllers falling within
     * this line's span of time will be cleared away before the line is drawn
     *
     * Does not return anything yet, as whether it should or not has yet to be
     * determined.
     */
    virtual void addControlLine(
            double x1, double y1,
            double x2, double y2,
            bool eraseExistingContollers);

    /** Draw a rubber band indicating the controller line that will be drawn if
     * the user clicks another event into existence while moving the pencil
     * around with the shift key down.
     */
    virtual void drawRubberBand(float x1, float y1, float x2, float y2);

    /** Turn off the rubber band (user released shift without drawing a line)
     */
    virtual void stopRubberBand();

    virtual Event * insertEvent(float,float);
    virtual void eraseEvent(Event *event);
    virtual void eraseControllerEvent();

    ControlParameter* getControlParameter() { return m_controller; }

public slots:
    void slotSetTool(const QString&) override;

protected:
    virtual void init();
    virtual bool isOnThisRuler(Event *);

    //--------------- Data members ---------------------------------
    int  m_defaultItemWidth;

    ControlParameter  *m_controller;
    QRectF m_lastDrawnRect;
    // ??? See if we can remove this.
    bool m_moddingSegment;
    QLineF *m_rubberBand;
    bool m_rubberBandVisible;
};


}

#endif
