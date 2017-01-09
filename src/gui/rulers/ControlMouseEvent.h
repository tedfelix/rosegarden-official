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

#ifndef RG_CONTROL_MOUSE_EVENT_H
#define RG_CONTROL_MOUSE_EVENT_H

#include <QPoint>

#include "base/Event.h" // for timeT

namespace Rosegarden
{

class ControlItem;

class ControlMouseEvent
{
public:
    std::vector<ControlItem*> itemList; // list of items under the cursor, if any

//    timeT time; // un-snapped and un-cropped
//    float value;
    float x;
    float y;

    Qt::KeyboardModifiers modifiers;
    Qt::MouseButtons buttons;

    ControlMouseEvent() :
        itemList(),
        x(0), y(0), 
        modifiers(0), buttons(0) { }
    
    ControlMouseEvent(const ControlMouseEvent *e) :
        itemList(e->itemList),
        x(e->x), y(e->y),
        modifiers(e->modifiers), buttons(e->buttons) { }
};

}

#endif
