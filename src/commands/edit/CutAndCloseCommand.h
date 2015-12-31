
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

#ifndef RG_CUTANDCLOSECOMMAND_H
#define RG_CUTANDCLOSECOMMAND_H

#include "document/Command.h"
#include <QString>
#include "base/Event.h"
#include <QCoreApplication>




namespace Rosegarden
{

class Segment;
class EventSelection;
class Clipboard;


/// Cut a selection and close the gap

class CutAndCloseCommand : public MacroCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::CutAndCloseCommand)

public:
    CutAndCloseCommand(EventSelection &selection,
                       Clipboard *clipboard);

    static QString getGlobalName() { return tr("C&ut and Close"); }

protected:
    class CloseCommand : public NamedCommand
    {
    public:
        CloseCommand(Segment *segment,
                     timeT fromTime,
                     timeT toTime) :
            NamedCommand("Close"),
            m_segment(segment),
            m_gapEnd(fromTime),
            m_gapStart(toTime) { }

        virtual void execute();
        virtual void unexecute();

    private:
        Segment *m_segment;
        timeT m_gapEnd;
        timeT m_gapStart;
        int m_staticEvents;
    };
};    



}

#endif
