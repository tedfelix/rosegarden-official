/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

    This file based on code from KMix
    Copyright (C) 2000 Stefan Schimanski <1Stein@gmx.de>.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "LedButton.h"

#include <QMouseEvent>

namespace Rosegarden
{


LedButton::LedButton(const QColor &color, QWidget *parent) :
    Led(color, parent)
{
}

void LedButton::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        toggle();
        emit stateChanged(state());
    }
}


}
