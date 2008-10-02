
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.

    This file is Copyright 2007-2008
        Yves Guillemot      <yc.guillemot@wanadoo.fr> 

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
#include <QSize>
#include <QWidget>
#include <QVBoxLayout>

class QLabel;
class QResizeEvent;


namespace Rosegarden
{


class NotationView;
class Composition;
class TrackHeader;
//class QVBoxLayout;


class HeadersGroup : public QWidget
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

    /**
     * Highlight as "current" the header of the specified track.
     */
    int getWidth()
    {
        return m_lastWidth;
    }

    typedef enum { ShowNever, ShowWhenNeeded, ShowAlways } ShowHeadersModeType;

    // Used to ensure to have one default value and only one.
    static const ShowHeadersModeType DefaultShowMode = ShowAlways;

    // Useful in configuration dialog.
    static bool isValidShowMode(int mode)
    {
        return ((mode >= ShowNever) && (mode <= ShowAlways));
    }

public slots :
    /**
     * Called when notation canvas moves.
     * Setting force to true forces the headers to be redrawn even 
     * if x has not changed since the last call.
     */
    void slotUpdateAllHeaders(int x, int y, bool force = false);

signals :
    void headersResized(int newWidth);

private:
    void resizeEvent(QResizeEvent * ev);

    NotationView * m_notationView;
    Composition * m_composition;

    typedef std::vector<TrackHeader *> TrackHeaderVector;
    TrackHeaderVector m_headers;

    int m_usedHeight;
    QLabel * m_filler;
    int m_lastX;
    int m_lastWidth;

    QVBoxLayout *m_layout;
};


}

#endif
