// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
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
class SegmentCanvas;
class RosegardenGUIDoc;
class BarButtons;
class TempoRuler;
class TrackButtons;
class MultiViewCommandHistory;
class KCommand;
class QCanvasRectangle;
class QScrollView;
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
                QWidget* parent = 0, const char* name = 0,
                WFlags f=0);

    ~TrackEditor();

    SegmentCanvas* getSegmentCanvas()       { return m_segmentCanvas; }
    TempoRuler*    getTempoRuler()          { return m_tempoRuler; }
    BarButtons*    getTopBarButtons()       { return m_topBarButtons; }
    BarButtons*    getBottomBarButtons()    { return m_bottomBarButtons; }
    TrackButtons*  getTrackButtons()        { return m_trackButtons; }
    QScrollBar*    getHorizontalScrollBar() { return m_horizontalScrollBar; }

    int getTrackCellHeight() const;

    /**
     * Must be called after construction and signal connection
     * if a document was passed to ctor, otherwise segments will
     * be created but not registered in the main doc
     */
    void setupSegments();

    /**
     * Add a new segment - DCOP interface
     */
    virtual void addSegment(int track, int start, unsigned int duration);

    /**
     * Manage command history
     */
    MultiViewCommandHistory *getCommandHistory();
    void addCommandToHistory(KCommand *command);


public slots:

//!!! I suspect most of these of never actually being used as slots, only as plain methods

    /**
     * Set the position pointer during playback
     */
    void slotSetPointerPosition(Rosegarden::timeT position);

    /**
     * Adjust the canvas size to that required for the composition
     */
    void slotReadjustCanvasSize();

    /**
     * Show the given loop on the ruler or wherever
     */
    void slotSetLoop(Rosegarden::timeT start, Rosegarden::timeT end);

    /**
     * Show a Segment as it records
     */
    void slotUpdateRecordingSegmentItem(Rosegarden::Segment *segment);

    /*
     * Destroys same
     */
    void slotDeleteRecordingSegmentItem();

    /**
     * c.f. what we have in rosegardenguiview.h
     * These are instrumental in passing through
     * key presses from GUI front panel down to
     * the SegmentCanvas.
     *
     */
    void slotSetSelectAdd(bool value);
    void slotSetSelectCopy(bool value);
    void slotSetFineGrain(bool value);

    /**
     * Scroll horizontally to make the given position visible,
     * paging to as to get some visibility of the next screenful
     * (for playback etc)
     */
    void slotScrollHoriz(int hpos);

    /**
     * Scroll horizontally to make the given position somewhat
     * nearer to visible, scrolling by only "a small distance"
     * at a time
     */
    void slotScrollHorizSmallSteps(int hpos);

    /**
     * Add given number of tracks
     */
    void slotAddTracks(unsigned int nbTracks, Rosegarden::InstrumentId id);

    void slotDeleteSelectedSegments();

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
    void stateChange(const QString&, bool);

    /**
     * A URI was dropped on the canvas
     *
     * @see RosegardenGUI#slotOpenURL()
     */
    void droppedURI(QString uri);

    /**
     * An audio file was dropped from the audio manager dialog
     */
    void droppedAudio(QString audioDesc);

protected:

    virtual void dragEnterEvent(QDragEnterEvent *event);
    virtual void dropEvent(QDropEvent*);
    
    virtual void paintEvent(QPaintEvent* e);
    
    void init(QWidget *);

    bool isCompositionModified();
    void setCompositionModified(bool);
    
    //--------------- Data members ---------------------------------

    RosegardenGUIDoc        *m_document;
    Rosegarden::RulerScale  *m_rulerScale;
    TempoRuler              *m_tempoRuler;
    BarButtons              *m_topBarButtons;
    BarButtons              *m_bottomBarButtons;
    TrackButtons            *m_trackButtons;
    QScrollBar              *m_horizontalScrollBar;
    SegmentCanvas           *m_segmentCanvas;
    QCanvasRectangle        *m_pointer;
    QScrollView             *m_trackButtonScroll;

    bool                     m_showTrackLabels;
    unsigned int             m_canvasWidth;
    unsigned int             m_compositionRefreshStatusId;

    typedef std::map<Rosegarden::Segment *, unsigned int>
        SegmentRefreshStatusIdMap;
    SegmentRefreshStatusIdMap m_segmentsRefreshStatusIds;
};

#endif
