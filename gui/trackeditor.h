// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2001
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

#ifndef TRACKSEDITOR_H
#define TRACKSEDITOR_H

#include <list>
#include <qwidget.h>
#include <qheader.h>
#include <qcanvas.h>
#include "trackheader.h"

#include "trackseditoriface.h"

namespace Rosegarden { class Track; }
class TrackItem;
class TracksCanvas;
class RosegardenGUIDoc;

/**
 * Global widget for track edition.
 *
 * Shows a global overview of the composition, and lets the user
 * manipulate the tracks
 *
 * @see TracksCanvas
 */
class TracksEditor : public QWidget, virtual public TracksEditorIface
{
    Q_OBJECT
public:
    /**
     * Create a new TracksEditor representing the document \a doc
     */
    TracksEditor(RosegardenGUIDoc* doc,
                 QWidget* parent = 0, const char* name = 0,
                 WFlags f=0);

    /**
     * Create a new empty TracksEditor 
     *
     * The TracksEditor will have \a nbTracks available tracks, and
     * ( \a nbBars * time resolution ) time steps.
     *
     * @see setTimeStepsResolution()
     */
    TracksEditor(unsigned int nbTracks,
                 unsigned int nbBars,
                 QWidget* parent = 0, const char* name = 0,
                 WFlags f=0);

    /// Clear the TracksCanvas
    void clear();

    /**
     * Reset all the tracks Y coordinates after the order of the
     * instruments has been changed
     */
    void updateTrackOrder();

    TracksCanvas*       canvas()       { return m_tracksCanvas; }
    const TracksCanvas* canvas() const { return m_tracksCanvas; }

    /**
     * Must be called after construction and signal connection
     * if a document was passed to ctor, otherwise tracks will
     * be created but not registered in the main doc
     */
    void setupTracks();

    /**
     * Returns the horizontal resolution of the grid in MIDI timesteps
     */
    unsigned int getTimeStepsResolution() const;

    /**
     * Add a new track - DCOP interface
     */
    virtual void addTrack(int instrument, int start, unsigned int nbTimeSteps);


public slots:
    /**
     * Sets the horizontal resolution of the grid in MIDI timesteps
     * (default is 384 - a whole note)
     */
    void setTimeStepsResolution(unsigned int);
    
    /**
     * Set the position pointer during playback
     */
    void setPointerPosition(int position);

protected slots:
    void trackOrderChanged(int section, int fromIdx, int toIdx);
    void addTrack(TrackItem*);
    void deleteTrack(Rosegarden::Track*);
    void updateTrackInstrumentAndStartIndex(TrackItem*);

signals:
    /**
     * Emitted when the represented data changed and the TracksCanvas
     * needs to update itself
     *
     * @see TracksCanvas::update()
     */
    void needUpdate();

    /**
     * Emitted when a new track is created by the user
     * \a item is the TrackItem representing the track on the TracksCanvas
     * \a instrument is the instrument for the track
     *
     * It is up to the slot to create the new Track, to insert it in the
     * Composition and to set its instrument.
     *
     * @see RosegardenGUIDoc::createNewTrack()
     */
    void createNewTrack(TrackItem* item, int instrument);

protected:

    void init(unsigned int nbTracks, unsigned int nbBars);
    void setupHorizontalHeader();

    RosegardenGUIDoc* m_document;

    TracksCanvas *m_tracksCanvas;
    QHeader *m_hHeader;
    Rosegarden::TrackHeader *m_vHeader;

    unsigned int m_timeStepsResolution;

    QCanvasLine *m_pointer;
};

#endif
