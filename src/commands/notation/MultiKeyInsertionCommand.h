
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

#ifndef RG_MULTIKEYINSERTIONCOMMAND_H
#define RG_MULTIKEYINSERTIONCOMMAND_H

#include "base/NotationTypes.h"
#include "base/TimeT.h"
#include "misc/Strings.h"
#include "document/Command.h"
#include "document/RosegardenDocument.h"

#include <QCoreApplication>
#include <QString>


namespace Rosegarden
{


class RosegardenDocument;


class MultiKeyInsertionCommand : public MacroCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::MultiKeyInsertionCommand)

public:

    MultiKeyInsertionCommand(RosegardenDocument *doc,
                             timeT time,
                             const Key& key,
                             bool shouldConvert,
                             bool shouldTranspose,
                             bool shouldTransposeKey,
			     bool shouldIgnorePercussion);
    ~MultiKeyInsertionCommand() override;

    static QString getGlobalName(const Key *key = nullptr) {
        if (key) {
            return tr("Change all to &Key %1...").arg(strtoqstr(key->getName()));
        } else {
            return tr("Add &Key Change...");
        }
    }
};


}

#endif
