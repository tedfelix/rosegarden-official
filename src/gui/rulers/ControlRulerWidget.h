/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#pragma once

#include "gui/general/AutoScroller.h"  // For FollowMode
#include "base/Controllable.h"  // For ControlList

#include <QWidget>

class QStackedWidget;


namespace Rosegarden
{


class ControllerEventsRuler;
class ControlRuler;
class ControlRulerTabBar;
class EventSelection;
class PropertyControlRuler;
class PropertyName;
class RosegardenDocument;
class RulerScale;
class Segment;
class SelectionSituation;
class ViewElement;
class ViewSegment;


class ControlRulerWidget : public QWidget
{

    Q_OBJECT

public:
    ControlRulerWidget();

    void setSegments(RosegardenDocument *document,
                     std::vector<Segment *> segments);

    /// Switch to showing this Segment.
    void setViewSegment(ViewSegment *);

    void setRulerScale(RulerScale *);
    void setRulerScale(RulerScale *, int gutter);

    void addControlRuler(const ControlParameter &);
    void togglePitchBendRuler();
    void togglePropertyRuler(const PropertyName &);

    /**
     * Returns true if we're showing any one of the myriad possible rulers we
     * might be showing.  This allows our parent to show() or hide() this entire
     * widget as appropriate for the sort of notation layout in effect.
     */
    bool isAnyRulerVisible();

    bool hasSelection();
    EventSelection *getSelection();
    SelectionSituation *getSituation();

    ControlParameter *getControlParameter();

    /// Returns Velocity ruler if currently shown else return 0
    PropertyControlRuler *getActivePropertyRuler();

public slots:

    void slotAddPropertyRuler(const PropertyName &);
    void slotRemoveRuler(int);
    void slotSetPannedRect(QRectF pr);
    void slotSetCurrentViewSegment(ViewSegment *);
    void slotSelectionChanged(EventSelection *);
    void slotHoveredOverNoteChanged();
    void slotHoveredOverNoteChanged(int evPitch, bool haveEvent, timeT evTime);
    void slotUpdateRulers(timeT,timeT);
    void slotSetToolName(const QString &);
    void slotDragScroll(timeT);

signals:
    /// DEPRECATED.  This is being replaced by the new mouse*() signals.
    void dragScroll(timeT);

    void mousePress();
    void mouseMove(FollowMode);
    void mouseRelease();

    void childRulerSelectionChanged(EventSelection *);
    void showContextHelp(const QString &);

private:
    void setSegment(Segment *segment);

    // ??? Remove this and use the Singleton directly.
    RosegardenDocument *m_document;

    ControllerEventsRuler *getActiveRuler();
    
    QStackedWidget *m_stackedWidget;
    ControlRulerTabBar *m_tabBar;

    typedef std::list<ControlRuler *> ControlRulerList;
    ControlRulerList m_controlRulerList;
    void removeRuler(ControlRulerList::iterator rulerIter);

    const ControlList *m_controlList;

    Segment *m_segment;
    ViewSegment *m_viewSegment;
    RulerScale *m_scale;
    int m_gutter;
    QString m_currentToolName;
    QRectF m_pannedRect;
    std::vector<ViewElement *> m_selectedElements;
    
    void addRuler(ControlRuler *, QString);

private slots:
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
