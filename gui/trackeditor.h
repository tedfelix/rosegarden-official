
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

class TrackPart;
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

    bool moveTrack(int section, int fromIdx, int toIdx);

    // called by parent view widget when reading a music file
    bool addTrackPart(unsigned int trackNb,
                      unsigned int start, unsigned int nbBars);
    
    TrackPart* getTrackAtIdx(int idx);

    TracksCanvas*       canvas()       { return m_tracksCanvas; }
    const TracksCanvas* canvas() const { return m_tracksCanvas; }

protected slots:
    void trackOrderChanged(int section, int fromIdx, int toIdx);
    void addTrackPart(TrackPart*);
    void deleteTrackPart(TrackPart*);

signals:
    void needUpdate();
    void createNewTrack(unsigned int trackNb,
                        unsigned int nbBars,
                        unsigned int startAt);

protected:

    void init(unsigned int nbTracks, unsigned int nbBars);
    void setupTracks();

    RosegardenGUIDoc* m_document;

    TracksCanvas *m_tracksCanvas;
    QHeader *m_hHeader, *m_vHeader;

    list<TrackPart*> m_trackParts;
};

#endif
