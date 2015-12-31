
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_PROPERTYCONTROLRULER_H
#define RG_PROPERTYCONTROLRULER_H

#include "base/PropertyName.h"
#include "ControlRuler.h"
#include <QString>
#include "base/Event.h"
#include "base/Segment.h"


class QWidget;
class QMouseEvent;
class QContextMenuEvent;


namespace Rosegarden
{

class ViewElement;
class ViewSegment;
class Segment;
class RulerScale;


/**
 * PropertyControlRuler : edit a property on events on a staff (only
 * events with a ViewElement attached, mostly notes)
 */
class PropertyControlRuler :  public ControlRuler, public ViewSegmentObserver
{
public:
    PropertyControlRuler(PropertyName propertyName,
                        ViewSegment*,
                        RulerScale*,
                        QWidget* parent=0, const char* name=0);

    virtual ~PropertyControlRuler();

    virtual void update();
    
    virtual void paintEvent(QPaintEvent *);

    virtual QString getName();

    const PropertyName& getPropertyName()     { return m_propertyName; }

    // Allow something external to reset the selection of Events
    // that this ruler is displaying
    //
    virtual void setViewSegment(ViewSegment *);

    // ViewSegmentObserver interface
    virtual void elementAdded(const ViewSegment *, ViewElement*);
    virtual void elementRemoved(const ViewSegment *, ViewElement*);
    virtual void viewSegmentDeleted(const ViewSegment *);

    virtual void selectAllProperties();

    /// SegmentObserver interface
    virtual void endMarkerTimeChanged(const Segment *, bool shorten);

    void updateSelection(std::vector<ViewElement*>*);
    void updateSelectedItems();

public slots:
    void slotHoveredOverNoteChanged(int evPitch, bool haveEvent, timeT evTime);
    virtual void slotSetTool(const QString &);

protected:
    void addControlItem2(ViewElement *);

    virtual void mousePressEvent(QMouseEvent*);
    virtual void mouseReleaseEvent(QMouseEvent*);
    virtual void mouseMoveEvent(QMouseEvent*);
    virtual void contextMenuEvent(QContextMenuEvent*);

    virtual void init();

    //--------------- Data members ---------------------------------

    PropertyName m_propertyName;
};



}

#endif
