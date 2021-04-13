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
class RulerScale;
class Segment;
class SelectionSituation;
class ViewElement;
class ViewSegment;

/**
 * The ruler area (m_stackedWidget) and the tabs (m_tabBar) that appear below
 * the Matrix and Notation editors.
 */
class ControlRulerWidget : public QWidget
{

    Q_OBJECT

public:
    ControlRulerWidget();

    /// Set the Segment(s) from the document that we will be displaying.
    /**
     * This is only called once when the MatrixView comes up.  So this
     * does not track the currently displayed Segment.  That's
     * setViewSegment().
     *
     * ??? ControlRuler needs both the Segment and the ViewSegment
     *     in order to function.  Should we get rid of this and
     *     only deal in ViewSegments?  Would that make more sense?
     *     To a certain extent, it would.  However, we *need* the
     *     complete list of Segments so that we can maintain the
     *     ruler lists.  So, I think we need to keep this.
     */
    void setSegments(std::vector<Segment *> segments);

    /// Switch to showing this Segment.
    /**
     * This is called (by the Scene) when the Segment being edited changes.
     */
    void setViewSegment(ViewSegment *);

    void setRulerScale(RulerScale *);
    /// gutter is more of a left margin.  See m_gutter.
    void setRulerScale(RulerScale *, int gutter);

    void launchMatrixRulers();
    void launchNotationRulers();

    // ??? This doesn't toggle for the menu.  Consequently we can end
    //     up with duplicate rulers.  Need to fix this.
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

    /// Return the active ruler's event selection, or nullptr if none.
    /**
     * @author Tom Breton (Tehom)
     */
    EventSelection *getSelection();

    /**
     * @return the active ruler's parameter situation, or nullptr if none.
     *         Return is owned by caller.
     * @author Tom Breton (Tehom)
     */
    SelectionSituation *getSituation();

    ControlParameter *getControlParameter();

    /// Returns Velocity ruler if currently shown else return 0
    PropertyControlRuler *getActivePropertyRuler();

public slots:

    /// Connected to ControlRulerTabBar::tabCloseRequest().
    void slotRemoveRuler(int);
    /// Connected to the Matrix Panned and Panner.
    void slotSetPannedRect(QRectF pr);
    /// MatrixScene and NotationScene call this when the current Segment changes.
    void slotSetCurrentViewSegment(ViewSegment *);
    /// Update the velocity ruler selection to match the segment selection.
    /**
     * Comes from the view indicating the view's selection changed.  We do NOT
     * emit childRulerSelectionChanged() here.
     */
    void slotSelectionChanged(EventSelection *);
    /// Connected to MatrixMover::hoveredOverNoteChanged().
    void slotHoveredOverNoteChanged(int evPitch, bool haveEvent, timeT evTime);

    void slotUpdateRulers(timeT startTime, timeT endTime);
    /// Connected to toolChanged() signals.
    void slotSetTool(const QString &);

signals:
    // These three are used by MatrixWidget and NotationWidget for
    // autoscrolling when working in the rulers.
    // ??? Auto-scroll does not seem to work with the selection tool.  Works
    //     with the move tool.  Tested with PitchBend.
    void mousePress();
    void mouseMove(FollowMode);
    void mouseRelease();

    /// See ControlRuler::rulerSelectionChanged() for details.
    void childRulerSelectionChanged(EventSelection *);

    void showContextHelp(const QString &);

private:

    /// All of the Segments that the parent editor might switch between.
    /**
     * We hold on to this so that we can update the ruler lists for all
     * of these Segments when rulers are opened and closed.
     */
    std::vector<Segment *> m_segments;

    /// The Segment we are currently editing.
    ViewSegment *m_viewSegment;


    // *** UI

    /// The Rulers
    QStackedWidget *m_stackedWidget;
    ControllerEventsRuler *getActiveRuler();

    /// The tabs under the rulers.
    ControlRulerTabBar *m_tabBar;


    typedef std::list<ControlRuler *> ControlRulerList;
    ControlRulerList m_controlRulerList;
    void addPropertyRuler(const PropertyName &);
    void addRuler(ControlRuler *, QString);
    void removeRuler(ControlRuler *ruler);

    RulerScale *m_scale;

    /// Left margin used by NotationWidget to line things up?
    /**
     * ??? rename: m_leftMargin?
     */
    int m_gutter;

    QString m_currentToolName;

    /// Current pan position in the Segment.
    QRectF m_pannedRect;

    /// Selection for the property (velocity) ruler only.
    std::vector<ViewElement *> m_selectedElements;

private slots:
    /**
     * ControlRuler::rulerSelectionChanged() is connected to this
     * slot.  This slot picks up child ruler selection changes and emits
     * childRulerSelectionChanged() to be caught by the associated (matrix or
     * notation) scene, so it can add our child ruler's selected events to its
     * own selection for cut/copy/paste operations.  At least that's the theory.
     *
     * See the comments on ControlRuler::rulerSelectionChanged() for more.
     *
     * Pitch Bend ruler -> selection changes -> emit rulerSelectionChanged() ->
     * Control Ruler Widget -> this slot -> emit childRulerSelectionChanged ->
     * owning scene -> selection updates
     *
     * ??? Can we bypass this and just connect the ruler directly to
     *     MatrixScene?  I think the original intent was to connect it
     *     also to NotationScene.  So this would perform the distribution
     *     to both editors.  Each editor should connect to this on their
     *     own.
     */
    void slotChildRulerSelectionChanged(EventSelection *);

};


}
