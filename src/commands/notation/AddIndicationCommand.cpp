/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "AddIndicationCommand.h"

#include <klocale.h>
#include "misc/Strings.h"
#include "base/Event.h"
#include "base/NotationTypes.h"
#include "base/Segment.h"
#include "base/SegmentNotationHelper.h"
#include "base/Selection.h"
#include "document/BasicCommand.h"
#include "document/CommandRegistry.h"
#include "gui/editors/notation/NotationProperties.h"
#include <QString>


namespace Rosegarden
{

static std::string standardIndications[] = {
    Indication::Slur,
    Indication::PhrasingSlur,
    Indication::Glissando,
    Indication::Crescendo,
    Indication::Decrescendo,
    Indication::QuindicesimaUp,
    Indication::OttavaUp,
    Indication::OttavaDown,
    Indication::QuindicesimaDown
};
static const char *shortcuts[] = {
    ")",
    "Ctrl+)",
    "",
    "<",
    ">",
    "",
    "",
    "",
    ""
};
static const char *icons[] = {
    "group-slur",
    "",
    "group-glissando",
    "group-crescendo",
    "group-decrescendo",
    "",
    "group-ottava",
    "",
    ""
};
static const char *actionNames[] = {
    "slur",
    "phrasing_slur",
    "glissando",
    "crescendo",
    "decrescendo",
    "octave_2up",
    "octave_up",
    "octave_down",
    "octave_2down"
};

void
AddIndicationCommand::registerCommand(CommandRegistry *r)
{
    for (int i = 0;
         i < sizeof(standardIndications)/sizeof(standardIndications[0]);
         ++i) {
        r->registerCommand
            (getGlobalName(standardIndications[i]), icons[i],
             shortcuts[i], actionNames[i],
             new ArgumentAndSelectionCommandBuilder<AddIndicationCommand>());
    }
}

std::string
AddIndicationCommand::getArgument(QString actionName, CommandArgumentQuerier &)
{
    for (int i = 0;
         i < sizeof(standardIndications)/sizeof(standardIndications[0]);
         ++i) {
        if (actionName == actionNames[i]) return standardIndications[i];
    }
    throw CommandCancelled();
}

AddIndicationCommand::AddIndicationCommand(std::string indicationType,
                                           EventSelection &selection) :
    BasicCommand(getGlobalName(indicationType),
                 selection.getSegment(),
                 selection.getStartTime(),
                 selection.getEndTime()),
    m_indicationType(indicationType),
    m_indicationDuration(selection.getEndTime() - selection.getStartTime()),
    m_lastInsertedEvent(0)
{
    if (!canExecute()) {
        throw CommandFailed
            //!!! need to use text from trunk/src/gui/editors/notation/NotationView.cpp (but this requires an informal human-readable version of the indication name)
            (qstrtostr(i18n("Can't add identical overlapping indications")));
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
    selection->addEvent(getLastInsertedEvent());
    return selection;
}

bool
AddIndicationCommand::canExecute()
{
    Segment &s(getSegment());

    for (Segment::iterator i = s.begin(); s.isBeforeEndMarker(i); ++i) {

        if ((*i)->getAbsoluteTime() >= getStartTime() + m_indicationDuration) {
            return true;
        }

        if ((*i)->isa(Indication::EventType)) {

            try {
                Indication indication(**i);

                if ((*i)->getAbsoluteTime() + indication.getIndicationDuration() <=
                        getStartTime())
                    continue;

                std::string type = indication.getIndicationType();

                if (type == m_indicationType) {
                    // for all indications (including slur), we reject an
                    // exact overlap
                    if ((*i)->getAbsoluteTime() == getStartTime() &&
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
                            type == Indication::Decrescendo)
                        return false;
                }

                if (m_indicationType == Indication::QuindicesimaUp ||
                        m_indicationType == Indication::OttavaUp ||
                        m_indicationType == Indication::OttavaDown ||
                        m_indicationType == Indication::QuindicesimaDown) {
                    if (indication.isOttavaType())
                        return false;
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

    Indication indication(m_indicationType, m_indicationDuration);
    Event *e = indication.getAsEvent(getStartTime());
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
        return i18n("Add S&lur");
    } else if (indicationType == Indication::PhrasingSlur) {
        return i18n("Add &Phrasing Slur");
    } else if (indicationType == Indication::QuindicesimaUp) {
        return i18n("Add Double-Octave Up");
    } else if (indicationType == Indication::OttavaUp) {
        return i18n("Add Octave &Up");
    } else if (indicationType == Indication::OttavaDown) {
        return i18n("Add Octave &Down");
    } else if (indicationType == Indication::QuindicesimaDown) {
        return i18n("Add Double Octave Down");

        // We used to generate these ones from the internal names plus
        // caps, but that makes them untranslateable:
    } else if (indicationType == Indication::Crescendo) {
        return i18n("Add &Crescendo");
    } else if (indicationType == Indication::Decrescendo) {
        return i18n("Add &Decrescendo");
    } else if (indicationType == Indication::Glissando) {
        return i18n("Add &Glissando");
    }

    QString n = i18n("Add &%1%2", (char)toupper(indicationType[0]), strtoqstr(indicationType.substr(1)));
    return n;
}

}
