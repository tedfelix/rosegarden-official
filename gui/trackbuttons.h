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


#ifndef _TRACKBUTTONS_H_
#define _TRACKBUTTONS_H_

#include <vector>

#include <qframe.h>

class QVBoxLayout;
class QButtonGroup;
class TrackVUMeter;
class TrackLabel;
class RosegardenGUIDoc;

// This class creates a list of mute and record buttons
// based on the rosegarden document and a specialisation
// of the Vertical Box widget.
//
//
// 

class TrackButtons : public QFrame
{
    
    Q_OBJECT

public:
    TrackButtons(RosegardenGUIDoc* doc,
                 unsigned int trackCellHeight,
                 unsigned int trackLabelWidth,
                 QWidget* parent = 0,
                 const char* name = 0,
                 WFlags f=0);

    // Return the track selected for recording
    //
    int selectedRecordTrack();

    // Return a vector of muted tracks
    //
    std::vector<int> mutedTracks();

    // Return a vector of highlighted tracks
    //
    std::vector<int> getHighLightedTracks();

signals:
    // to emit what Track has been selected
    //
    void trackSelected(int);

public slots:

    // We use these to set the Track and Composition
    // objects with our returned values
    //
    void setRecordTrack(int id);
    void toggleMutedTrack(int mutedTrack);
    void labelSelected(int id);
    void renameTrack(QString newName, int trackNum);
    void setTrackMeter(double value, int trackNum);


private:

    void drawButtons();

    //--------------- Data members ---------------------------------

    RosegardenGUIDoc *m_doc;

    QButtonGroup *m_recordButtonGroup;
    QButtonGroup *m_muteButtonGroup;
    QVBoxLayout *m_layout;

    std::vector<TrackLabel *> m_trackLabels;
    std::vector<TrackVUMeter *> m_trackMeters;

    // Number of tracks on our view
    //
    int m_tracks;

    // The pixel offset from the top - just to overcome
    // the borders
    int m_offset;

    // The height of the cells
    //
    int m_cellSize;
    int m_lastID;
    int m_trackLabelWidth;

};


#endif // _TRACKBUTTONS_H_


