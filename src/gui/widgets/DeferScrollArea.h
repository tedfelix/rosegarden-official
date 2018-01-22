/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_DEFER_SCROLL_AREA_H
#define RG_DEFER_SCROLL_AREA_H

#include <QScrollArea>

namespace Rosegarden
{

/// QScrollArea that emits a signal on wheel events.
/**
 * This class allows for a connection between wheel scrolling events
 * in the TrackButtons and scrolling in the CompositionView.
 *
 * Replaces QDeferScrollView
 *
 * \author Chris J. Fryer
 */
class DeferScrollArea : public QScrollArea
{
    Q_OBJECT
public:
    DeferScrollArea(QWidget* parent=0);
    ~DeferScrollArea();

    void wheelEvent(QWheelEvent*);
public slots:
signals:
    void gotWheelEvent(QWheelEvent*);

protected:
};

}

#endif
