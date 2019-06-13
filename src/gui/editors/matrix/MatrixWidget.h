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
     * Show the pointer.  Used by MatrixView upon construction, this ensures
     * the pointer is visible initially.
     */
    void showInitialPointer();

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

    void setSnap(timeT);

    // Interface for MatrixView menu commands

    /// Edit > Select All
    void selectAll();
    /// Edit > Clear Selection
    void clearSelection();
    /// Move > Previous Segment
    void previousSegment();
    /// Move > Next Segment
    void nextSegment();
    /// Tools > Draw
    void setDrawTool();
    /// Tools > Erase
    void setEraseTool();
    /// Tools > Select and Edit
    void setSelectAndEditTool();
    /// Tools > Move
    void setMoveTool();
    /// Tools > Resize
    void setResizeTool();
    /// Tools > Velocity
    void setVelocityTool();
    /// Move > Scroll to Follow Playback
    void setScrollToFollowPlayback(bool);
    /// View > Rulers > Show Velocity Ruler
    void showVelocityRuler();
    /// View > Rulers > Show Pitch Bend Ruler
    void showPitchBendRuler();
    /// View > Rulers > Add Control Ruler
    void addControlRuler(QAction *);

signals:
    void editTriggerSegment(int);
    void toolChanged(QString);

    /// Forwarded from MatrixScene::segmentDeleted().
    void segmentDeleted(Segment *);
    /// Forwarded from MatrixScene::sceneDeleted().
    void sceneDeleted();

    void showContextHelp(const QString &);
    /// Forwarded from MatrixScene::selectionChanged()
    void selectionChanged();

public slots:
    /// Velocity combo box.
    void slotSetCurrentVelocity(int velocity) { m_currentVelocity = velocity; }

    /// Plays the preview note when using the computer keyboard to enter notes.
    void slotPlayPreviewNote(Segment *segment, int pitch);

protected:
    // QWidget Override
    /// Make sure the rulers are in sync when we are shown.
    void showEvent(QShowEvent *event) override;

private slots:
    /// Called when the document is modified in some way.
    void slotDocumentModified(bool);

    /// Connected to Panned::zoomIn() for ctrl+wheel.
    void slotZoomIn();
    /// Connected to Panned::zoomOut() for ctrl+wheel.
    void slotZoomOut();

    /// Scroll rulers to sync up with view.
    void slotScrollRulers();

    /// Scroll view so that specified time is visible.
    void slotEnsureTimeVisible(timeT);

    // MatrixScene Interface
    void slotDispatchMousePress(const MatrixMouseEvent *);
    void slotDispatchMouseMove(const MatrixMouseEvent *);
    void slotDispatchMouseRelease(const MatrixMouseEvent *);
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

    /// The mouse has left the view
    void slotMouseLeavesView();

    /// Instrument is being destroyed
    void slotInstrumentGone();

private:
    RosegardenDocument *m_document; // I do not own this

    QGridLayout *m_layout; // I own this


    // View

    /// QGraphicsScene holding the note Events.
    // ??? QSharedPointer
    MatrixScene *m_scene; // I own this

    /// The main view of the MatrixScene (m_scene).
    Panned *m_view; // I own this

    /// Whether the view will scroll along with the playback position pointer.
    bool m_playTracking;

    /// View horizontal zoom factor.
    double m_hZoomFactor;
    void setHorizontalZoomFactor(double factor);

    /// View vertical zoom factor.
    double m_vZoomFactor;
    void setVerticalZoomFactor(double factor);


    // Panner (Navigation Area)

    /// Navigation area under the main view.
    Panner *m_hpanner; // I own this
    void zoomInFromPanner();
    void zoomOutFromPanner();

    QWidget *m_changerWidget;
    Thumbwheel *m_segmentChanger;
    int m_lastSegmentChangerValue;
    void updateSegmentChangerBackground();


    // Pitch Ruler

    // This can be nullptr.  It tracks what pitchruler corresponds to.
    Instrument *m_instrument; // Studio owns this (TBC)
    /// Key mapping from the Instrument.
    // ??? QSharedPointer
    MidiKeyMapping *m_localMapping; // I own this
    /// Either a PercussionPitchRuler or a PianoKeyboard object.
    PitchRuler *m_pitchRuler; // I own this
    /// (Re)generate the pitch ruler (useful when key mapping changed)
    void generatePitchRuler();
    /// Contains m_pitchRuler.
    // ??? QSharedPointer
    QGraphicsScene *m_pianoScene; // I own this
    /// Contains m_pianoScene.
    Panned *m_pianoView; // I own this
    /// All Segments only have key mappings.  Use a PercussionPitchRuler.
    bool m_onlyKeyMapping;
    /// Percussion matrix editor?
    /**
     * For the Percussion matrix editor, we ignore key release events from
     * the PitchRuler.
     */
    bool m_drumMode;


    // Tools

    MatrixToolBox *m_toolBox; // I own this
    MatrixTool *m_currentTool; // Toolbox owns this
    void setTool(QString name);
    /// Used by the MatrixMover and MatrixPainter tools for preview notes.
    int m_currentVelocity;


    // Zoom Area

    Thumbwheel *m_HVzoom;
    int m_lastHVzoomValue;
    bool m_lastZoomWasHV;
    Thumbwheel *m_Hzoom;
    int m_lastH;
    Thumbwheel *m_Vzoom;
    int m_lastV;
    QPushButton *m_reset;


    // Rulers

    ChordNameRuler *m_chordNameRuler; // I own this
    TempoRuler *m_tempoRuler; // I own this
    StandardRuler *m_topStandardRuler; // I own this
    StandardRuler *m_bottomStandardRuler; // I own this
    ControlRulerWidget *m_controlsWidget; // I own this

    /// Used by all rulers to make sure they are all zoomed to the same scale.
    /**
     * See MatrixScene::getReferenceScale().
     */
    ZoomableRulerScale *m_referenceScale;


    // Auto-scroll

    /// Currently moving the scene via ensureVisible().
    /**
     * Appears to be intended to make sure that any mouse movements
     * that come in (slotDispatchMouseMove()) while the scene is
     * moving result in m_lastMouseMoveScenePos being updated.
     * However, it seems very unlikely that a mouse move event
     * could sneak in while m_inMove is true.  The UI is
     * single-threaded.  Maybe ensureVisible() pumps the message queue?
     *
     * At any rate, I'm planning on replacing this auto-scroll
     * implementation with a new one, so probably don't need to
     * spend much time analyzing the old one.
     */
    bool m_inMove;
    /// Mouse position while moving converted to Scene Coords.
    /**
     * Set by slotDispatchMouseMove().  Used by
     * slotEnsureLastMouseMoveVisible() to do auto scrolling.
     */
    QPointF m_lastMouseMoveScenePos;


    // Miscellaneous

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

};

}

#endif
