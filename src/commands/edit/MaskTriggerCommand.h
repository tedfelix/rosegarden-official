/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2023 the Rosegarden development team.

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

#include "document/BasicCommand.h"

#include <QCoreApplication>
#include <QString>


namespace Rosegarden
{


class EventSelection;


/// Mask/Unmask Ornament
/**
 * Notation > Note > Ornaments >
 *   - Skip This Part of Ornament (Mask Tied Note)
 *   - Don't Skip This Part of Ornament (Unmask Tied Note)
 */
class MaskTriggerCommand : public BasicCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::MaskTriggerCommand)

public:
    MaskTriggerCommand(EventSelection &selection, bool sounding) :
        BasicCommand(getGlobalName(sounding),
                     selection,
                     true),  // bruteForceRedo
        m_selection(&selection),
        m_sounding(sounding)
    { }

protected:
    void modifySegment() override;

private:
    static QString getGlobalName(bool sounding);

    // only used on 1st execute (cf bruteForceRedo)
    EventSelection *m_selection;
    bool m_sounding;
};


}

#endif /* ifndef RG_MASKTRIGGERCOMMAND_H */
