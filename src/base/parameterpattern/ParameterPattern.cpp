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

#include "ParameterPattern.h"

#include "AlternatingParameterPattern.h"
#include "FlatParameterPattern.h"
#include "HalfSinePattern.h"
#include "IncreaseParameterPattern.h"
#include "LinearParameterPattern.h"
#include "QuarterSinePattern.h"
#include "RelativeRamp.h"
#include "RingingParameterPattern.h"

#include "base/BaseProperties.h"
#include "document/CommandHistory.h"
#include "gui/dialogs/EventParameterDialog.h"
#include "gui/widgets/TmpStatusMsg.h"
#include <limits>

namespace Rosegarden
{


    /*** Nested class Result  ***/


// Get the selection we have.
EventSelection *
ParameterPattern::Result::getSelection(void)
{
    return m_situation->getEventSelection();
}

// Modify the segment we have.  This is the guts of
// SelectionPropertyCommand.
void
ParameterPattern::Result::modifySegment(void)
{
    typedef EventSelection::eventcontainer::iterator iterator;
    const EventSelection *selection = m_situation->getEventSelection();

    iterator begin =
        selection->getSegmentEvents().begin();
    iterator end =
        selection->getSegmentEvents().end();

    m_pattern->setEventProperties(begin, end, this);
}

/* ***** Static data ***** */

ParameterPattern *
ParameterPattern::FlatPattern = &FlatParameterPattern::single;

// Making the vector is tricky.  To get past std::vector's lack of
// nice static initialization, we put everything into an array and
// initialize the vector from that.

#define ENDOFARRAY(X) ((X) + sizeof(X)/sizeof(X[0]))
#define DEFINE_PPVEC_FROM_ARRAY(NAME, SOURCE)           \
    ParameterPattern::ParameterPatternVec               \
    ParameterPattern::NAME(SOURCE, ENDOFARRAY(SOURCE))

// Internal
ParameterPattern * velocityPatternsArray[] = {
    &FlatParameterPattern::single,
    &AlternatingParameterPattern::single,
    &LinearParameterPattern::crescendo,
    &LinearParameterPattern::diminuendo,
    &RingingParameterPattern::single,
    &IncreaseParameterPattern::decrease,
    &IncreaseParameterPattern::increase,
    &QuarterSinePattern::crescendo,
    &QuarterSinePattern::diminuendo,
    &HalfSinePattern::crescendo,
    &HalfSinePattern::diminuendo,
    &RelativeRamp::single,
};

// Internal
ParameterPattern ** endVelocityPatterns = ENDOFARRAY(velocityPatternsArray);

// A vector of all the patterns that are useful for setting velocity.
ParameterPattern::ParameterPatternVec
ParameterPattern::VelocityPatterns(velocityPatternsArray, endVelocityPatterns);

ParameterPattern * controllerPatternsArray[] = {
    &LinearParameterPattern::crescendo,
    &LinearParameterPattern::diminuendo,
    &IncreaseParameterPattern::decrease,
    &IncreaseParameterPattern::increase,
    &QuarterSinePattern::crescendo,
    &QuarterSinePattern::diminuendo,
    &HalfSinePattern::crescendo,
    &HalfSinePattern::diminuendo,
    &RelativeRamp::single,
};

DEFINE_PPVEC_FROM_ARRAY(ControllerPatterns, controllerPatternsArray);

/* ***** Helper functions ***** */
   
// Get the start and duration in timeT of the interval defined by
// begin and end.
ParameterPattern::StartAndDuration
ParameterPattern::getTimes (iterator begin, iterator end) 
{
    // Start with values such that anything we find will supersede
    // them.
    timeT endTime   = std::numeric_limits<int>::min();
    timeT startTime = std::numeric_limits<int>::max();

    for (iterator i = begin; i != end; ++i) {
        if ((*i)->getAbsoluteTime() < startTime) {
            startTime = (*i)->getAbsoluteTime();
        }

        if ((*i)->getAbsoluteTime() > endTime) {
            endTime = (*i)->getAbsoluteTime();
        }
    }

    return StartAndDuration(startTime, endTime - startTime);
}

/* ***** End-to-end functions ***** */

// Set some property, with a dialog.  Safe to call with nullptr
// selection.
// @param situation This call will take ownership of situation.
void
ParameterPattern::setProperties(QMainWindow *parent,
                                SelectionSituation   *situation,
                                const ParameterPatternVec *patterns)
{
    EventParameterDialog
        dialog(parent,
               EventParameterDialog::tr("Set Event Velocities"),
               situation,
               patterns);

    if (dialog.exec() == QDialog::Accepted) {
        TmpStatusMsg msg(EventParameterDialog::tr("Setting Velocities..."), 
                         parent);
        CommandHistory::getInstance()->addCommand
            (new SelectionPropertyCommand(dialog.getResult()));
    } else { delete situation; }
}

ParameterPattern::~ParameterPattern()
{

}
    
void
ParameterPattern::
setProperties(QMainWindow *parent,
              EventSelection *selection,
              const std::string eventType,
              const ParameterPatternVec *patterns,
              int normValue)
{
    if (!selection) { return; }

    // situation will ultimately be owned by SelectionPropertyCommand via
    // dialog result
    setProperties(parent,
                  new SelectionSituation(eventType, selection, normValue),
                  patterns);
}

// Set velocities, with a dialog
void
ParameterPattern::
setVelocities(QMainWindow *parent,
              EventSelection *selection,
              int normVelocity)
{
    setProperties(parent, selection, Note::EventType,
                  &ParameterPattern::VelocityPatterns, normVelocity);
}

// Set some property to targetValue, no dialog. 
void
ParameterPattern::
setPropertyFlat(EventSelection *selection,
                const std::string eventType,
                int targetValue)
{
    if (!selection) { return; }

    Result
        result(new SelectionSituation(eventType, selection),
               ParameterPattern::FlatPattern,
               targetValue);

    CommandHistory::getInstance()->addCommand
        (new SelectionPropertyCommand(result));
}

// Set velocity to targetVelocity, no dialog. 
void
ParameterPattern::
setVelocitiesFlat(EventSelection *selection, int targetVelocity)
{
    setPropertyFlat(selection, Note::EventType,
                    targetVelocity);
}

} // End namespace Rosegarden 
