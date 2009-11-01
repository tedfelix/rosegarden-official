/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2009 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_CONTROLRULERWIDGET_H_
#define _RG_CONTROLRULERWIDGET_H_

#include <QWidget>
#include "base/Event.h"
#include "base/ViewElement.h"
#include "base/MidiDevice.h"

class QStackedWidget;
class QTabBar;

namespace Rosegarden
{

class RosegardenDocument;
class Segment;
class ControlRuler;
class ControlRulerTabBar;
class ControlParameter;
class RulerScale;
class PropertyName;
class ViewSegment;
class EventSelection;

class ControlRulerWidget : public QWidget //, Observer
{
Q_OBJECT

public:
    ControlRulerWidget();
    virtual ~ControlRulerWidget();

    void setSegments(RosegardenDocument *document,
                     std::vector<Segment *> segments);

    void setViewSegment(ViewSegment *);
    QString getCurrentToolName() { return m_currentToolName; }
    void removeRuler(std::list<ControlRuler*>::iterator);

public slots:
    void slotTogglePropertyRuler(const PropertyName &);
    void slotToggleControlRuler(std::string);

    void slotAddControlRuler(const ControlParameter &);
    void slotAddPropertyRuler(const PropertyName &);
    void slotRemoveRuler(int);
    void slotSetPannedRect(QRectF pr);
    void slotSelectionChanged(EventSelection *);
    void slotHoveredOverNoteChanged();
    void slotHoveredOverNoteChanged(int evPitch, bool haveEvent, timeT evTime);
    void slotUpdatePropertyRulers();
    void slotSetToolName(const QString &);
    void slotDragScroll(timeT);

signals:
    void dragScroll(timeT);
    void childRulerSelectionChanged(EventSelection *);
    
protected:
    QStackedWidget *m_stackedWidget;
    ControlRulerTabBar *m_tabBar;
    
    std::list<ControlRuler *> m_controlRulerList;
    const ControlList *m_controlList;

    RosegardenDocument *m_document;
    Segment *m_segment;
    ViewSegment *m_viewSegment;
    RulerScale *m_scale;
    QString m_currentToolName;
    QRectF m_pannedRect;
    std::vector <ViewElement*> m_selectedElements;
    
    void addRuler(ControlRuler *, QString);

protected slots:
    /** ControlRuler emits rulerSelectionChanged() which is connected to this
     * slot.  This slot picks up child ruler selection changes and emits
     * childRulerSelectionChanged() to be caught by the associated (matrix or
     * notation) scene, so it can add our child ruler's selected events to its
     * own selection for cut/copy/paste operations.  At least that's the theory.
     *
     * Pitch Bend ruler -> selection changes -> emit rulerSelectionChanged() ->
     * Control Ruler Widget -> this slot -> emit childRulerSelectionChanged ->
     * owning scene -> selection updates
     */
    void slotChildRulerSelectionChanged(EventSelection *);

};
}

#endif
