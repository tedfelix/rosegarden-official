/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_CONTROLERASER_H
#define RG_CONTROLERASER_H

#include "ControlTool.h"

#include <QString>


namespace Rosegarden
{


class ControlRuler;


class ControlEraser : public ControlTool
{
    Q_OBJECT

public:

    ControlEraser(ControlRuler *);

    void handleLeftButtonPress(const ControlMouseEvent *) override;
    FollowMode handleMouseMove(const ControlMouseEvent *) override;
    void handleMouseRelease(const ControlMouseEvent *) override;

    void ready() override;
    void stow() override  { }

    static QString ToolName()  { return "eraser"; }

private:

    void setCursor(const ControlMouseEvent *);

};

}

#endif
