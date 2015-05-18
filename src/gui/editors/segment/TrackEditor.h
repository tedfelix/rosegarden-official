/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.

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

#include <map>


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
class RulerScale;
class RosegardenDocument;
class QDeferScrollView;
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
                RosegardenMainViewWidget *rosegardenMainViewWidget,
                RulerScale *rulerScale,
                bool showTrackLabels,
                double initialUnitsPerPixel,
                QWidget *parent);

    ~TrackEditor();

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

    /**
     * Add a new segment - DCOP interface
     */
    //virtual void addSegment(int track, int start, unsigned int duration);

    /// Calls update() on each of the rulers.
    void updateRulers();

    /// Are we scrolling as we play?
    bool isTracking() const { return m_playTracking; }

public slots:

    /// Scroll the view such that the numbered track is on-screen
    /**
     * RosegardenMainWindow::setDocument() connects this to
     * RosegardenDocument::makeTrackVisible(int).
     */
    void slotScrollToTrack(int trackPosition);

    /// Set the position pointer during playback
    /**
     * init() connects this to
     * RosegardenDocument::pointerPositionChanged(timeT).
     */
    void slotSetPointerPosition(timeT position);

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

    /// Act on a canvas scroll event
    //void slotCanvasScrolled(int, int);

    /**
     * Adjust the canvas size to that required for the composition
     */
    void slotReadjustCanvasSize();

    /**
     * Show the given loop on the ruler or wherever
     */
    void slotSetLoop(timeT start, timeT end);

    /**
     * Add given number of tracks
     */
    void slotAddTracks(unsigned int nbTracks, InstrumentId id, int position);

    /*
     * Delete a given track
     */
    void slotDeleteTracks(std::vector<TrackId> tracks);

    void slotDeleteSelectedSegments();
    void slotTurnRepeatingSegmentToRealCopies();
    void slotTurnLinkedSegmentsToRealCopies();

    void slotToggleTracking();

protected slots:
    // Dead Code.
//    void slotSegmentOrderChanged(int section, int fromIdx, int toIdx);

    //void slotTrackButtonsWidthChanged();

    /// Scroll the track buttons along with the segment canvas
    void slotVerticalScrollTrackButtons(int y);

signals:
    /**
     * Emitted when the represented data changed and the CompositionView
     * needs to update itself
     *
     * @see CompositionView::update()
     */
    // Dead Code.
//    void needUpdate();

    /**
     * sent back to RosegardenGUI
     */
    void stateChange(QString, bool);

    /**
     * A URI to a Rosegarden document was dropped on the canvas
     *
     * @see RosegardenGUI#slotOpenURL()
     */
    void droppedDocument(QString uri);

    /**
     * An audio file was dropped from the audio manager dialog
     */
    void droppedAudio(QString audioDesc);

    /**
     * An audio file was dropped from an external application and needs to be
     * inserted into AudioManagerDialog before adding to the composition.
     */
    void droppedNewAudio(QString audioDesc);

private:

    // QWidget overrides.
    virtual void dragEnterEvent(QDragEnterEvent *event);
    virtual void dropEvent(QDropEvent*);
    virtual void dragMoveEvent(QDragMoveEvent *);
    virtual void paintEvent(QPaintEvent* e);
    
    void init(RosegardenMainViewWidget *);

    bool isCompositionModified();
    void setCompositionModified(bool);
    
    /// return true if an actual move occurred between current and new position, newPosition contains the horiz. pos corresponding to newTimePosition
    bool handleAutoScroll(int currentPosition, timeT newTimePosition, double& newPosition);
    
    int getTrackCellHeight() const;

    void addCommandToHistory(Command *command);

    //--------------- Data members ---------------------------------

    RosegardenDocument      *m_doc;
    
    RulerScale              *m_rulerScale;
    TempoRuler              *m_tempoRuler;
    ChordNameRuler          *m_chordNameRuler;
    StandardRuler           *m_topStandardRuler;
    StandardRuler           *m_bottomStandardRuler;
    TrackButtons            *m_trackButtons;
    CompositionView         *m_compositionView;
    CompositionModelImpl    *m_compositionModel;
    QScrollArea             *m_trackButtonScroll;

    bool                     m_showTrackLabels;
    unsigned int             m_canvasWidth;
    unsigned int             m_compositionRefreshStatusId;
    bool                     m_playTracking;

    //typedef std::map<Segment *, unsigned int>
    //    SegmentRefreshStatusIdMap;
    //SegmentRefreshStatusIdMap m_segmentsRefreshStatusIds;

    double                   m_initialUnitsPerPixel;

};


}

#endif
