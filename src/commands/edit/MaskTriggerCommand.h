/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2012 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_MASKTRIGGERCOMMAND_H
#define RG_MASKTRIGGERCOMMAND_H

#include "document/BasicSelectionCommand.h"

namespace Rosegarden
{
class EventSelection;


/** Add or subtract a constant from all event velocities.
    Use SelectionPropertyCommand if you want to do something more
    creative. */
class MaskTriggerCommand : public BasicSelectionCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::MaskTriggerCommand)

public:
    MaskTriggerCommand(EventSelection &selection, bool sounding) :
        BasicSelectionCommand(getGlobalName(sounding), selection, true),
        m_selection(&selection), m_sounding(sounding) { }

	static QString getGlobalName(bool sounding);

protected:
    void modifySegment() override;

private:
    EventSelection *m_selection;// only used on 1st execute (cf bruteForceRedo)
    bool m_sounding;
};


}

#endif /* ifndef RG_MASKTRIGGERCOMMAND_H */
