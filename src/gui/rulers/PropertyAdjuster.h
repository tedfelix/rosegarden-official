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

#ifndef RG_PROPERTYADJUSTER_H
#define RG_PROPERTYADJUSTER_H

#include "ControlTool.h"
//#include <QString>
//#include "base/Event.h"


namespace Rosegarden
{

class Event;
class ControlRuler;

class PropertyAdjuster : public ControlTool
{
    Q_OBJECT

    friend class ControlToolBox;

public:
    void handleLeftButtonPress(const ControlMouseEvent *) override;
    FollowMode handleMouseMove(const ControlMouseEvent *) override;
    void handleMouseRelease(const ControlMouseEvent *) override;

    /**
     * Respond to an event being deleted -- it may be the one the tool
     * is remembering as the current event.
     */
//    virtual void handleEventRemoved(Event *event);

    void ready() override;
    void stow() override;

    static QString ToolName();

signals:
//    void hoveredOverNoteChanged(int evPitch, bool haveEvent, timeT evTime);

protected slots:
//    void slotMatrixScrolled(int x, int y); //!!! do we need this? probably not

protected:
    PropertyAdjuster(ControlRuler *);
    void setCursor(const ControlMouseEvent *);

    void setBasicContextHelp();

    float m_mouseStartY;
    float m_mouseLastY;
    bool m_canSelect;
};

}

#endif
