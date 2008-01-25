
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2008
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>

    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#ifndef _RG_TRACKHEADER_H_
#define _RG_TRACKHEADER_H_

#include "base/NotationTypes.h"
#include "base/Track.h"

#include <qsize.h>
#include <qwidget.h>
#include <qlabel.h>

class QLabel;


namespace Rosegarden
{

class NotePixmapFactory;
class NotationView;
class ColourMap;

class TrackHeader : public QLabel
{
    Q_OBJECT
public:
    /**
     * Create a new track header for track of id trackId.
     * *parent is the parent widget, height the height of staff and
     * ypos is the staff y position on canvas.
     */
    TrackHeader(QWidget *parent, TrackId trackId, int height, int ypos);

    /**
     * Update header to canvas position x
     * (Called when notation canvas moves)
     */
    void updateHeader(double x);

    /**
     * Draw a blue line around header when current is true
     * (intended to highlight the "current" track).
     */
    void setCurrent(bool current);

    /**
     * Return the Id of the associated track.
     */
    TrackId getId()
    { return m_track;
    }


private :
    /**
     * Convert the transpose value to the instrument tune and
     * return it in a printable string.
     */
    void transposeValueToName(int transpose, QString &transposeName);


    TrackId m_track;
    int m_height;
    int m_ypos;
    NotationView * m_notationView;

    Clef m_lastClef;
    Rosegarden::Key m_lastKey;
    QString m_lastLabel;
    int m_lastTranspose;
    bool m_neverUpdated;
    bool m_isCurrent;
    int m_lastStatusPart;
};

}

#endif
