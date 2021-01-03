/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.

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

#ifndef RG_LEDBUTTON_H
#define RG_LEDBUTTON_H

#include "Led.h"

class QWidget;
class QMouseEvent;
class QColor;

namespace Rosegarden
{


/// Adds button behavior to Led.
class LedButton : public Led
{
    Q_OBJECT

public: 
    LedButton(const QColor &color, QWidget *parent = nullptr);

signals:
    void stateChanged(bool newState);

protected:    
    void mousePressEvent(QMouseEvent *e) override;
};


}

#endif
