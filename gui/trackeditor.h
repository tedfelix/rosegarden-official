
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

namespace Rosegarden { class Track; }
class TrackItem;
class TracksCanvas;
class RosegardenGUIDoc;

/**
 * Global widget for track edition.
 * Shows a global overview of the composition.
 *
 *@author Guillaume Laurent, Chris Cannam, Rich Bown
 */

class TracksEditor : public QWidget  {
   Q_OBJECT
public: 
    TracksEditor(RosegardenGUIDoc* doc,
                 QWidget* parent = 0, const char* name = 0,
                 WFlags f=0);

    TracksEditor(unsigned int nbTracks,
                 unsigned int nbBars,
                 QWidget* parent = 0, const char* name = 0,
                 WFlags f=0);

    void clear();

    void moveTrack(int section, int fromIdx, int toIdx);

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

public slots:
    /**
     * Sets the horizontal resolution of the grid in MIDI timesteps
     * (default is 384 - a whole note)
     */
    void setTimeStepsResolution(unsigned int);
    
protected slots:
    void trackOrderChanged(int section, int fromIdx, int toIdx);
    void addTrack(TrackItem*);
    void deleteTrack(Rosegarden::Track*);
    void updateTrackInstrumentAndStartIndex(TrackItem*);

signals:
    void needUpdate();
    void createNewTrack(TrackItem*);

protected:

    void init(unsigned int nbTracks, unsigned int nbBars);
    void setupHorizontalHeader();

    RosegardenGUIDoc* m_document;

    TracksCanvas *m_tracksCanvas;
    QHeader *m_hHeader, *m_vHeader;

    unsigned int m_timeStepsResolution;
};

#endif
