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

#include <list>
#include <qwidget.h>
#include <qheader.h>
#include <qcanvas.h>
#include "trackheader.h"

#include "trackseditoriface.h"
#include "multiviewcommandhistory.h"

#include "Event.h" // for timeT
#include "Track.h"

namespace Rosegarden { class Segment; class RulerScale; }
class SegmentItem;
class SegmentCanvas;
class RosegardenGUIDoc;
class BarButtons;
class TrackButtons;

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
                QWidget* parent = 0, const char* name = 0,
                WFlags f=0);

    /// Clear the SegmentCanvas
    void clear();

    SegmentCanvas*       getSegmentCanvas()       { return m_segmentCanvas; }
    const SegmentCanvas* getSegmentCanvas() const { return m_segmentCanvas; }

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
    //!!! can the DCOP and the slot not be the same method? this is most confusing
    virtual void addSegment(int track, int start, unsigned int nbTimeSteps) {
	addSegment(Rosegarden::TrackId(track),
		   Rosegarden::timeT(start),
		   Rosegarden::timeT(nbTimeSteps));
    }

    /**
     * Manage command history
     */
    MultiViewCommandHistory *getCommandHistory();
    void addCommandToHistory(KCommand *command);


public slots:

    /**
     * Receive notification from the command history that a
     * command has happened
     */
    void commandExecuted(KCommand *);

    /**
     * Set the position pointer during playback
     */
    void setPointerPosition(Rosegarden::timeT position);

    /**
     * Create a Segment Item from a Segment (after recording)
     *
     */
    //!!! go?
    void addSegmentItem(Rosegarden::Segment *segment);

    /*
     * Delete a SegmentItem
     */
    void deleteSegmentItem(Rosegarden::Segment *segment);

    /**
     * Show a Segment as it records
     */
    void updateRecordingSegmentItem(Rosegarden::Segment *segment);

    /*
     * Resync a SegmentItem to reflect its Segment
     **/
//!!! go
    void updateSegmentItem(Rosegarden::Segment *segment);

    /*
     * Destroys same
     */
    void deleteRecordingSegmentItem();

    /**
     * c.f. what we have in rosegardenguiview.h
     * These are instrumental in passing through
     * key presses from GUI front panel down to
     * the SegmentCanvas.
     *
     */
    void setSelectAdd(bool value);
    void setSelectCopy(bool value);
    void setFineGrain(bool value);


protected slots:
    void segmentOrderChanged(int section, int fromIdx, int toIdx);

    void addSegment(Rosegarden::TrackId track,
                    Rosegarden::timeT time,
                    Rosegarden::timeT duration);

    void deleteSegment(Rosegarden::Segment *);
    void updateSegmentDuration(Rosegarden::Segment *,
			       Rosegarden::timeT);
    void updateSegmentTrackAndStartTime(Rosegarden::Segment *,
					Rosegarden::TrackId,
					Rosegarden::timeT);

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

    /**
     * Emitted when a new segment is created by the user
     * \a item is the SegmentItem representing the segment on the SegmentCanvas
     * \a instrument is the instrument for the segment
     *
     * It is up to the slot to create the new Segment, to insert it in the
     * Composition and to set its instrument.
     *
     * @see RosegardenGUIDoc::createNewSegment()
     */
//!!!    void createNewSegment(Rosegarden::timeT,
//                          Rosegarden::timeT,
//                          Rosegarden::TrackId);

    void splitSegment(Rosegarden::Segment*, Rosegarden::timeT);

protected:

    void init(unsigned int nbTracks, int firstBar, int lastBar);

    //--------------- Data members ---------------------------------

    RosegardenGUIDoc        *m_document;
    Rosegarden::RulerScale  *m_rulerScale;
    BarButtons              *m_barButtons;
    TrackButtons            *m_trackButtons;
    SegmentCanvas           *m_segmentCanvas;
    QCanvasLine             *m_pointer;
};

#endif
