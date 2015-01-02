/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.

    This file is Copyright 2007-2009
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
#include "StaffHeader.h"
#include "NotationWidget.h"
#include "NotationScene.h"
#include "NotationStaff.h"
#include "NotationHLayout.h"
#include "NotePixmapFactory.h"
#include "document/RosegardenDocument.h"

#include <QSize>
#include <QWidget>
#include <QLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QEvent>
#include <QResizeEvent>
#include <QString>


namespace Rosegarden
{


HeadersGroup::
HeadersGroup(RosegardenDocument *document) :
        QWidget(0),
        m_composition(document->getComposition()),
        m_scene(0),
        m_widget(0),
        m_usedHeight(0),
        m_filler(0),
        m_lastX(INT_MIN),
        m_lastWidth(-1),
        m_layout(0),
        m_startOfView(0),
        m_endOfView(0),
        m_currentSegment(0),
        m_currentSegStartTime(0),
        m_currentSegEndTime(0),
        m_currentTrackId(0)
{
    m_layout = new QVBoxLayout();
    m_layout->setSpacing(0);
    m_layout->setContentsMargins(0, 0, 0, 0);
    setLayout(m_layout);
}

HeadersGroup::
~HeadersGroup()
{
    // nothing here
}

void
HeadersGroup::removeAllHeaders()
{
    TrackHeaderVector::iterator i;
    for (i=m_headers.begin(); i!=m_headers.end(); ++i) {
        disconnect(*i, SIGNAL(showToolTip(QString)),
                   m_widget, SLOT(slotShowHeaderToolTip(QString)));
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
HeadersGroup::addHeader(int trackId, int height, int ypos, double /* xcur */)
{
    StaffHeader *sh = new StaffHeader(this, trackId, height, ypos);
    m_layout->addWidget(sh);
    m_headers.push_back(sh);
    m_usedHeight += height;

    connect(sh, SIGNAL(showToolTip(QString)),
            m_widget, SLOT(slotShowHeaderToolTip(QString)));

    connect(sh, SIGNAL(staffModified()),
            m_widget, SLOT(slotRegenerateHeaders()), Qt::QueuedConnection);
            // Without Qt::QueuedConnection, headers may be deleted
            // from themselves leading to crash
}

void
HeadersGroup::setTracks(NotationWidget *widget, NotationScene *scene)
{
    if (m_scene) disconnect(m_scene, SIGNAL(currentStaffChanged()),
                            this, SLOT(slotSetCurrentSegment()));

    m_scene = scene;
    m_widget = widget;

    connect(m_scene, SIGNAL(currentStaffChanged()),
            this, SLOT(slotSetCurrentSegment()));
    slotSetCurrentSegment();


    // std::vector<NotationStaff *> *staffs = scene->getStaffs();

    TrackIntMap *trackHeights = scene->getTrackHeights();
    TrackIntMap *trackCoords = scene->getTrackCoords();
    int minTrack = scene->getMinTrack();
    int maxTrack = scene->getMaxTrack();

    // Destroy then recreate all track headers
    removeAllHeaders();

    if (m_scene->getPageMode() == StaffLayout::LinearMode) {
        for (int i = minTrack; i <= maxTrack; ++i) {
            TrackIntMap::iterator hi = trackHeights->find(i);
            if (hi != trackHeights->end()) {
                TrackId trackId = m_composition
                                        .getTrackByPosition(i)->getId();
                addHeader(trackId, (*trackHeights)[i],
                          (*trackCoords)[i],
                          m_widget->getViewLeftX());
            }
        }

        slotUpdateAllHeaders(m_widget->getViewLeftX(), true);
    }

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
}

void
HeadersGroup::slotUpdateAllHeaders(int x, bool force)
{
    // Minimum header width 
    /// TODO : use a real button width got from a real button
    // 2 buttons (2 x 24) + 2 margins (2 x 4) + buttons spacing (4)
    int headerMinWidth =  4 + 24 + 4 + 24 + 4;

    // Maximum header width (may be overriden by clef and key width)
    int headerMaxWidth = (m_widget->getNotationViewWidth() * 10) / 100;

    if ((x != m_lastX) || force) {
        m_lastX = x;
        TrackHeaderVector::iterator i;
        int neededWidth = 0;

        // Compute times of left and right of view
        NotationHLayout *nhl = m_scene->getHLayout();
        m_startOfView = nhl->getTimeForX(x);
        m_endOfView = nhl->getTimeForX(m_widget->getViewRightX());

// int barStart = m_composition.getBarNumber(m_startOfView) + 1;
// int barEnd = m_composition.getBarNumber(m_endOfView) + 1;
// std::cerr << "XXX (" << m_startOfView << ", " << m_endOfView
//           << "   ["<< barStart << ", " << barEnd << "]\n";

        // Pass 1 : get the max width needed
        for (i=m_headers.begin(); i!=m_headers.end(); ++i) {
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
        for (i=m_headers.begin(); i!=m_headers.end(); ++i) {
           (*i)->updateHeader(neededWidth);
        }

        if (neededWidth != m_lastWidth) {
            setFixedWidth(neededWidth);
            m_lastWidth = neededWidth;

            emit headersResized(m_lastWidth);
        }
    }
}


void
HeadersGroup::slotSetCurrentSegment()
{
    NotationStaff *notationStaff = m_scene->getCurrentStaff();
    m_currentSegment = &(notationStaff->getSegment());
    m_currentSegStartTime = m_currentSegment->getStartTime();
    m_currentSegEndTime = m_currentSegment->getEndMarkerTime();
    m_currentTrackId = m_currentSegment->getTrack();

    emit currentSegmentChanged();
}


bool
HeadersGroup::timeIsInCurrentSegment(timeT t)
{
    return (t >= m_currentSegStartTime) && (t < m_currentSegEndTime);
}


NotationScene *
HeadersGroup::getScene()
{
    return m_scene;
}

QSize
HeadersGroup::sizeHint() const
{
    return QSize(m_lastWidth, m_usedHeight);
}

QSize
HeadersGroup::minimumSizeHint() const
{
    return QSize(m_lastWidth, m_usedHeight);
}

}
#include "HeadersGroup.moc"

