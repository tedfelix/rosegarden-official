
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

#ifndef RG_PASTESEGMENTSCOMMAND_H
#define RG_PASTESEGMENTSCOMMAND_H

#include "base/Track.h"
#include "document/Command.h"
#include <QString>
#include <QCoreApplication>
#include <vector>
#include "base/Event.h"




namespace Rosegarden
{

class Segment;
class Composition;
class Clipboard;


/// Paste one or more segments from the clipboard into the composition

class PasteSegmentsCommand : public NamedCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::PasteSegmentsCommand)

public:
    PasteSegmentsCommand(Composition *composition,
                         Clipboard *clipboard,
                         timeT pasteTime,
                         TrackId baseTrack,
                         bool useExactTracks);

    virtual ~PasteSegmentsCommand();

    static QString getGlobalName() { return tr("&Paste"); }

    virtual void execute();
    virtual void unexecute();

protected:
    Composition *m_composition;
    Clipboard *m_clipboard;
    timeT m_pasteTime;
    TrackId m_baseTrack;
    bool m_exactTracks;
    std::vector<Segment *> m_addedSegments;
    bool m_detached;
    timeT m_oldEndTime;
};



}

#endif
