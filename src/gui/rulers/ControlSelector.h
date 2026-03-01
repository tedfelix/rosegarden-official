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

#ifndef RG_CONTROLSELECTOR_H
#define RG_CONTROLSELECTOR_H

#include "ControlMover.h"

#include <QString>


namespace Rosegarden
{


class ControlMouseEvent;
class ControlRuler;


/// Selection tool for ControlRuler.
/**
 * Handles click-drag rubber band selection of events in a ControlRuler.
 */
class ControlSelector : public ControlMover
{
    Q_OBJECT

public:

    ControlSelector(ControlRuler *parent);

    // ControlTool overrides
    void handleLeftButtonPress(const ControlMouseEvent *e) override;
    FollowMode handleMouseMove(const ControlMouseEvent *e) override;
    void handleMouseRelease(const ControlMouseEvent *e) override;

    static QString ToolName()  { return "selector"; }

};


}

#endif
