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
#include "Selection.h"

#include "segmentcommands.h" // for SegmentReconfigureCommand::SegmentRec

namespace Rosegarden { class Segment; class RulerScale; }
class SegmentItem;
class SegmentCanvas;
class RosegardenGUIDoc;
class BarButtons;
class TrackButtons;
class MultiViewCommandHistory;
class KCommand;
class QCanvasRectangle;
class QScrollView;

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
                Rosegarden::RulerScale *rulerScale,
                bool showTrackLabels,
                QWidget* parent = 0, const char* name = 0,
                WFlags f=0);

    ~TrackEditor();

    /// Clear the SegmentCanvas
    void clear();

    SegmentCanvas* getSegmentCanvas()       { return m_segmentCanvas; }
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
     * Scroll horizontally to given position
     */
    void slotScrollHoriz(int hpos);

    /**
     * Add given number of tracks
     */
    void slotAddTracks(unsigned int nbTracks);

protected slots:
    void slotSegmentOrderChanged(int section, int fromIdx, int toIdx);

    void slotTrackButtonsWidthChanged();

    void slotSelectedSegments(const Rosegarden::SegmentSelection &segments);

    void slotDeleteSelectedSegments();

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
     * Emitted when the SegmentCanvas needs to be scrolled 
     * horizontally to a position
     *
     * @see 
     */
    void scrollHorizTo(int);

    /*
     * Send up to RosegardenGUIView to select track of Segments
     *
     */
    void trackSelected(int);

    /*
     * Send up to RosegardenGUIView
     *
     */
    void instrumentSelected(int);

    /*
     * Send up to RosegardenGUIView
     *
     */
    void selectedSegments(const Rosegarden::SegmentSelection &segments);

protected:
    
    virtual void paintEvent(QPaintEvent* e);
    
    void init(unsigned int nbTracks, int firstBar, int lastBar);

    bool isCompositionModified();
    void setCompositionModified(bool);
    
    //--------------- Data members ---------------------------------

    RosegardenGUIDoc        *m_document;
    Rosegarden::RulerScale  *m_rulerScale;
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
};

#endif
