
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2022 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_SETNOTETYPECOMMAND_H
#define RG_SETNOTETYPECOMMAND_H

#include "base/NotationTypes.h"
#include "document/BasicCommand.h"

#include <QCoreApplication>


namespace Rosegarden
{


class EventSelection;


class SetNoteTypeCommand : public BasicCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::SetNoteTypeCommand)

public:
    SetNoteTypeCommand(EventSelection &selection,
                       Note::Type type,
                       bool notationOnly) :
        BasicCommand(tr("&Set Note Type"), selection, true),
        m_selection(&selection),
        m_type(type),
        m_notationOnly(notationOnly)
    { }

protected:
    void modifySegment() override;

private:
    // only used on 1st execute (cf bruteForceRedo)
    EventSelection *m_selection;

    Note::Type m_type;
    bool m_notationOnly;
};



}

#endif
