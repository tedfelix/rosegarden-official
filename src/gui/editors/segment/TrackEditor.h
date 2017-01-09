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

#ifndef RG_TRACKEDITOR_H
#define RG_TRACKEDITOR_H

#include "base/MidiProgram.h"
#include "base/Event.h"
#include "gui/editors/segment/TrackButtons.h"

#include <QString>
#include <QWidget>
#include <QScrollArea>

#include <vector>


class QPaintEvent;
class QDropEvent;
class QDragEnterEvent;
class QScrollBar;
class QScrollArea;

namespace Rosegarden
{

class Command;
class TrackButtons;
class TempoRuler;
class Segment;
class SimpleRulerScale;
class RosegardenDocument;
class DeferScrollArea;
class CompositionView;
class CompositionModelImpl;
class ChordNameRuler;
class StandardRuler;

/// Holds the CompositionView, TrackButtons, and Rulers.
/**
 * An object of this class is created and owned by RosegardenMainViewWidget.
 * It can be accessed globally as follows:
 *
 *   RosegardenMainWindow::self()->getView()->getTrackEditor()
 *
 * This class owns instances of the following classes:
 *
 *   * CompositionView - The segment canvas.
 *   * TrackButtons - The buttons to the left of the segment canvas.
 *   * Various Rulers
 *
 * @see RosegardenMainViewWidget::getTrackEditor()
 */
class TrackEditor : public QWidget
{
    Q_OBJECT
public:
    TrackEditor(RosegardenDocument *doc,
                RosegardenMainViewWidget *mainViewWidget,
                SimpleRulerScale *rulerScale,
                bool showTrackLabels);

    // ??? These accessors are mostly for RosegardenMainViewWidget.
    //     Consider moving the code from RosegardenMainViewWidget into
    //     here and wrapping in functions at a higher level of
    //     abstraction instead of exposing internals.  The ruler
    //     functions should be easy to start with.  See updateRulers().

    CompositionView *getCompositionView()     { return m_compositionView; }
    TempoRuler      *getTempoRuler()          { return m_tempoRuler; }
    ChordNameRuler  *getChordNameRuler()      { return m_chordNameRuler; }
    StandardRuler   *getTopStandardRuler()    { return m_topStandardRuler; }
    StandardRuler   *getBottomStandardRuler() { return m_bottomStandardRuler; }
    TrackButtons    *getTrackButtons()        { return m_trackButtons; }
    //RulerScale      *getRulerScale()          { return m_rulerScale; }

    /// Calls update() on each of the rulers.
    void updateRulers();

    /// Are we scrolling as we play?
    bool isTracking() const { return m_playTracking; }
    /// Toggle playback scrolling.
    void toggleTracking();

    /// Adjust the canvas size to that required for the composition
    void updateCanvasSize();

    void addTracks(unsigned int nbTracks, InstrumentId id, int position);
    void deleteTracks(std::vector<TrackId> tracks);
    void deleteSelectedSegments();
    void turnRepeatingSegmentToRealCopies();
    void turnLinkedSegmentsToRealCopies();

public slots:

    /// Scroll the view such that the numbered track is on-screen
    /**
     * RosegardenMainWindow::setDocument() connects this to
     * RosegardenDocument::makeTrackVisible(int).
     */
    void slotScrollToTrack(int trackPosition);

signals:
    /**
     * init() connects this to RosegardenMainViewWidget::stateChange().
     *
     * Used to modify have_segments and have_selection states.
     */
    void stateChange(QString name, bool state);

    /// A URI to a Rosegarden document was dropped on the canvas.
    /**
     * RosegardenMainViewWidget's ctor connects this to
     * RosegardenMainWindow::slotOpenDroppedURL().
     */
    void droppedDocument(QString uri);

    /// An audio file was dropped from the audio manager dialog.
    /**
     * RosegardenMainViewWidget's ctor connects this to
     * RosegardenMainViewWidget::slotDroppedAudio().
     */
    void droppedAudio(QString audioDesc);

    /// An audio file was dropped from an external application.
    /**
     * Inserts the audio file into AudioManagerDialog before adding to the
     * composition.
     *
     * RosegardenMainViewWidget's ctor connects this to
     * RosegardenMainViewWidget::slotDroppedNewAudio().
     */
    void droppedNewAudio(QString audioDesc);

    /*
     * Emitted when the represented data changed and the CompositionView
     * needs to update itself
     *
     * @see CompositionView::update()
     */
    //void needUpdate();

private slots:
    /// Set the position pointer during playback
    /**
     * Scrolls as needed to keep the position pointer visible.
     *
     * init() connects this to
     * RosegardenDocument::pointerPositionChanged(timeT).
     */
    void slotSetPointerPosition(timeT pointerTime);

    /// Update the pointer position as it is being dragged along.
    /**
     * Scroll to make sure the pointer is visible.
     *
     * init() connects this to the top and bottom rulers'
     * dragPointerToPosition(timeT).
     */
    void slotPointerDraggedToPosition(timeT position);

    /// Scroll to make sure the loop end is visible.
    /**
     * init() connects this to the top and bottom rulers'
     * dragLoopToPosition(timeT).
     */
    void slotLoopDraggedToPosition(timeT position);

    /// Show the given loop on the rulers
    /**
     * init() connects this to RosegardenDocument::loopChanged().
     */
    void slotSetLoop(timeT start, timeT end);

    /// Scroll the track buttons along with the segment canvas
    void slotVerticalScrollTrackButtons(int y);

    /// Triggers a refresh if the composition has changed.
    void slotCommandExecuted();

    /// Adjust the size of the TrackButtons to match the CompositionView.
    /**
     * Connected to RosegardenScrollView::viewportResize().
     */
    void slotViewportResize();

    // Act on a canvas scroll event
    //void slotCanvasScrolled(int, int);
    //void slotSegmentOrderChanged(int section, int fromIdx, int toIdx);
    //void slotTrackButtonsWidthChanged();

private:

    /// Initialization routine called by ctor.
    void init(RosegardenMainViewWidget *);

    // QWidget overrides.
    virtual void dragEnterEvent(QDragEnterEvent *);
    virtual void dropEvent(QDropEvent *);
    virtual void dragMoveEvent(QDragMoveEvent *);

    /// Scroll when dragging the pointer or loop end.
    /**
     * Returns true if an actual move occurred between currentPosition and
     * newTimePosition.  Output parameter newPosition contains the horizontal
     * position corresponding to newTimePosition.
     */
    bool handleAutoScroll(
            int currentPosition, timeT newTimePosition, double &newPosition);

    /// Wrapper around CommandHistory::addCommand().
    void addCommandToHistory(Command *command);

    //--------------- Data members ---------------------------------

    RosegardenDocument      *m_doc;
    unsigned int             m_compositionRefreshStatusId;

    // Segment Canvas
    CompositionView         *m_compositionView;
    CompositionModelImpl    *m_compositionModel;
    bool                     m_playTracking;
    int                      m_trackCellHeight;

    // Track Buttons to the left of the Segment Canvas
    TrackButtons            *m_trackButtons;
    // Scrollable parent for the TrackButtons.
    DeferScrollArea         *m_trackButtonScroll;
    bool                     m_showTrackLabels;

    // Rulers
    SimpleRulerScale        *m_rulerScale;
    TempoRuler              *m_tempoRuler;
    ChordNameRuler          *m_chordNameRuler;
    StandardRuler           *m_topStandardRuler;
    StandardRuler           *m_bottomStandardRuler;

    //unsigned int             m_canvasWidth;
    //typedef std::map<Segment *, unsigned int> SegmentRefreshStatusIdMap;
    //SegmentRefreshStatusIdMap m_segmentsRefreshStatusIds;
};


}

#endif
