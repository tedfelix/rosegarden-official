
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

#ifndef RG_PROPERTYCONTROLRULER_H
#define RG_PROPERTYCONTROLRULER_H

#include "base/PropertyName.h"
#include "ControlRuler.h"
#include "base/Event.h"
#include "base/Segment.h"
#include "base/ViewSegment.h"

#include <QString>

class QWidget;
class QMouseEvent;
class QContextMenuEvent;


namespace Rosegarden
{

class ViewElement;
class ViewSegment;
class Segment;
class RulerScale;

/// The Velocity Ruler
/**
 * ??? rename: PropertyRuler?  Or simplify to only handle velocity and
 *             call it VelocityRuler.
 */
class PropertyControlRuler :  public ControlRuler, public ViewSegmentObserver
{
public:
    PropertyControlRuler(PropertyName propertyName,
                        ViewSegment *viewSegment,
                        RulerScale *scale,
                        QWidget *parent);

    ~PropertyControlRuler() override;

    virtual void update();

    void paintEvent(QPaintEvent *) override;

    QString getName() override;

    const PropertyName& getPropertyName()     { return m_propertyName; }

    // Allow something external to reset the selection of Events
    // that this ruler is displaying
    //
    void setViewSegment(ViewSegment *) override;

    // ViewSegmentObserver interface
    void elementAdded(const ViewSegment *, ViewElement*) override;
    void elementRemoved(const ViewSegment *, ViewElement*) override;
    void viewSegmentDeleted(const ViewSegment *) override;

    virtual void selectAllProperties();

    /// SegmentObserver interface
    virtual void endMarkerTimeChanged(const Segment *, bool shorten);

    void updateSelection(std::vector<ViewElement*>*);
    void updateSelectedItems();

public slots:
    void slotHoveredOverNoteChanged(int evPitch, bool haveEvent, timeT evTime);
    void slotSetTool(const QString &) override;

protected:
    void addControlItem2(ViewElement *);

    void mousePressEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void contextMenuEvent(QContextMenuEvent*) override;

    virtual void init();

    //--------------- Data members ---------------------------------

    PropertyName m_propertyName;
};



}

#endif
