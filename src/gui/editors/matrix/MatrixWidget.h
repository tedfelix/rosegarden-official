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

#ifndef RG_MATRIX_WIDGET_H
#define RG_MATRIX_WIDGET_H

#include "base/Event.h"             // for timeT
#include "base/MidiTypes.h"         // for MidiByte
#include "gui/general/SelectionManager.h"

#include <vector>

#include <QWidget>

class QGraphicsScene;
class QGridLayout;
class QPushButton;

namespace Rosegarden
{

class RosegardenDocument;
class Segment;
class MatrixScene;
class MatrixToolBox;
class MatrixTool;
class MatrixMouseEvent;
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
class Thumbwheel;


/// QWidget that holds the matrix editor.
/**
 * Container widget for the matrix editor (which is a QGraphicsView)
 * and any associated rulers and panner widgets.  This class also owns
 * the editing tools.
 *
 * MatrixView::m_matrixWidget is the only instance of this class.
 */
class MatrixWidget : public QWidget,
                     public SelectionManager
{
    Q_OBJECT

public:
    MatrixWidget(bool drumMode);
    virtual ~MatrixWidget() override;

    /**
     * ??? Only one caller.  Might want to fold this into the ctor.
     */
    void setSegments(RosegardenDocument *document,
                     std::vector<Segment *> segments);

    MatrixScene *getScene() { return m_scene; }

    int getCurrentVelocity() const { return m_currentVelocity; }

    bool isDrumMode() const { return m_drumMode; }

    bool hasOnlyKeyMapping() const { return m_onlyKeyMapping; }

    MatrixToolBox *getToolBox() { return m_toolBox; }

    void setCanvasCursor(QCursor cursor);

    // These delegate to MatrixScene, which possesses the selection
    EventSelection *getSelection() const override;
    void setSelection(EventSelection *s, bool preview) override;

    ControlRulerWidget *getControlsWidget()  { return m_controlsWidget; }
    
    /// Delegates to MatrixScene::getSnapGrid()
    const SnapGrid *getSnapGrid() const;

    Segment *getCurrentSegment();
    Device *getCurrentDevice();
    bool segmentsContainNotes() const;

    void setTempoRulerVisible(bool visible);
    void setChordNameRulerVisible(bool visible);

    void setHoverNoteVisible(bool visible);

signals:
    void editTriggerSegment(int);
    void toolChanged(QString);
    void segmentDeleted(Segment *);
    void sceneDeleted();
    void showContextHelp(const QString &);
    /// Forwarded from MatrixScene::selectionChanged()
    void selectionChanged();

public slots:
    // ??? I suspect few of these is actually used as slots.  MatrixView
    //     performs all the createAction() calls and uses its own slots.
    //     Nothing is ever connected to these.  Confirm and remove slot-ness
    //     (monster).

    /// Edit > Select All
    /**
     * ??? Not used as a slot?  MatrixView has its own.
     */
    void slotSelectAll();
    /// Edit > Clear Selection
    /**
     * ??? Not used as a slot?  MatrixView has its own.
     */
    void slotClearSelection();

    /// Move > Previous Segment
    /**
     * ??? Not used as a slot?  MatrixView has its own.
     * ??? rename: slotPreviousSegment()
     */
    void slotCurrentSegmentPrior();
    /// Move > Next Segment
    /**
     * ??? Not used as a slot?  MatrixView has its own.
     * ??? rename: slotNextSegment()
     */
    void slotCurrentSegmentNext();

    /// Tools > Draw
    /**
     * ??? Not used as a slot?  MatrixView has its own.
     * ??? rename: slotSetDrawTool() or slotToolsDraw()
     */
    void slotSetPaintTool();
    /// Tools > Erase
    /**
     * ??? Not used as a slot?  MatrixView has its own.
     */
    void slotSetEraseTool();
    /// Tools > Select and Edit
    /**
     * ??? Not used as a slot?  MatrixView has its own.
     * ??? rename: slotSetSelectAndEditTool() or slotToolsSelectAndEdit()
     */
    void slotSetSelectTool();
    /// Tools > Move
    /**
     * ??? Not used as a slot?  MatrixView has its own.
     */
    void slotSetMoveTool();
    /// Tools > Resize
    /**
     * ??? Not used as a slot?  MatrixView has its own.
     */
    void slotSetResizeTool();
    /// Tools > Velocity
    /**
     * ??? Not used as a slot?  MatrixView has its own.
     */
    void slotSetVelocityTool();

    /// Move > Scroll to Follow Playback
    /**
     * ??? This is never used as a slot.  Move to public and rename.
     */
    void slotSetPlayTracking(bool);

    /// Velocity combo box.
    void slotSetCurrentVelocity(int velocity) { m_currentVelocity = velocity; }
    /**
     * ??? This is never used as a slot.  Move to public and rename.  The
     *     one in MatrixView is never used as a slot either.  Document.
     */
    void slotSetSnap(timeT);

    /**
     * ??? This is never used as a slot.  Only used privately.
     *     Move to private and rename.
     */
    void slotZoomInFromPanner();
    /**
     * ??? This is never used as a slot.  Only used privately.
     *     Move to private and rename.
     */
    void slotZoomOutFromPanner();

    /// View > Rulers > Show Velocity Ruler
    /**
     * ??? Not used as a slot?  MatrixView has its own.
     */
    void slotToggleVelocityRuler();
    /// View > Rulers > Show Pitch Bend Ruler
    /**
     * ??? Not used as a slot?  MatrixView has its own.
     */
    void slotTogglePitchbendRuler();
    /**
     * ??? This is never used as a slot.
     *     Move to public and rename.
     */
    void slotAddControlRuler(QAction*);

    /**
     * ??? This is never used as a slot.  Only used privately.
     *     Move to private and rename.
     */
    void slotHScroll();
    /**
     * ??? This is never used as a slot.  Only used privately.
     *     Move to private and rename.
     */
    void slotEnsureTimeVisible(timeT);

    /**
     * Show the pointer.  Used by MatrixView upon construction, this ensures
     * the pointer is visible initially.
     *
     * ??? This is never used as a slot.
     *     Move to public and rename.
     */
    void showInitialPointer();
    
    /**
     * ??? This is never used as a slot.
     *     Move to public and rename.
     */
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

    /// Instrument is being destroyed
    void slotInstrumentGone();

protected :
    void showEvent(QShowEvent * event) override;

    /// (Re)generate the pitch ruler (useful when key mapping changed)
    void generatePitchRuler();

private slots:
    /// Called when the document is modified in some way.
    void slotDocumentModified(bool);

    /// Connected to Panned::zoomIn() for ctrl+wheel.
    void slotZoomIn();
    /// Connected to Panned::zoomOut() for ctrl+wheel.
    void slotZoomOut();

private:
    RosegardenDocument *m_document; // I do not own this
    Panned *m_view; // I own this
    Panner *m_hpanner; // I own this
    MatrixScene *m_scene; // I own this
    MatrixToolBox *m_toolBox; // I own this
    MatrixTool *m_currentTool; // Toolbox owns this
    // This can be nullptr.  It tracks what pitchruler corresponds to.
    Instrument *m_instrument; // Studio owns this (TBC)
    bool m_drumMode;
    bool m_onlyKeyMapping;
    bool m_playTracking;

    double m_hZoomFactor;
    void setHorizontalZoomFactor(double factor);

    double m_vZoomFactor;
    void setVerticalZoomFactor(double factor);

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
    void updateSegmentChangerBackground();

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

    /// The last note we sent in case we're swooshing up and
    /// down the keyboard and don't want repeat notes sending
    ///
    MidiByte m_lastNote;

    /// The first note we sent in similar case (only used for
    /// doing effective sweep selections
    ///
    MidiByte m_firstNote;

    // Use to hide hover note when mouse move is not related to a pitch change
    bool m_hoverNoteIsVisible;



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

    /// ??? Never used as a slot.  Rename.
    void slotSetTool(QString name);

};

}

#endif
