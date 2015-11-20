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


#include "SetLyricsCommand.h"

#include "base/Event.h"
#include "misc/Strings.h"
#include "misc/Debug.h"
#include "base/Composition.h"
#include "base/NotationTypes.h"
#include "base/Segment.h"
#include "base/BaseProperties.h"
#include <QRegExp>
#include <QString>
#include <QStringList>


namespace Rosegarden
{

using namespace BaseProperties;

SetLyricsCommand::SetLyricsCommand(Segment *segment, int verse, QString newLyricData) :
        NamedCommand(getGlobalName()),
        m_segment(segment),
        m_verse(verse),
        m_newLyricData(newLyricData)
{
    // nothing
}

SetLyricsCommand::~SetLyricsCommand()
{
    for (std::vector<Event *>::iterator i = m_oldLyricEvents.begin();
            i != m_oldLyricEvents.end(); ++i) {
        delete *i;
    }
}

void
SetLyricsCommand::execute()
{
    // This and LyricEditDialog::unparse() are opposites that will
    // need to be kept in sync with any changes to one another.  (They
    // should really both be in a common lyric management class.)

    // first remove old lyric events

    Segment::iterator i = m_segment->begin();

    while (i != m_segment->end()) {

        Segment::iterator j = i;
        ++j;

        if ((*i)->isa(Text::EventType)) {
            std::string textType;
            if ((*i)->get<String>(Text::TextTypePropertyName, textType) &&
                textType == Text::Lyric) {
                long verse = 0;
                (*i)->get<Int>(Text::LyricVersePropertyName, verse);
                if (verse == m_verse) {
                    m_oldLyricEvents.push_back(new Event(**i));
                    m_segment->erase(i);
                }
            }
        }

        i = j;
    }

    // now parse the new string

    QStringList barStrings =
        m_newLyricData.split("/", QString::KeepEmptyParts); // empties ok

    Composition *comp = m_segment->getComposition();
    int barNo = comp->getBarNumber(m_segment->getStartTime());

    QStringList::Iterator bsi = barStrings.begin();
    while ( bsi != barStrings.end() ) {
        NOTATION_DEBUG << "Parsing lyrics for bar number " << barNo << ": \"" << *bsi << "\"" << endl;

        std::pair<timeT, timeT> barRange = comp->getBarRange(barNo++);
        QString syllables = *bsi;
        syllables.replace(QRegExp("\\[\\d+\\] "), " ");
        syllables.replace(QRegExp("\n"), " ");
        QStringList syllableList = syllables.split(" ", QString::SkipEmptyParts); // no empties

        i = m_segment->findTime(barRange.first);
        timeT laterThan = barRange.first - 1;

	++bsi; // update here in order to check whether we are in the last bar string
        bool isLastBSI = (bsi == barStrings.end());

        for (QStringList::Iterator ssi = syllableList.begin();
                ssi != syllableList.end(); ++ssi) {

	    // As a rule, syllables belong to a certain bar. However, from the
	    // last barString list syllables may flow to the following bars.
	    // As a result, one may copy&paste the full syllable list of a verse.
            while (m_segment->isBeforeEndMarker(i) &&
                    (isLastBSI || (*i)->getAbsoluteTime() < barRange.second) &&
                    (!(*i)->isa(Note::EventType) ||
                     (*i)->getNotationAbsoluteTime() <= laterThan ||
                     ((*i)->has(TIED_BACKWARD) &&
                      (*i)->get
                      <Bool>(TIED_BACKWARD)))) ++i;

            timeT time = m_segment->getEndMarkerTime();
            timeT notationTime = time;
            if (m_segment->isBeforeEndMarker(i)) {
                time = (*i)->getAbsoluteTime();
                notationTime = (*i)->getNotationAbsoluteTime();
            }

            QString syllable = *ssi;
            syllable.replace(QRegExp("~"), " ");
            syllable = syllable.simplified();
            if (syllable == "")
                continue;
            laterThan = notationTime + 1;
            if (syllable == ".")
                continue;

            NOTATION_DEBUG << "Syllable \"" << syllable << "\" at time " << time << endl;

            Text text(qstrtostr(syllable), Text::Lyric);
            Event *event = text.getAsEvent(time);
            event->set<Int>(Text::LyricVersePropertyName, m_verse);
            m_segment->insert(event);
        }
    }

    // The verse count changed
    m_segment->invalidateVerseCount();
}

void
SetLyricsCommand::unexecute()
{
    // Before we inserted the new lyric events (in execute()), we
    // removed all the existing ones.  That means we know any lyric
    // events found now must have been inserted by execute(), so we
    // can safely remove them before restoring the old ones.

    Segment::iterator i = m_segment->begin();

    while (i != m_segment->end()) {

        Segment::iterator j = i;
        ++j;

        if ((*i)->isa(Text::EventType)) {
            std::string textType;
            if ((*i)->get<String>(Text::TextTypePropertyName, textType) &&
                textType == Text::Lyric) {
                long verse = 0;
                (*i)->get<Int>(Text::LyricVersePropertyName, verse);
                if (verse == m_verse) {
                    m_segment->erase(i);
                }
            }
        }

        i = j;
    }

    // Now restore the old ones and clear out the vector.

    for (std::vector<Event *>::iterator i = m_oldLyricEvents.begin();
            i != m_oldLyricEvents.end(); ++i) {
        m_segment->insert(*i);
    }

    m_oldLyricEvents.clear();
}

}
