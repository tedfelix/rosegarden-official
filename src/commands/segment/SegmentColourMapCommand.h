
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_SEGMENTCOLOURMAPCOMMAND_H
#define RG_SEGMENTCOLOURMAPCOMMAND_H

#include "base/ColourMap.h"
#include "base/Segment.h"
#include "document/Command.h"
#include <QString>
#include <QCoreApplication>




namespace Rosegarden
{

class RosegardenDocument;


class SegmentColourMapCommand : public NamedCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::SegmentColourMapCommand)

public:
    SegmentColourMapCommand(      RosegardenDocument*      doc,
                            const ColourMap& map);
    virtual ~SegmentColourMapCommand();

    static QString getGlobalName()
        { return tr("Change Segment Color Map..."); }

    virtual void execute();
    virtual void unexecute();
protected:
    RosegardenDocument *                m_doc;
    ColourMap             m_oldMap;
    ColourMap             m_newMap;
};


// Trigger Segment commands.  These are the commands that create
// and manage the triggered segments themselves.  See editcommands.h
// for SetTriggerCommand and ClearTriggersCommand which manipulate
// the events that do the triggering.


}

#endif
