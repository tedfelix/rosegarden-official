/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "AddIndicationCommand.h"

#include "misc/Strings.h"
#include "base/Event.h"
#include "base/NotationTypes.h"
#include "base/Segment.h"
#include "base/SegmentNotationHelper.h"
#include "base/Selection.h"
#include "base/BaseProperties.h"
#include "document/BasicCommand.h"
#include "document/CommandRegistry.h"
#include "gui/editors/notation/NotationProperties.h"
#include <vector>
#include <QString>


namespace Rosegarden
{

static std::vector<std::string> getStandardIndications()
{
    std::vector<std::string> v;
    v.push_back(Indication::Slur);
    v.push_back(Indication::PhrasingSlur);
    v.push_back(Indication::Glissando);
    v.push_back(Indication::Crescendo);
    v.push_back(Indication::Decrescendo);
    v.push_back(Indication::QuindicesimaUp);
    v.push_back(Indication::OttavaUp);
    v.push_back(Indication::OttavaDown);
    v.push_back(Indication::QuindicesimaDown);
    v.push_back(Indication::TrillLine);
    v.push_back(Indication::FigParameterChord);
    v.push_back(Indication::Figuration);
    return v;
}

static const char *actionNames[] = {
    "slur",
    "phrasing_slur",
    "glissando",
    "crescendo",
    "decrescendo",
    "octave_2up",
    "octave_up",
    "octave_down",
    "octave_2down",
    "trill_line",
    "parameter_chord",
    "figuration"
};

void
AddIndicationCommand::registerCommand(CommandRegistry *r)
{
    std::vector<std::string> standardIndications = getStandardIndications();
    
    for (size_t i = 0; i < standardIndications.size(); ++i) {
        r->registerCommand
            (actionNames[i],
             new ArgumentAndSelectionCommandBuilder<AddIndicationCommand>());
    }
}

std::string
AddIndicationCommand::getArgument(QString actionName, CommandArgumentQuerier &)
{
    std::vector<std::string> standardIndications = getStandardIndications();
    
    for (size_t i = 0; i < standardIndications.size(); ++i) {
        if (actionName == actionNames[i]) return standardIndications[i];
    }
    throw CommandCancelled();
}

AddIndicationCommand::AddIndicationCommand(std::string indicationType,
                                           EventSelection &selection) :
    BasicCommand(getGlobalName(indicationType),
                 selection.getSegment(),
                 std::min(selection.getStartTime(), selection.getNotationStartTime()),
                 std::max(selection.getEndTime(), selection.getNotationEndTime())),
    m_indicationType(indicationType),
    m_indicationStart(selection.getNotationStartTime()),
    m_indicationDuration(selection.getTotalNotationDuration()),
    m_lastInsertedEvent(0)
{
    if (!canExecute()) {
        throw CommandFailed
            //!!! need to use text from trunk/src/gui/editors/notation/NotationView.cpp (but this requires an informal human-readable version of the indication name)
            (qstrtostr(tr("Can't add identical overlapping indications")));
    }
}

AddIndicationCommand::~AddIndicationCommand()
{
    // empty
}

EventSelection *
AddIndicationCommand::getSubsequentSelection()
{
    EventSelection *selection = new EventSelection(getSegment());
    //!!! EXPERIMENTAL: Let's see if any users notice and complain about this.
    // The behavior up to now has been to leave the indication that was just
    // created the only thing selected in the segment after adding it.  The nice
    // thing about that is that it shows you in blue what you just did, and
    // primes you to do something else with the indication.  However, I'm facing
    // several hundred groups of notes I have to add slurs to, and using the
    // keyboard for this is the only sane way to go.  With the old behavior, I
    // have to remember to hit escape in between groups to clear the selection
    // away from the slur I created on the last iteration.  If I don't, the next
    // slur will begin in the same place the last one did.  Having to hit escape
    // in between iterations is awkward, and not sitting well with my fingers at
    // all.  I keep messing it up, and it made me crazy enough to hunt this down
    // and make it stop.  So let's turn the following line off, and see if there
    // are any negative repercussions:
//    selection->addEvent(getLastInsertedEvent());
    return selection;
}

bool
AddIndicationCommand::canExecute()
{
    Segment &s(getSegment());

    for (Segment::iterator i = s.begin(); s.isBeforeEndMarker(i); ++i) {

        if ((*i)->getNotationAbsoluteTime() >=
            m_indicationStart + m_indicationDuration) {
            return true;
        }

        if ((*i)->isa(Indication::EventType)) {

            try {
                Indication indication(**i);

                if ((*i)->getNotationAbsoluteTime() +
                    indication.getIndicationDuration() <= m_indicationStart)
                    continue;

                std::string type = indication.getIndicationType();

                if (type == m_indicationType) {
                    // for all indications (including slur), we reject an
                    // exact overlap
                    if ((*i)->getAbsoluteTime() == m_indicationStart &&
                        indication.getIndicationDuration() == m_indicationDuration) {
                        return false;
                    }
                } else if (m_indicationType == Indication::Slur) {
                    continue;
                }

                // for non-slur indications we reject a partial
                // overlap such as this one, if it's an overlap with
                // an indication of the same "sort"

                if (m_indicationType == Indication::Crescendo ||
                    m_indicationType == Indication::Decrescendo) {
                    if (type == Indication::Crescendo ||
                        type == Indication::Decrescendo) return false;
                }

                if (m_indicationType == Indication::QuindicesimaUp ||
                    m_indicationType == Indication::OttavaUp ||
                    m_indicationType == Indication::OttavaDown ||
                    m_indicationType == Indication::QuindicesimaDown) {
                    if (indication.isOttavaType()) return false;
                }
            } catch (...) {}
        }
    }

    return true;
}

void
AddIndicationCommand::modifySegment()
{
    SegmentNotationHelper helper(getSegment());
    Segment::iterator i, j;
    int actualSubordering = Indication::EventSubOrdering;

    helper.segment().getTimeSlice(getStartTime(), i, j);
    for (Segment::iterator k = i; k != j; ++k) {
        if ((*k)->has(BaseProperties::IS_GRACE_NOTE)) {
            // If a grace note is inserted before an indication, the
            // subordering is minor than Indication::EventSubOrdering,
            // therefore we have to decrement the subordering to insert
            // the new indication before the grace note.
            if ((*k)->getSubOrdering() <= actualSubordering) {
                actualSubordering = (*k)->getSubOrdering() - 1;
            }
        }
    }

    Indication indication(m_indicationType, m_indicationDuration);
    Event *e;

    e = new Event(Indication::EventType, m_indicationStart, m_indicationDuration,
                  actualSubordering);
    e->set<String>(Indication::IndicationTypePropertyName, m_indicationType);
    e->set<Int>("indicationduration", m_indicationDuration);
    helper.segment().insert(e);
    m_lastInsertedEvent = e;

    if (indication.isOttavaType()) {
        for (Segment::iterator i = getSegment().findTime(getStartTime());
             i != getSegment().findTime(getStartTime() + m_indicationDuration);
             ++i) {
            if ((*i)->isa(Note::EventType)) {
                (*i)->setMaybe<Int>(NotationProperties::OTTAVA_SHIFT,
                                    indication.getOttavaShift());
            }
        }
    }
}

QString
AddIndicationCommand::getGlobalName(std::string indicationType)
{
    if (indicationType == Indication::Slur) {
        return tr("Add S&lur");
    } else if (indicationType == Indication::PhrasingSlur) {
        return tr("Add &Phrasing Slur");
    } else if (indicationType == Indication::QuindicesimaUp) {
        return tr("Add Double-Octave Up");
    } else if (indicationType == Indication::OttavaUp) {
        return tr("Add Octave &Up");
    } else if (indicationType == Indication::OttavaDown) {
        return tr("Add Octave &Down");
    } else if (indicationType == Indication::QuindicesimaDown) {
        return tr("Add Double Octave Down");

        // We used to generate these ones from the internal names plus
        // caps, but that makes them untranslateable:
    } else if (indicationType == Indication::Crescendo) {
        return tr("Add &Crescendo");
    } else if (indicationType == Indication::Decrescendo) {
        return tr("Add &Decrescendo");
    } else if (indicationType == Indication::Glissando) {
        return tr("Add &Glissando");
    } else if (indicationType == Indication::TrillLine) {
        return tr("Add Tri&ll With Line");
    } else if (indicationType == Indication::FigParameterChord) {
        return tr("Add Parameter Chord");
    } else if (indicationType == Indication::Figuration) {
        return tr("Add Figuration");
    }

    QString n = tr("Add &%1%2").arg((char)toupper(indicationType[0])).arg(strtoqstr(indicationType.substr(1)));
    return n;
}

}
