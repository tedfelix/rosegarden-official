
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2007
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


#ifndef _RG_HEADERSGROUP_H_
#define _RG_HEADERSGROUP_H_

#include "base/Track.h"

#include <vector>
#include <qsize.h>
#include <qwidget.h>
#include <qvbox.h>


class QLabel;


namespace Rosegarden
{


class NotationView;
class Composition;
class TrackHeader;


class HeadersGroup : public QVBox
{
    Q_OBJECT
public:
    /**
     * Create an empty headers group
     */
    HeadersGroup(QWidget *parent, NotationView * nv, Composition * comp);

    void removeAllHeaders();

    void addHeader(int trackId, int height, int ypos, double xcur);

    /**
     * Resize a filler at bottom of group to set the headersGroup height
     * to the value specified in parameter.
     * (Used to give to the headers group exactly the same height as the
     * canvas. Necessary to get synchronous vertical scroll.) 
     */
    void completeToHeight(int height);

    NotationView * getNotationView()
    { return m_notationView;
    }

    Composition * getComposition()
    { return m_composition;
    }

    /**
     * Return the total height of all the headers (without the filler).
     */
    int getUsedHeight()
    { return m_usedHeight;
    }

    /**
     * Highlight as "current" the header of the specified track.
     */
    void setCurrent(TrackId trackId);


public slots :
    /**
     * Called when notation canvas moves.
     * Setting force to true forces the headers to be redrawn even 
     * if x has not changed since the last call.
     */
    void slotUpdateAllHeaders(int x, int y, bool force = false);

private:
    NotationView * m_notationView;
    Composition * m_composition;
    
    typedef std::vector<TrackHeader *> TrackHeaderVector;
    TrackHeaderVector m_headers;

    int m_usedHeight;
    QLabel * m_filler;
    int m_lastX;
    int m_width;
};


}

#endif
