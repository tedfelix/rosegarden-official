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

#ifndef RG_PROPERTYADJUSTER_H
#define RG_PROPERTYADJUSTER_H

#include "ControlTool.h"

#include <QString>


namespace Rosegarden
{


class ControlRuler;


/// Adjusts velocity within the velocity ruler.
class PropertyAdjuster : public ControlTool
{
    Q_OBJECT

    friend class ControlToolBox;

public:

    void handleLeftButtonPress(const ControlMouseEvent *) override;
    FollowMode handleMouseMove(const ControlMouseEvent *) override;
    void handleMouseRelease(const ControlMouseEvent *) override;

    void ready() override;
    void stow() override;

    static QString ToolName();

private:

    explicit PropertyAdjuster(ControlRuler *);

    void setCursor(const ControlMouseEvent *);

    void setBasicContextHelp();

    //float m_mouseStartY;
    float m_mouseLastY{0};
    bool m_canSelect;

};


}

#endif
