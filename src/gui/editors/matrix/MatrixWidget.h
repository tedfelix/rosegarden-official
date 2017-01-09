/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_MATRIX_WIDGET_H
#define RG_MATRIX_WIDGET_H

#include <QWidget>
#include <QPushButton>
#include <QSharedPointer>

#include "base/Event.h"             // for timeT
#include "base/MidiTypes.h"         // for MidiByte
#include "gui/general/SelectionManager.h"
#include "gui/widgets/Thumbwheel.h"

#include <vector>

class QGraphicsScene;
class QStackedLayout; // To be replaced
class QGridLayout;

namespace Rosegarden
{

class RosegardenDocument;
class Segment;
class MatrixScene;
class MatrixToolBox;
class MatrixTool;
class MatrixMouseEvent;
class RulerScale;
class SnapGrid;
class ZoomableRulerScale;
class Panner;
class Panned;
class EventSelection;
class PitchRuler;
class MidiKeyMapping;
class ControlRulerWidget;
class StandardRuler;
class TempoRuler;
class ChordNameRuler;
class Device;
class Instrument;
class InstrumentStaticSignals;


/**
 * Container widget for the matrix editor (which is a QGraphicsView)
 * and any associated rulers and panner widgets.  This class also owns
 * the editing tools.
 */
class MatrixWidget : public QWidget,
                     public SelectionManager
{
    Q_OBJECT

public:
    MatrixWidget(bool drumMode);
    virtual ~MatrixWidget();

    void setSegments(RosegardenDocument *document,
                     std::vector<Segment *> segments);

    MatrixScene *getScene() { return m_scene; }
    Panned *getView() { return m_view; }

    void setHorizontalZoomFactor(double factor);
    void setVerticalZoomFactor(double factor);

    double getHorizontalZoomFactor() const;
    double getVerticalZoomFactor() const;

    int getCurrentVelocity() const { return m_currentVelocity; }

    bool isDrumMode() const { return m_drumMode; }

    bool hasOnlyKeyMapping() const { return m_onlyKeyMapping; }

    bool getPlayTracking() const { return m_playTracking; }

    MatrixToolBox *getToolBox() { return m_toolBox; }

    void setCanvasCursor(QCursor cursor);

    // These delegate to MatrixScene, which possesses the selection
    virtual EventSelection *getSelection() const;
    virtual void setSelection(EventSelection *s, bool preview);

    ControlRulerWidget *getControlsWidget(void)
    { return m_controlsWidget; }
    
    // This delegates to MatrixScene
    const SnapGrid *getSnapGrid() const;

    Segment *getCurrentSegment();
    Device *getCurrentDevice();
    bool segmentsContainNotes() const;

    void setTempoRulerVisible(bool visible);
    void setChordNameRulerVisible(bool visible);

    void updateSegmentChangerBackground();

signals:
    void editTriggerSegment(int);
    void toolChanged(QString);
    void segmentDeleted(Segment *);
    void sceneDeleted();
    void showContextHelp(const QString &);
    void selectionChanged();

public slots:
    void slotSelectAll();
    void slotClearSelection();

    void slotCurrentSegmentPrior();
    void slotCurrentSegmentNext();

    void slotSetTool(QString name);
    void slotSetPaintTool();
    void slotSetEraseTool();
    void slotSetSelectTool();
    void slotSetMoveTool();
    void slotSetResizeTool();
    void slotSetVelocityTool();

    void slotSetPlayTracking(bool);

    void slotSetCurrentVelocity(int velocity) { m_currentVelocity = velocity; }
    void slotSetSnap(timeT);

    void slotZoomInFromPanner();
    void slotZoomOutFromPanner();

    void slotToggleVelocityRuler();
    void slotTogglePitchbendRuler();
    void slotAddControlRuler(QAction*);

    void slotHScroll();
    void slotEnsureTimeVisible(timeT);

    /** Show the pointer.  Used by MatrixView upon construction, this ensures
     * the pointer is visible initially.
     */
    void showInitialPointer();
    
    void slotPlayPreviewNote(Segment * segment, int pitch);

protected slots:
    void slotDispatchMousePress(const MatrixMouseEvent *);
    void slotDispatchMouseRelease(const MatrixMouseEvent *);
    void slotDispatchMouseMove(const MatrixMouseEvent *);
    void slotDispatchMouseDoubleClick(const MatrixMouseEvent *);

    void slotPointerPositionChanged(timeT, bool moveView = true);
    void slotEnsureLastMouseMoveVisible();

    void slotHScrollBarRangeChanged(int min, int max);

    void slotHoveredOverKeyChanged(unsigned int);
    void slotKeyPressed(unsigned int, bool);
    void slotKeySelected(unsigned int, bool);
    void slotKeyReleased(unsigned int, bool);

    /// The horizontal zoom thumbwheel moved
    void slotHorizontalThumbwheelMoved(int);

    /// The vertical zoom thumbwheel moved
    void slotVerticalThumbwheelMoved(int);

    /// The primary (combined axes) thumbwheel moved
    void slotPrimaryThumbwheelMoved(int);

    /// Reset the zoom to 100% and reset the zoomy wheels
    void slotResetZoomClicked();

    /// Trap a zoom in from the panner and sync it to the primary thumb wheel
    void slotSyncPannerZoomIn();

    /// Trap a zoom out from the panner and sync it to the primary thumb wheel
    void slotSyncPannerZoomOut();

    /// The segment control thumbwheel moved
    void slotSegmentChangerMoved(int);

    void slotInitialHSliderHack(int);

    /// The mouse has left the view
    void slotMouseLeavesView();

    /// Pitch ruler may need regeneration
    void slotInstrumentChanged(Instrument *instrument);

    /// Pitch ruler may need regeneration
    void slotPercussionSetChanged(Instrument *instr);

    /// Instrument is being destroyed
    void slotInstrumentGone(void);

protected :
    virtual void showEvent(QShowEvent * event);

    /// (Re)generate the pitch ruler (useful when key mapping changed)
    void generatePitchRuler();

private:
    RosegardenDocument *m_document; // I do not own this
    Panned *m_view; // I own this
    Panner *m_hpanner; // I own this
    MatrixScene *m_scene; // I own this
    MatrixToolBox *m_toolBox; // I own this
    MatrixTool *m_currentTool; // Toolbox owns this
    // This can be NULL.  It tracks what pitchruler corresponds to.
    Instrument *m_instrument; // Studio owns this (TBC)
    bool m_drumMode;
    bool m_onlyKeyMapping;
    bool m_playTracking;
    double m_hZoomFactor;
    double m_vZoomFactor;
    int m_currentVelocity;
    ZoomableRulerScale *m_referenceScale; // m_scene own this (refers to scene scale)
    bool m_inMove;
    QPointF m_lastMouseMoveScenePos;

    Thumbwheel  *m_HVzoom;
    Thumbwheel  *m_Hzoom;
    Thumbwheel  *m_Vzoom;
    QPushButton *m_reset;

    /** The primary zoom wheel behaves just like using the mouse wheel over any
     * part of the Panner.  We don't need to keep track of absolute values here,
     * just whether we rolled up or down.  We'll do that by keeping track of the
     * last setting and comparing it to see which way it moved.
     */
    int m_lastHVzoomValue;
    bool m_lastZoomWasHV;
    int m_lastV;
    int m_lastH;

    QWidget *m_changerWidget;
    Thumbwheel  *m_segmentChanger;
    int m_lastSegmentChangerValue;

    /// Either a PercussionPitchRuler or a PianoKeyboard object.
    PitchRuler *m_pitchRuler; // I own this
    /// Contains m_pianoScene.
    Panned *m_pianoView; // I own this
    /// Contains m_pitchRuler.
    QGraphicsScene *m_pianoScene; // I own this

    ControlRulerWidget *m_controlsWidget; // I own this

    MidiKeyMapping *m_localMapping; // I own this

    StandardRuler *m_topStandardRuler; // I own this
    StandardRuler *m_bottomStandardRuler; // I own this
    TempoRuler *m_tempoRuler; // I own this
    ChordNameRuler *m_chordNameRuler; // I own this

    QGridLayout *m_layout; // I own this

    bool m_hSliderHacked;

    bool m_Thorn;

    /// The last note we sent in case we're swooshing up and
    /// down the keyboard and don't want repeat notes sending
    ///
    MidiByte m_lastNote;

    /// The first note we sent in similar case (only used for
    /// doing effective sweep selections
    ///
    MidiByte m_firstNote;



    /**
     * Widgets vertical positions inside the main QGridLayout
     */
    enum {
        CHORDNAMERULER_ROW,
        TEMPORULER_ROW,
        TOPRULER_ROW,
        PANNED_ROW,
        BOTTOMRULER_ROW,
        CONTROLS_ROW,
        HSLIDER_ROW,
        PANNER_ROW
    };

    /**
     * Widgets horizontal positions inside the main QGridLayout
     */
    enum {
        HEADER_COL,
        MAIN_COL,
    };

};

}

#endif
