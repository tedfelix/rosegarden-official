/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2020 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[Typematic]"

#include "Typematic.h"


namespace Rosegarden
{


Typematic::Typematic()
{
    timer.setSingleShot(true);

    connect(&timer, &QTimer::timeout,
            this, &Typematic::slotTimeout);
}

void Typematic::press(bool pressed)
{
    if (pressed) {
        emit click();
        timer.start(250);
    } else {
        timer.stop();
    }
}

void Typematic::slotTimeout()
{
    emit click();
    timer.start(50);
}


}
