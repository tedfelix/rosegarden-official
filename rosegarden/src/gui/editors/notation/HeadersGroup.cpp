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



#include <qsize.h>
#include <qwidget.h>
#include <qvbox.h>
#include <qlabel.h>

#include "HeadersGroup.h"
#include "TrackHeader.h"
#include "NotationView.h"
#include "NotePixmapFactory.h"


namespace Rosegarden
{


HeadersGroup::
HeadersGroup(QWidget *parent, NotationView * nv, Composition * comp) :
        QVBox(parent),
        m_notationView(nv),
        m_composition(comp),
        m_usedHeight(0),
        m_filler(0),
        m_lastX(-2147483648),
        m_width(0)
{
}

void
HeadersGroup::removeAllHeaders()
{
    TrackHeaderVector::iterator i;
    for (i=m_headers.begin(); i!=m_headers.end(); i++) {
        delete *i;
    }
    m_headers.erase(m_headers.begin(), m_headers.end());

    if (m_filler) {
        delete m_filler;
        m_filler = 0;
    }
    m_usedHeight = 0;
}

void
HeadersGroup::addHeader(int trackId, int height, int ypos, double xcur)
{
    TrackHeader * sh = new TrackHeader(this, trackId, height, ypos);
    m_headers.push_back(sh);
    m_usedHeight += height;
    sh->updateHeader(xcur);
}

void
HeadersGroup::completeToHeight(int height)
{
    if (height > m_usedHeight) {
        if (!m_filler) m_filler = new QLabel(this);
        m_filler->setFixedHeight(height - m_usedHeight);
    }
}

void
HeadersGroup::slotUpdateAllHeaders(int x, int y, bool force)
{
    double xleft = m_notationView->getCanvasLeftX();

    /// Sometimes, when editor starts, xleft is null but x is not
    /// and the displayed headers are wrong.
    /// The following line is a quick workaround before finding
    /// some better fix.
    if ((x != 0) && (xleft == 0)) xleft = x;

    if ((x != m_lastX) || force) {
        m_lastX = x;
        TrackHeaderVector::iterator i;
        for (i=m_headers.begin(); i!=m_headers.end(); i++) {
            (*i)->updateHeader(xleft);
        }
    }
}

void
HeadersGroup::setCurrent(TrackId trackId)
{
    TrackHeaderVector::iterator i;
    for (i=m_headers.begin(); i!=m_headers.end(); i++)
                    (*i)->setCurrent((*i)->getId() == trackId);
}

}
#include "HeadersGroup.moc"
