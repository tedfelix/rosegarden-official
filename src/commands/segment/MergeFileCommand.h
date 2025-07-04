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

#pragma once

#include "document/Command.h"  // for NamedCommand
#include "base/Composition.h"  // for tempoT and SegmentVec
#include "base/NotationTypes.h"  // for TimeSignature
#include "base/TimeT.h"
#include "base/Track.h"

#include <QCoreApplication>  // for Q_DECLARE_TR_FUNCTIONS()

#include <vector>
#include <map>


namespace Rosegarden
{


class RosegardenDocument;

class MergeFileCommand : public NamedCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::MergeFileCommand)

public:
    MergeFileCommand(RosegardenDocument *srcDoc,
                     bool mergeAtEnd,
                     bool mergeTimesAndTempos);
    ~MergeFileCommand() override;

    void execute() override;
    void unexecute() override;

private:
    // *** ATTRIBUTES

    // This is only valid during execute().  It is then cleared.
    RosegardenDocument *m_sourceDocument;
    bool m_mergeAtEnd;
    bool m_mergeTimesAndTempos;

    // *** UNDO

    /// Tracks created by this command.
    std::vector<Track *> m_newTracks;

    typedef std::map<timeT, TimeSignature> TimeSignatureMap;
    /// Time signatures added by this command.
    TimeSignatureMap m_newTimeSignatures;
    /// Time signatures clobbered by this command.
    TimeSignatureMap m_clobberedTimeSignatures;

    typedef std::map<timeT, tempoT> TempoChangeMap;
    /// Tempo changes added by this command.
    TempoChangeMap m_newTempoChanges;
    /// Tempo changes clobbered by this command.
    TempoChangeMap m_clobberedTempoChanges;

    /// Whether this command expanded the Composition.
    bool m_compositionExpanded;
    timeT m_oldCompositionEnd;
    timeT m_newCompositionEnd;

    /// Segments added by this command.
    /**
     * Only valid after undo and used for redo.
     */
    Composition::SegmentVector m_newSegments;

    void redo();

    // *** STATUS

    /// Command has been undone.
    /**
     * ??? Seems like a concept that many commands might find useful.  Why
     *     isn't there a Command::m_undone?
     * ??? There are actually four states: notDone, done, undone, redone
     *     But only "undone" is important in most cases.
     */
    bool m_undone;
};


}
