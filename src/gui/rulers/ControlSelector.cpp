/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_CONTROLSELECTOR_CPP_
#define _RG_CONTROLSELECTOR_CPP_

#include <QMouseEvent>
#include "ControlSelector.h"

namespace Rosegarden {

ControlSelector::ControlSelector(ControlRuler* parent)
    : QObject(parent),
      m_ruler(parent)
{
}

void ControlSelector::handleMouseButtonPress(QMouseEvent *e)
{
    QPoint p = m_ruler->inverseMapPoint(e->pos());

    getSelectionRectangle()->setX(p.x());
    getSelectionRectangle()->setY(p.y());
    getSelectionRectangle()->setSize(0,0);

    getSelectionRectangle()->show();
    m_ruler->canvas()->update();
}

void ControlSelector::handleMouseButtonRelease(QMouseEvent*)
{
    getSelectionRectangle()->hide();
    m_ruler->canvas()->update();
}

void ControlSelector::handleMouseMove(QMouseEvent *e, int, int)
{
    QPoint p = m_ruler->inverseMapPoint(e->pos());

    int w = int(p.x() - getSelectionRectangle()->x());
    int h = int(p.y() - getSelectionRectangle()->y());
    if (w > 0) ++w; else --w;
    if (h > 0) ++h; else --h;

    getSelectionRectangle()->setSize(w, h);

    m_ruler->canvas()->update();
}

}

#endif /*CONTROLSELECTOR_CPP_*/
