/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2012 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_TRACKEDITOR_H_
#define _RG_TRACKEDITOR_H_

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
class CompositionModel;
class ChordNameRuler;
class StandardRuler;

/**
 * Global widget for segment edition.
 *
 * Shows a global overview of the composition, and lets the user
 * manipulate the segments.
 *
 * An object of this class is created and owned by RosegardenMainViewWidget.
 *
 * @see CompositionView
 * @see RosegardenMainViewWidget::getTrackEditor()
 */
class TrackEditor : public QWidget
{
    Q_OBJECT
public:
    /**
     * Create a new TrackEditor representing the document \a doc
     */
    TrackEditor(RosegardenDocument* doc,
                QWidget* rosegardenguiview,
                RulerScale *rulerScale,
                bool showTrackLabels,
                double initialUnitsPerPixel = 0,
                QWidget* parent = 0);

    ~TrackEditor();

    CompositionView *getCompositionView()     { return m_compositionView; }
    TempoRuler      *getTempoRuler()          { return m_tempoRuler; }
    ChordNameRuler  *getChordNameRuler()      { return m_chordNameRuler; }
    StandardRuler   *getTopStandardRuler()    { return m_topStandardRuler; }
    StandardRuler   *getBottomStandardRuler() { return m_bottomStandardRuler; }
    TrackButtons    *getTrackButtons()        { return m_trackButtons; }
    RulerScale      *getRulerScale()          { return m_rulerScale; }

    int getTrackCellHeight() const;

    /**
     * Add a new segment - DCOP interface
     */
    virtual void addSegment(int track, int start, unsigned int duration);

    /**
     * Manage command history
     */
    void addCommandToHistory(Command *command);

    void updateRulers();

    bool isTracking() const { return m_playTracking; }

public slots:

    /**
     * Scroll the view such that the numbered track is on-screen
     */
    void slotScrollToTrack(int trackPosition);

    /**
     * Set the position pointer during playback
     */
    void slotSetPointerPosition(timeT position);

    /**
     * Update the pointer position as it is being dragged along
     * This changes how the segment canvas will scroll to follow the pointer
     */
    void slotPointerDraggedToPosition(timeT position);

    /**
     * Update the loop end position as it is being dragged along
     * This changes how the segment canvas will scroll to follow the pointer
     */
    void slotLoopDraggedToPosition(timeT position);

    /**
     * Act on a canvas scroll event
     */
    void slotCanvasScrolled(int, int);

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
     * And audio file was dropped from konqi say and needs to be
     * inserted into AudioManagerDialog before adding to the 
     * composition.
     */
    void droppedNewAudio(QString audioDesc);

protected:

    // QWidget overrides.
    virtual void dragEnterEvent(QDragEnterEvent *event);
    virtual void dropEvent(QDropEvent*);
    virtual void dragMoveEvent(QDragMoveEvent *);
    virtual void paintEvent(QPaintEvent* e);
    
    void init(QWidget *);

    bool isCompositionModified();
    void setCompositionModified(bool);
    
    /// return true if an actual move occurred between current and new position, newPosition contains the horiz. pos corresponding to newTimePosition
    bool handleAutoScroll(int currentPosition, timeT newTimePosition, double& newPosition);
    
    //--------------- Data members ---------------------------------

    RosegardenDocument      *m_doc;
    
    RulerScale              *m_rulerScale;
    TempoRuler              *m_tempoRuler;
    ChordNameRuler          *m_chordNameRuler;
    StandardRuler           *m_topStandardRuler;
    StandardRuler           *m_bottomStandardRuler;
    TrackButtons            *m_trackButtons;
    CompositionView         *m_compositionView;
    CompositionModel        *m_compositionModel;
    QScrollArea             *m_trackButtonScroll;

    bool                     m_showTrackLabels;
    unsigned int             m_canvasWidth;
    unsigned int             m_compositionRefreshStatusId;
    bool                     m_playTracking;

    typedef std::map<Segment *, unsigned int>
        SegmentRefreshStatusIdMap;
    SegmentRefreshStatusIdMap m_segmentsRefreshStatusIds;

    double                   m_initialUnitsPerPixel;

private:
};


}

#endif
