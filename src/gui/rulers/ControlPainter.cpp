/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2010 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "ControlPainter.h"

#include "base/BaseProperties.h"
#include "base/Event.h"
#include "base/Segment.h"
#include "base/Selection.h"
#include "base/SnapGrid.h"
#include "base/ViewElement.h"
//#include "commands/matrix/MatrixModifyCommand.h"
//#include "commands/matrix/MatrixInsertionCommand.h"
//#include "commands/notation/NormalizeRestsCommand.h"
#include "document/CommandHistory.h"
#include "ControlItem.h"
#include "ControlRuler.h"
#include "ControllerEventsRuler.h"
#include "ControlTool.h"
#include "ControlMouseEvent.h"
#include "misc/Debug.h"

#include <QCursor>

namespace Rosegarden
{

ControlPainter::ControlPainter(ControlRuler *parent) :
    ControlMover(parent, "ControlPainter")
{
    m_overCursor = Qt::OpenHandCursor;
    m_notOverCursor = Qt::CrossCursor;
    m_controlLineOrigin.first = -1;
    m_controlLineOrigin.second = -1;
//    createAction("select", SLOT(slotSelectSelected()));
//    createAction("draw", SLOT(slotDrawSelected()));
//    createAction("erase", SLOT(slotEraseSelected()));
//    createAction("resize", SLOT(slotResizeSelected()));
//
//    createMenu();
}

void
ControlPainter::handleLeftButtonPress(const ControlMouseEvent *e)
{
    if (e->itemList.size()) {
        ControllerEventsRuler *ruler = static_cast <ControllerEventsRuler*> (m_ruler);
        std::vector <ControlItem*>::const_iterator it = e->itemList.begin();
        ruler->clearSelectedItems();
        ruler->addToSelection(*it);
        ruler->eraseControllerEvent();

        m_ruler->setCursor(Qt::CrossCursor);
    }
    else {
        // Make new control event here
        // This tool should not be applied to a PropertyControlRuler but in case it is
        ControllerEventsRuler* ruler = dynamic_cast <ControllerEventsRuler*>(m_ruler);
        //if (ruler) ruler->insertControllerEvent(e->x,e->y);
        if (ruler) {
            ControlItem *item = ruler->addControlItem(e->x,e->y);
            ControlMouseEvent *newevent = new ControlMouseEvent(e);
            newevent->itemList.push_back(item);
            m_overItem = true;
            ControlMover::handleLeftButtonPress(newevent);

            // If shift was pressed, draw a line of controllers between the new
            // control event and the previous one
            if (e->modifiers & (Qt::ShiftModifier)) {
                std::cout << "shift was pressed...  now we can tell some new command/dialog to draw a line from ("
                          << m_controlLineOrigin.first << ", " << m_controlLineOrigin.second << ") to ("
                          << e->x << ", " << e->y << ")" << std::endl;

                // NOTES:  OK, this is getting sane start/end coordinates now.
                // Need to figure out how to translate them into a time
                // (rulerScale?) and feed this into some new dialog/command to
                // do the business of actually generating stuff.
                //
                // That could be a lot like the pitch bend thing, but the pitch
                // bend thing works rather differently.  With that, you have to
                // select at least one note event on the matrix grid somewhere,
                // whereas this is going to use ruler coordinates.  It may well
                // be worth making the pitch bend thing work off the same
                // principle for defining the span of time it works with, and
                // that implies some kind of popup what do you want to do menu,
                // or else the use of different modifiers (shift+click to draw a
                // line to here, ctrl+shift+click to draw a pitch bend sequence
                // to here, or something like that).  Let's leave that
                // possibility unresolved for now, and not try to consolidate
                // any behavior.  Let's just concentrate on the line thing, and
                // deciding what to do with the "command/dialog" from here.
                //
                // The "insert a command here" school of thought is that we
                // should just git'r done and draw a line at some arbitrary
                // resolution, which is based on a calculation of what is
                // possible, with the idea to reach up to 128th note resolution.
                // If you can only go 20 steps in 1000 units of time, there's no
                // use having more than 20 events in the line, but if you can go
                // 1000 unites in 1000 units of time, you can have one event per
                // tick (or whatever) and 256th or 512th note resolution (I
                // forget) which suggests you'd want to have some kind of way to
                // dial in what kind of stepping factor to use.
                //
                // That all tends to lead to the "run a dialog here and the
                // dialog inserts a command" although that's begging for some
                // speedy override to "do what I said last time stupid and don't
                // make me keep going through the damn dialog" option.  (Or the
                // command work might get done here, instead of in the dialog.
                // Either way, there'd be both a command and a dialog.)
                //
                // Since it's all comparatively easy to change around and any
                // code here could go into an eventual dialog, let's start by
                // just writing some code that guesstimates and attempts to do
                // something useful.
                //
                // So where does the possible number of steps come from. Most
                // controllers it's 0-127, but pitch bend is different.  That's
                // the most likely case to wind up with a need for some step
                // factor.
                //
                // Otherwise, while I've never done such calculations, figuring
                // out the slope of this thing ought not be too bad.  Surely
                // figuring out the "run" is a matter of subtracting start time
                // from end time.  Then the "rise" we have to figure based on
                // where we started (need a control-value-for-Y function to map
                // where that is to a value, and I'm sure there's one in here
                // somewhere) and where we went.  So we span 960 units and we
                // rose 100 steps, so we do an event every 9.6 units.  Hrm.
                // That isn't actually possible, is it?  I don't think timeT is
                // a float.  So round it I guess?  An event every 10 units, with
                // the gap between the last calculated unit and the final unit
                // at the endpoint being a smidge smaller than the rest.
                //
                // This could possibly work really well without any hand
                // holding, if we just go for as much as is reasonably possible.
                // That example I was chewing on would be 100 events in a
                // quarter note, wouldn't it?  Somewhere between 128th and 64th
                // resolution.  And then if the rise were double over the same
                // run, we'd get something a little less than 256th note
                // resolution, or if we have the same rise over double the run,
                // it's about 64th resolution, and if we...  Well, I get the
                // idea.  Going for "as much as is reasonably possible" as a
                // baseline is probably pretty sound, given that most
                // controllers have a maximum rise of either 127 or 256, I
                // forget which.
                //
                // Well, talk is cheap.  Tomorrow, let's try to write some code!
                //
                // (Us being us, ourselves, and we, principally.)
                //
                // (This comment is deprecated, and will be disappearing from
                // future revisions of this software.)
            }

            // Save these coordinates for next time
            m_controlLineOrigin.first = e->x;
            m_controlLineOrigin.second = e->y;
        }
    }
 
}

const QString ControlPainter::ToolName = "painter";
}

#include "ControlPainter.moc"
