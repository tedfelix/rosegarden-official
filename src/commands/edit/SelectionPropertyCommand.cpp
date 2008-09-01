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


#include "SelectionPropertyCommand.h"

#include "base/Event.h"
#include "base/PropertyName.h"
#include "base/Selection.h"
#include "document/BasicSelectionCommand.h"
#include <QString>


namespace Rosegarden
{

SelectionPropertyCommand::SelectionPropertyCommand(
    EventSelection *selection,
    const PropertyName &property,
    PropertyPattern pattern,
    int value1,
    int value2):
        BasicSelectionCommand(getGlobalName(), *selection, true),
        m_selection(selection),
        m_property(property),
        m_pattern(pattern),
        m_value1(value1),
        m_value2(value2)
{}

void
SelectionPropertyCommand::modifySegment()
{
    EventSelection::eventcontainer::iterator i =
        m_selection->getSegmentEvents().begin();

    int count = 0;

    timeT endTime = 0;
    timeT startTime = 0;
    bool haveStart = false, haveEnd = false;

    // Get start and end times
    //
    for (;i != m_selection->getSegmentEvents().end(); ++i) {
        if ((*i)->getAbsoluteTime() < startTime || !haveStart) {
            startTime = (*i)->getAbsoluteTime();
            haveStart = true;
        }

        if ((*i)->getAbsoluteTime() > endTime || !haveEnd) {
            endTime = (*i)->getAbsoluteTime();
            haveEnd = true;
        }
    }

    double step = double(m_value1 - m_value2) / double(endTime - startTime);
    double lowStep = double(m_value2) / double(endTime - startTime);

    for (i = m_selection->getSegmentEvents().begin();
            i != m_selection->getSegmentEvents().end(); ++i) {
        // flat
        if (m_pattern == FlatPattern)
            (*i)->set
            <Int>(m_property, m_value1);
        else if (m_pattern == AlternatingPattern) {
            if (count % 2 == 0)
                (*i)->set
                <Int>(m_property, m_value1);
            else
                (*i)->set
                <Int>(m_property, m_value2);

            // crescendo, decrescendo
            // (determined by step, above, which is in turn influenced by whether
            // value1 is greater than value2)
        } else if ((m_pattern == CrescendoPattern) ||
                   (m_pattern == DecrescendoPattern)) {
            (*i)->set
            <Int>(m_property,
                  m_value1 -
                  int(step *
                      ((*i)->getAbsoluteTime() - startTime)));
            // ringing
        } else if (m_pattern == RingingPattern) {
            if (count % 2 == 0)
                (*i)->set
                <Int>
                (m_property,
                 m_value1 - int(step *
                                ((*i)->getAbsoluteTime() - startTime)));
            else {
                int value = m_value2 - int(lowStep *
                                           ((*i)->getAbsoluteTime() - startTime));
                if (value < 0)
                    value = 0;

                (*i)->set
                <Int>(m_property, value);
            }
        }

        count++;
    }
}

}
