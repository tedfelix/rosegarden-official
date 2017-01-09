/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_MATRIX_MOUSE_EVENT_H
#define RG_MATRIX_MOUSE_EVENT_H

#include <QString>
#include <QPoint>

#include "base/Event.h" // for timeT

namespace Rosegarden
{

class MatrixViewSegment;
class MatrixElement;

class MatrixMouseEvent
{
public:
    MatrixViewSegment *viewSegment;
    MatrixElement *element; // under event, if any

    timeT time; // un-snapped and un-cropped

    timeT snappedLeftTime;  // snapped and cropped to segment extents
    timeT snappedRightTime; // snapped and cropped to segment extents
    timeT snapUnit;

    int pitch;
    double sceneX;
    int sceneY;

    QPoint viewpos;

    Qt::KeyboardModifiers modifiers;
    Qt::MouseButtons buttons;

    MatrixMouseEvent() :
        viewSegment(0), element(0),
        time(0), pitch(0),
        sceneX(0), sceneY(0),
        modifiers(0), buttons(0) { }
};

}

#endif
