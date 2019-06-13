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
#include "MatrixTool.h"
#include "base/MidiTypes.h"         // for MidiByte
#include "gui/general/SelectionManager.h"

#include <vector>

#include <QSharedPointer>
#include <QTimer>
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


/// QWidget that fills the Matrix Editor's (MatrixView) client area.
/**
 * The main Matrix Editor window, MatrixView, owns the only instance
 * of this class.  See MatrixView::m_matrixWidget.
 *
 * MatrixWidget contains the matrix itself (m_view and m_scene).
 * MatrixWidget also contains all the other parts of the Matrix Editor
 * that appear around the matrix.  From left to right, top to bottom:
 *
 *   - The chord name ruler
 *   - The tempo ruler
 *   - The top standard ruler
 *   - The pitch ruler, m_pianoView (to the left of the matrix)
 *   - The matrix itself, m_view and m_scene
 *   - The bottom standard ruler
 *   - The controls widget (optional)
 *   - The panner, m_hpanner (navigation area)
 *   - The zoom area (knobs in the bottom right corner)
 *
 * This class also owns the editing tools.
 */
class MatrixWidget : public QWidget,
                     public SelectionManager
{
    Q_OBJECT

public:
    MatrixWidget(bool drumMode);
    virtual ~MatrixWidget() override;

    Device *getCurrentDevice();
    MatrixScene *getScene()  { return m_scene; }

    /**
     * Show the pointer.  Used by MatrixView upon construction, this ensures
     * the pointer is visible initially.
     */
    void showInitialPointer();

    /// Set the Segment(s) to display.
    /**
     * ??? Only one caller.  Might want to fold this into the ctor.
     */
    void setSegments(RosegardenDocument *document,
                     std::vector<Segment *> segments);
    /// MatrixScene::getCurrentSegment()
    Segment *getCurrentSegment();
    /// MatrixScene::segmentsContainNotes()
    bool segmentsContainNotes() const;

    /// All segments only have a key mapping.
    bool hasOnlyKeyMapping() const { return m_onlyKeyMapping; }

    ControlRulerWidget *getControlsWidget()  { return m_controlsWidget; }

    /// MatrixScene::getSnapGrid()
    const SnapGrid *getSnapGrid() const;
    /// MatrixScene::setSnap()
    void setSnap(timeT);

    void setChordNameRulerVisible(bool visible);
    void setTempoRulerVisible(bool visible);

    // ??? This seems broken.  I don't see a hover anywhere.
    void setHoverNoteVisible(bool visible);


    // SelectionManager interface.

    // These delegate to MatrixScene, which possesses the selection
    /// MatrixScene::getSelection()
    EventSelection *getSelection() const override;
    /// MatrixScene::setSelection()
    void setSelection(EventSelection *s, bool preview) override;


    // Tools

    MatrixToolBox *getToolBox() { return m_toolBox; }

    /// Used by the tools to set an appropriate mouse cursor.
    void setCanvasCursor(QCursor cursor);

    bool isDrumMode() const { return m_drumMode; }

    /// Velocity for new notes.  (And moved notes too.)
    int getCurrentVelocity() const { return m_currentVelocity; }


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
    void toolChanged(QString);

    /**
     * Emitted when the user double-clicks on a note that triggers a
     * segment.
     *
     * RosegardenMainViewWidget::slotEditTriggerSegment() launches the event
     * editor on the triggered segment in response to this.
     */
    void editTriggerSegment(int);

    /// Forwarded from MatrixScene::segmentDeleted().
    void segmentDeleted(Segment *);
    /// Forwarded from MatrixScene::sceneDeleted().
    void sceneDeleted();
    /// Forwarded from MatrixScene::selectionChanged()
    void selectionChanged();

    void showContextHelp(const QString &);

public slots:
    /// Velocity combo box.
    void slotSetCurrentVelocity(int velocity)  { m_currentVelocity = velocity; }

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
    /**
     * This is used for drags in the rulers.
     */
    void slotEnsureTimeVisible(timeT);

    // MatrixScene Interface
    void slotDispatchMousePress(const MatrixMouseEvent *);
    void slotDispatchMouseMove(const MatrixMouseEvent *);
    void slotDispatchMouseRelease(const MatrixMouseEvent *);
    void slotDispatchMouseDoubleClick(const MatrixMouseEvent *);

    /// Display the playback position pointer.
    void slotPointerPositionChanged(timeT t);
    void slotStandardRulerDrag(timeT t);
    /// Handle StandardRuler startMouseMove()
    void slotSRStartMouseMove();
    /// Handle StandardRuler stopMouseMove()
    void slotSRStopMouseMove();

    /// Handle ControlRulerWidget::mousePress().
    void slotCRWMousePress();
    /// Handle ControlRulerWidget::mouseMove().
    void slotCRWMouseMove(FollowMode followMode);
    /// Handle ControlRulerWidget::mouseRelease().
    void slotCRWMouseRelease();

    /// Hide the horizontal scrollbar when not needed.
    /**
     * ??? Why do we need to manage this?  We turn off the horizontal
     *     scrollbar in the ctor with Qt::ScrollBarAlwaysOff.
     */
    void slotHScrollBarRangeChanged(int min, int max);

    // PitchRuler slots
    /// Draw the hover note as we move from one key to the next.
    /**
     * ??? Hover note doesn't seem to work.
     */
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

    /// The Segment control thumbwheel moved, display a different Segment.
    void slotSegmentChangerMoved(int);

    /// The mouse has left the view, hide the hover note.
    /**
     * ??? Hover note doesn't seem to work.
     */
    void slotMouseLeavesView();

    /// Instrument is being destroyed
    void slotInstrumentGone();

    void slotOnAutoScrollTimer();

private:
    // ??? Instead of storing the document, which can change, get the
    //     document as needed via RosegardenMainWindow::self()->getDocument().
    RosegardenDocument *m_document; // I do not own this

    QGridLayout *m_layout; // I own this


    // View

    /// QGraphicsScene holding the note Events.
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


    // Pointer

    void updatePointer(timeT t);


    // Panner (Navigation Area below the matrix)

    /// Navigation area under the main view.
    Panner *m_panner; // I own this
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
    QSharedPointer<MidiKeyMapping> m_localMapping;
    /// Either a PercussionPitchRuler or a PianoKeyboard object.
    PitchRuler *m_pitchRuler; // I own this
    /// (Re)generate the pitch ruler (useful when key mapping changed)
    void generatePitchRuler();
    /// Contains m_pitchRuler.
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

    /// First note selected when doing a run up/down the keyboard.
    /**
     * Used to allow selection of a range of pitches by shift-clicking a
     * note on the pitch ruler then dragging up or down to another note.
     */
    MidiByte m_firstNote;

    /// Last note selected when doing a run up/down the keyboard.
    /**
     * Used to prevent redundant note on/offs when doing a run up/down the
     * pitch ruler.
     */
    MidiByte m_lastNote;

    /// Hide pitch ruler hover note when mouse move is not related to a pitch change.
    /**
     * ??? I don't ever see a hover note.  Is it broken?
     */
    bool m_hoverNoteIsVisible;


    // Tools

    MatrixToolBox *m_toolBox; // I own this
    MatrixTool *m_currentTool; // Toolbox owns this
    void setTool(QString name);
    /// Used by the MatrixMover and MatrixPainter tools for preview notes.
    int m_currentVelocity;


    // Zoom Area (to the right of the Panner)

    /// The big zoom wheel.
    Thumbwheel *m_HVzoom;
    /// Used to compute how far the big zoom wheel has moved.
    int m_lastHVzoomValue;
    /// Which zoom factor to use.  For the pitch ruler.
    bool m_lastZoomWasHV;
    /// Thin horizontal zoom wheel under the big zoom wheel.
    Thumbwheel *m_Hzoom;
    /// Used to compute how far the horizontal zoom wheel has moved.
    int m_lastH;
    /// Thin vertical zoom wheel to the right of the big zoom wheel.
    Thumbwheel *m_Vzoom;
    /// Used to compute how far the vertical zoom wheel has moved.
    int m_lastV;
    /// Small reset button to the lower right of the big zoom wheel.
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
    ZoomableRulerScale *m_referenceScale;  // Owned by MatrixScene


    // Auto-scroll

    QTimer m_autoScrollTimer;
    FollowMode m_followMode;
    void startAutoScroll();
    void doAutoScroll();
    void stopAutoScroll();

};

}

#endif
