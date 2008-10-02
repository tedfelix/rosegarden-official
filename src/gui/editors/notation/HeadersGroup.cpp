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


#include <limits>

#include "HeadersGroup.h"
#include "TrackHeader.h"
#include "NotationView.h"
#include "NotePixmapFactory.h"

#include <QSize>
#include <QWidget>
#include <QLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QEvent>
#include <QResizeEvent>


namespace Rosegarden
{


HeadersGroup::
HeadersGroup(QWidget *parent, NotationView * nv, Composition * comp) :
        QWidget(parent),
        m_notationView(nv),
        m_composition(comp),
        m_usedHeight(0),
        m_filler(0),
        m_lastX(INT_MIN),
        m_lastWidth(-1),
        m_layout( new QVBoxLayout(this) )
{
}

void
HeadersGroup::removeAllHeaders()
{
    delete m_layout;

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
    m_lastWidth = -1;
}

void
HeadersGroup::addHeader(int trackId, int height, int ypos, double xcur)
{
    TrackHeader * sh = new TrackHeader(this, trackId, height, ypos);
    m_layout->addWidget(sh);
    m_headers.push_back(sh);
    m_usedHeight += height;
}

void
HeadersGroup::completeToHeight(int height)
{
    if (height > m_usedHeight) {
        if (!m_filler) {
            m_filler = new QLabel(this);
            m_layout->addWidget(m_filler);
        }
        m_filler->setFixedHeight(height - m_usedHeight);
    }
    setLayout(m_layout);  // May it harm to call setLayout more than once ?
}

void
HeadersGroup::slotUpdateAllHeaders(int x, int y, bool force)
{
    // Minimum header width 
    int headerMinWidth = m_notationView->getHeadersTopFrameMinWidth();

    // Maximum header width (may be overriden by clef and key width)
    int headerMaxWidth = (m_notationView->getCanvasVisibleWidth() * 10) / 100;

    if ((x != m_lastX) || force) {
        m_lastX = x;
        TrackHeaderVector::iterator i;
        int neededWidth = 0;

        // Pass 1 : get the max width needed
        for (i=m_headers.begin(); i!=m_headers.end(); i++) {
            int w = (*i)->lookAtStaff(x, headerMaxWidth);
            if (w > neededWidth) neededWidth = w;
        }

        if (neededWidth < headerMinWidth) neededWidth = headerMinWidth;

        // Only when m_lastWidth is valid (the first time, m_lastWidth = -1)
        if (m_lastWidth > 0) {
            // Don't redraw the headers when change of width is very small
            const int treshold = 10;   // Treshold value should be refined ...
            int deltaWidth = m_lastWidth - neededWidth;
            if ((deltaWidth < treshold) && (deltaWidth > -treshold))
                neededWidth = m_lastWidth;
        }

        // Pass 2 : redraw the headers when necessary
        for (i=m_headers.begin(); i!=m_headers.end(); i++) {
            (*i)->updateHeader(neededWidth);
        }

        if (neededWidth != m_lastWidth) {
            setFixedWidth(neededWidth);
            m_lastWidth = neededWidth;

            // Suppress vertical white stripes on canvas when headers
            // width changes while scrolling
            /// TODO : Limit "setChanged()" to the useful part of canvas
            m_notationView->canvas()->setAllChanged();
            m_notationView->canvas()->update();
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

void
HeadersGroup::resizeEvent(QResizeEvent * ev)
{
    // Needed to avoid gray zone at the right of headers
    // when width is decreasing
    emit headersResized( ev->size().width() );
}

}
#include "HeadersGroup.moc"
