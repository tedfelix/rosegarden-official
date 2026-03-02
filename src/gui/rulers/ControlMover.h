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

#ifndef RG_CONTROLMOVER_H
#define RG_CONTROLMOVER_H

#include "ControlItem.h"  // ControlItemList
#include "ControlTool.h"

#include <QCursor>
#include <QPointF>
#include <QString>

#include <vector>


namespace Rosegarden
{


class ControlMouseEvent;
class ControlRuler;
class SnapGrid;
class RulerScale;


/// Move tool for ControlRuler.
class ControlMover : public ControlTool
{
    Q_OBJECT

public:

    ControlMover(ControlRuler *parent,
                 const QString &menuName = "ControlMover");

    // ControlTool overrides.
    void handleLeftButtonPress(const ControlMouseEvent *e) override;
    FollowMode handleMouseMove(const ControlMouseEvent *e) override;
    void handleMouseRelease(const ControlMouseEvent *e) override;

    // BaseTool overrides.
    void ready() override;
    void stow() override;

    static QString ToolName();

protected:

    // ??? This is only used by ControlSelector.  Move it there.
    ControlItemList m_addedItems;

    /// Hover cursor.
    QCursor m_overCursor{Qt::OpenHandCursor};
    /// Normal, not hovering cursor.
    QCursor m_notOverCursor{Qt::ArrowCursor};

private:

    SnapGrid *m_snapGrid;
    RulerScale *m_rulerScale;

    /// Set the cursor to m_overCursor or m_notOverCursor as appropriate.
    void setCursor(const ControlMouseEvent *e);

    // Starting point for each selected item.
    std::vector <QPointF> m_startPointList;
    // Mouse click point for start of drag.
    float m_mouseStartX{0};
    float m_mouseStartY{0};
    // Previous position used for drag hysteresis.
    float m_lastDScreenX{0};
    float m_lastDScreenY{0};

};


}

#endif
