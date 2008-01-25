
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2008
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>

    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_ADJUSTMENUINTERPRETCOMMAND_H_
#define _RG_ADJUSTMENUINTERPRETCOMMAND_H_

#include "document/BasicSelectionCommand.h"
#include <map>
#include <string>
#include <qstring.h>
#include "base/Event.h"
#include <klocale.h>




namespace Rosegarden
{

class Quantizer;
class Indication;
class EventSelection;
class Event;


class InterpretCommand : public BasicSelectionCommand
{
public:
    // bit masks: pass an OR of these to the constructor
    static const int NoInterpretation;
    static const int GuessDirections;    // allegro, rit, pause &c: kinda bogus
    static const int ApplyTextDynamics;  // mp, ff
    static const int ApplyHairpins;      // self-evident
    static const int StressBeats;        // stress bar/beat boundaries
    static const int Articulate;         // slurs, marks, legato etc
    static const int AllInterpretations; // all of the above

    InterpretCommand(EventSelection &selection,
                                   const Quantizer *quantizer,
                                   int interpretations) :
        BasicSelectionCommand(getGlobalName(), selection, true),
        m_selection(&selection),
        m_quantizer(quantizer),
        m_interpretations(interpretations) { }

    virtual ~InterpretCommand();

    static QString getGlobalName() { return i18n("&Interpret..."); }
    
protected:
    virtual void modifySegment();

private:
    EventSelection *m_selection;// only used on 1st execute (cf bruteForceRedo)
    const Quantizer *m_quantizer;
    int m_interpretations;

    typedef std::map<timeT,
                     Indication *> IndicationMap;
    IndicationMap m_indications;

    void guessDirections();
    void applyTextDynamics();
    void applyHairpins();
    void stressBeats();
    void articulate(); // must be applied last

    // test if the event is within an indication of the given type, return
    // an iterator pointing to that indication if so
    IndicationMap::iterator findEnclosingIndication(Event *,
                                                    std::string type);
    int getVelocityForDynamic(std::string dynamic);
};


}

#endif
