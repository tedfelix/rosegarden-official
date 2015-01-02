
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_KEYINSERTIONCOMMAND_H
#define RG_KEYINSERTIONCOMMAND_H

#include "base/NotationTypes.h"
#include "document/BasicCommand.h"
#include <QString>
#include "base/Event.h"
#include <QCoreApplication>
#include "misc/Strings.h"


class Add;


namespace Rosegarden
{

class Segment;
class Event;

//!!! Note, the shouldIgnorePercussion parameter probably shouldn't have been
// added to the individual KeyInsertionCommand in the first place, but I haven't
// made up my mind yet for sure, and I already changed all the calls to this
// constructor, so I'm leaving this in until after the new code is field
// tested, and I can determine it really never will be wanted (DMM)
/*
 * Inserts a key change into a single segment, taking segment transpose into
 * account (fixes #1520716) if desired.
 */

class KeyInsertionCommand : public BasicCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::KeyInsertionCommand)

public:
    KeyInsertionCommand(Segment &segment,
                        timeT time,
                        Key key,
                        bool shouldConvert,
                        bool shouldTranspose,
                        bool shouldTransposeKey,
			bool shouldIgnorePercussion);
    virtual ~KeyInsertionCommand();

    static QString getGlobalName(Key *key = 0) {
        if (key) {
            return tr("Change to &Key %1...").arg(strtoqstr(key->getName()));
        } else {
            return tr("Add &Key Change...");
        }
    }

    virtual EventSelection *getSubsequentSelection();
    Event *getLastInsertedEvent() { return m_lastInsertedEvent; }

protected:
    virtual void modifySegment();

    Key m_key;
    Event *m_lastInsertedEvent;
    bool m_convert;
    bool m_transpose;
    bool m_transposeKey;
    bool m_ignorePercussion;
};

/*
 * Inserts a key change into multiple segments at the same time, taking
 * individual segment transpose into account (fixes #1520716) if desired.
 */

}

#endif
