/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_AUTOSCROLLER_H
#define RG_AUTOSCROLLER_H

#include <QObject>
#include <QTimer>

class QAbstractScrollArea;
class QWidget;

namespace Rosegarden
{


typedef int FollowMode;
constexpr int NO_FOLLOW = 0;  // Was NoFollow
constexpr int FOLLOW_HORIZONTAL = 1;  // Was FollowHorizontal
constexpr int FOLLOW_VERTICAL = 2;  // Was FollowVertical

/// Handle auto-scroll behavior when the mouse is dragged outside a window.
/**
 * See RosegardenScrollView::doAutoScroll().
 *
 * This is really nothing more than an addition to QAbstractScrollArea.
 * Perhaps we should consider generalizing it (offer fixed/variable in all
 * directions, edge of screen detection in all directions, etc...) and creating
 * an AutoScrollArea that adds all this.  Then we can derive from that.
 */
class AutoScroller : public QObject
{
    Q_OBJECT

public:

    AutoScroller();

    // Setup

    /// QAbstractScrollArea that we will be auto-scrolling.
    void connectScrollArea(QAbstractScrollArea *i_abstractScrollArea)
            { m_abstractScrollArea = i_abstractScrollArea; }
    void setVScrollRate(int i_vScrollRate)
            { m_vScrollRate = i_vScrollRate; }

    // Control

    void start();
    void setFollowMode(FollowMode i_followMode)
            { m_followMode = i_followMode; }
    void stop();

    bool isRunning() const  { return m_timer.isActive(); }

private slots:

    void slotOnTimer();

private:

    QAbstractScrollArea *m_abstractScrollArea;
    int m_vScrollRate;

    FollowMode m_followMode;

    QTimer m_timer;

    void doAutoScroll();

};


}

#endif
