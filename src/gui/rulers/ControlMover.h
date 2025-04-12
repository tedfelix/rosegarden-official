/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_CONTROLMOVER_H
#define RG_CONTROLMOVER_H

#include "ControlTool.h"
#include "ControlItem.h"
#include <QCursor>

class QRectF;
class QPoint;

namespace Rosegarden
{

class Event;
class ControlRuler;
class SnapGrid;
class RulerScale;

class ControlMover : public ControlTool
{
    Q_OBJECT

    friend class ControlToolBox;

public:
    ControlMover(ControlRuler *parent,
                 const QString& menuName = "ControlMover");
    void handleLeftButtonPress(const ControlMouseEvent *) override;
    FollowMode handleMouseMove(const ControlMouseEvent *) override;
    void handleMouseRelease(const ControlMouseEvent *) override;

    void ready() override;
    void stow() override;

    static QString ToolName();

signals:

protected slots:

protected:
    void setCursor(const ControlMouseEvent *);
    QCursor m_overCursor;
    QCursor m_notOverCursor;

    float m_mouseStartX;
    float m_mouseStartY;
    float m_lastDScreenX;
    float m_lastDScreenY;
    QRectF *m_selectionRect;
    ControlItemList m_addedItems;
    std::vector <QPointF> m_startPointList;
    SnapGrid* m_snapGrid;
    RulerScale* m_rulerScale;
};

}

#endif
