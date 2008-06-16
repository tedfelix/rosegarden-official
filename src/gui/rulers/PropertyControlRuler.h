
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

#ifndef _RG_PROPERTYCONTROLRULER_H_
#define _RG_PROPERTYCONTROLRULER_H_

#include "base/PropertyName.h"
#include "base/Staff.h"
#include "ControlRuler.h"
#include <qstring.h>
#include "base/Event.h"


class QWidget;
class QMouseEvent;
class QContextMenuEvent;
class QCanvasLine;
class QCanvas;


namespace Rosegarden
{

class ViewElement;
class Staff;
class Segment;
class RulerScale;
class EditViewBase;


/**
 * PropertyControlRuler : edit a property on events on a staff (only
 * events with a ViewElement attached, mostly notes)
 */
class PropertyControlRuler : public ControlRuler, public StaffObserver
{
public:
    PropertyControlRuler(PropertyName propertyName,
                         Staff*,
                         RulerScale*,
                         EditViewBase* parentView,
                         QCanvas*,
                         QWidget* parent=0, const char* name=0, WFlags f=0);

    virtual ~PropertyControlRuler();

    virtual QString getName();

    const PropertyName& getPropertyName()     { return m_propertyName; }

    // Allow something external to reset the selection of Events
    // that this ruler is displaying
    //
    void setStaff(Staff *);

    // StaffObserver interface
    virtual void elementAdded(const Staff *, ViewElement*);
    virtual void elementRemoved(const Staff *, ViewElement*);
    virtual void staffDeleted(const Staff *);
    virtual void startPropertyLine();
    virtual void selectAllProperties();

    /// SegmentObserver interface
    virtual void endMarkerTimeChanged(const Segment *, bool shorten);

protected:

    virtual void contentsMousePressEvent(QMouseEvent*);
    virtual void contentsMouseReleaseEvent(QMouseEvent*);
    virtual void contentsMouseMoveEvent(QMouseEvent*);
    virtual void contentsContextMenuEvent(QContextMenuEvent*);

    void drawPropertyLine(timeT startTime,
                          timeT endTime,
                          int startValue,
                          int endValue);

    virtual void init();
    virtual void drawBackground();
    virtual void computeStaffOffset();

    //--------------- Data members ---------------------------------

    PropertyName       m_propertyName;
    Staff*             m_staff;

    QCanvasLine                   *m_propertyLine;
    
    bool                           m_propertyLineShowing;
    int                            m_propertyLineX;
    int                            m_propertyLineY;
};



}

#endif
