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

#ifndef RG_NOTATION_MOUSE_EVENT_H
#define RG_NOTATION_MOUSE_EVENT_H

#include "base/NotationTypes.h"
#include "base/Event.h"
#include <QString>

namespace Rosegarden
{

class NotationStaff;
class NotationElement;

class NotationMouseEvent
{
public:
    NotationStaff *staff;   // under event, if any
    NotationElement *element; // under event, if any
    bool exact; // exact click on this element (wasn't just the nearest)
    Clef clef;
    Key key;

    timeT time;
    int height; // height on staff
    double sceneX;
    int sceneY;

    Qt::KeyboardModifiers modifiers;
    Qt::MouseButtons buttons;

    NotationMouseEvent() :
        staff(nullptr), element(nullptr), exact(false),
        time(0), height(0),
        sceneX(0), sceneY(0),
        modifiers(), buttons() { }
};

}

#endif
