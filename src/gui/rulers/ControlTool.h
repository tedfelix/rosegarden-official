/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef CONTROLTOOL_H_
#define CONTROLTOOL_H_

#include <QObject>
#include "gui/general/BaseTool.h"

class QString;

namespace Rosegarden {

class ControlRuler;
class ControlMouseEvent;

class ControlTool : public BaseTool
{
    Q_OBJECT

public:
    enum FollowMode {
        NoFollow = 0x0,
        FollowHorizontal = 0x1,
        FollowVertical = 0x2
    };

    ControlTool(QString rcFileName, QString menuName, ControlRuler *ruler);
    virtual ~ControlTool() {};
//    virtual int operator()(double x, int val) = 0;
    virtual void handleLeftButtonPress(const ControlMouseEvent *) {}
    virtual void handleMidButtonPress(const ControlMouseEvent *) {}
    virtual void handleRightButtonPress(const ControlMouseEvent *) {}
    virtual void handleMouseRelease(const ControlMouseEvent *) {}
    virtual void handleMouseDoubleClick(const ControlMouseEvent *) {}
    virtual FollowMode handleMouseMove(const ControlMouseEvent *) { return NoFollow; }

protected:
    virtual void createMenu();

    ControlRuler *m_ruler;
    bool m_overItem;
};

}

#endif /*CONTROLTOOL_H_*/
