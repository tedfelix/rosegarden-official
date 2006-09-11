// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2006
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef SEGMENTSEDITOR_H
#define SEGMENTSEDITOR_H

#include <vector>

#include <qwidget.h>

#include "trackseditoriface.h"

#include "Event.h" // for timeT
#include "Track.h"
#include "Instrument.h"

#include "segmentcommands.h" // for SegmentReconfigureCommand::SegmentRec

namespace Rosegarden {
    class Segment;
    class RulerScale;
    class SegmentSelection;
}

class SegmentItem;
class CompositionView;
class CompositionModel;
class RosegardenGUIDoc;
class BarButtons;
class TempoRuler;
class ChordNameRuler;
class TrackButtons;
class MultiViewCommandHistory;
class KCommand;
class QDeferScrollView;
class QScrollBar;

/**
 * Global widget for segment edition.
 *
 * Shows a global overview of the composition, and lets the user
 * manipulate the segments
 *
 * @see SegmentCanvas
 */
class TrackEditor : public QWidget, virtual public TrackEditorIface
{
    Q_OBJECT
public:
    /**
     * Create a new TrackEditor representing the document \a doc
     */
    TrackEditor(RosegardenGUIDoc* doc,
                QWidget* rosegardenguiview,
                Rosegarden::RulerScale *rulerScale,
                bool showTrackLabels,
                double initialUnitsPerPixel = 0,
                QWidget* parent = 0, const char* name = 0,
                WFlags f=0);

    ~TrackEditor();

    CompositionView* getSegmentCanvas()       { return m_segmentCanvas; }
    TempoRuler*    getTempoRuler()          { return m_tempoRuler; }
    ChordNameRuler*getChordNameRuler()      { return m_chordNameRuler; }
    BarButtons*    getTopBarButtons()       { return m_topBarButtons; }
    BarButtons*    getBottomBarButtons()    { return m_bottomBarButtons; }
    TrackButtons*  getTrackButtons()        { return m_trackButtons; }
    Rosegarden::RulerScale*    getRulerScale() { return m_rulerScale; }

    int getTrackCellHeight() const;

    /**
     * Add a new segment - DCOP interface
     */
    virtual void addSegment(int track, int start, unsigned int duration);

    /**
     * Manage command history
     */
    MultiViewCommandHistory *getCommandHistory();
    void addCommandToHistory(KCommand *command);

    void updateRulers();

    bool isTracking() const { return m_playTracking; }

public slots:

    /**
     * Scroll the view such that the numbered track is on-screen
     */
    void slotScrollToTrack(int track);

    /**
     * Set the position pointer during playback
     */
    void slotSetPointerPosition(Rosegarden::timeT position);

    /**
     * Update the pointer position as it is being dragged along
     * This changes how the segment canvas will scroll to follow the pointer
     */
    void slotPointerDraggedToPosition(Rosegarden::timeT position);

    /**
     * Update the loop end position as it is being dragged along
     * This changes how the segment canvas will scroll to follow the pointer
     */
    void slotLoopDraggedToPosition(Rosegarden::timeT position);

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
    void slotSetLoop(Rosegarden::timeT start, Rosegarden::timeT end);

    /**
     * Add given number of tracks
     */
    void slotAddTracks(unsigned int nbTracks, Rosegarden::InstrumentId id);

    /*
     * Delete a given track
     */
    void slotDeleteTracks(std::vector<Rosegarden::TrackId> tracks);

    void slotDeleteSelectedSegments();
    void slotTurnRepeatingSegmentToRealCopies();

    void slotToggleTracking();

protected slots:
    void slotSegmentOrderChanged(int section, int fromIdx, int toIdx);

    void slotTrackButtonsWidthChanged();

    /// Scroll the track buttons along with the segment canvas
    void slotVerticalScrollTrackButtons(int y);

signals:
    /**
     * Emitted when the represented data changed and the SegmentCanvas
     * needs to update itself
     *
     * @see SegmentCanvas::update()
     */
    void needUpdate();

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

    virtual void dragEnterEvent(QDragEnterEvent *event);
    virtual void dropEvent(QDropEvent*);
    
    virtual void paintEvent(QPaintEvent* e);
    
    void init(QWidget *);

    bool isCompositionModified();
    void setCompositionModified(bool);
    
    /// return true if an actual move occurred between current and new position, newPosition contains the horiz. pos corresponding to newTimePosition
    bool handleAutoScroll(int currentPosition, Rosegarden::timeT newTimePosition, double& newPosition);
    
    //--------------- Data members ---------------------------------

    RosegardenGUIDoc        *m_doc;
    Rosegarden::RulerScale  *m_rulerScale;
    TempoRuler              *m_tempoRuler;
    ChordNameRuler          *m_chordNameRuler;
    BarButtons              *m_topBarButtons;
    BarButtons              *m_bottomBarButtons;
    TrackButtons            *m_trackButtons;
    CompositionView         *m_segmentCanvas;
    CompositionModel        *m_compositionModel;
    QDeferScrollView        *m_trackButtonScroll;

    bool                     m_showTrackLabels;
    unsigned int             m_canvasWidth;
    unsigned int             m_compositionRefreshStatusId;
    bool                     m_playTracking;

    typedef std::map<Rosegarden::Segment *, unsigned int>
        SegmentRefreshStatusIdMap;
    SegmentRefreshStatusIdMap m_segmentsRefreshStatusIds;

    double                   m_initialUnitsPerPixel;
};

#endif
