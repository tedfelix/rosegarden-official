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

#define RG_MODULE_STRING "[ControlEraser]"
#define RG_NO_DEBUG_PRINT

#include "ControlEraser.h"

#include "ControlTool.h"
#include "ControlItem.h"
#include "ControllerEventsRuler.h"
#include "ControlMouseEvent.h"

#include "misc/Debug.h"


namespace Rosegarden
{


ControlEraser::ControlEraser(ControlRuler *parent) :
    ControlTool("", "ControlEraser", parent)
{
}

void
ControlEraser::handleLeftButtonPress(const ControlMouseEvent *e)
{
    if (!e->itemList.size())
        return;

    ControllerEventsRuler *ruler = dynamic_cast<ControllerEventsRuler *>(m_ruler);
    if (!ruler)
        return;

    ControlItemVector::const_iterator it;

    // If any items under the cursor are selected, delete entire selection.
    for (it = e->itemList.begin(); it != e->itemList.end(); ++it) {
        if ((*it)->isSelected()) {
            // Erase the entire selection.
            ruler->eraseControllerEvent();
            break;
        }
    }

    // If none were selected...
    if (it == e->itemList.end()) {
        // Delete the first.
        it = e->itemList.begin();
        ruler->clearSelectedItems();
        ruler->addToSelection(*it);
        ruler->eraseControllerEvent();
    }
}

FollowMode
ControlEraser::handleMouseMove(const ControlMouseEvent *e)
{
    emit showContextHelp(tr("Click on a value to delete it"));
    if (e->buttons == Qt::NoButton) {
        // No button pressed, set cursor style
        setCursor(e);
    }

    return NO_FOLLOW;
}

void
ControlEraser::handleMouseRelease(const ControlMouseEvent *e)
{
    if (m_overItem) {
        // This is the end of a drag event, reset the cursor to the state that it started
        m_ruler->setCursor(Qt::PointingHandCursor);
    }

    // May have moved off the item during a drag so use setCursor to correct its state
    setCursor(e);
}

void ControlEraser::setCursor(const ControlMouseEvent *e)
{
    bool isOverItem = false;

    if (e->itemList.size()) isOverItem = true;

    if (!m_overItem) {
        if (isOverItem) {
            m_ruler->setCursor(Qt::PointingHandCursor);
            m_overItem = true;
        }
    } else {
        if (!isOverItem) {
            m_ruler->setCursor(Qt::ArrowCursor);
            m_overItem = false;
        }
    }
}

void ControlEraser::ready()
{
    m_ruler->setCursor(Qt::CrossCursor);
    m_overItem = false;
}


}
