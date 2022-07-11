
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2022 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_INTERPRETCOMMAND_H
#define RG_INTERPRETCOMMAND_H

#include "document/BasicCommand.h"

#include <QCoreApplication>

#include <map>
#include <string>


namespace Rosegarden
{


class Quantizer;
class Indication;
class EventSelection;
class Event;


class InterpretCommand : public BasicCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::InterpretCommand)

public:
    // interpretation bit masks: pass an OR of these to the constructor
    static constexpr int NoInterpretation  = 0;
    static constexpr int GuessDirections   = 1 << 0;  // allegro, rit, pause &c: kinda bogus
    static constexpr int ApplyTextDynamics = 1 << 1;  // mp, ff
    static constexpr int ApplyHairpins     = 1 << 2;
    static constexpr int StressBeats       = 1 << 3;  // stress bar/beat boundaries
    static constexpr int Articulate        = 1 << 4;  // slurs, marks, legato etc
    static constexpr int AllInterpretations = (1 << 5) - 1;  // all of the above

    InterpretCommand(EventSelection &selection,
                     const Quantizer *quantizer,
                     int interpretations) :
        BasicCommand(tr("&Interpret..."), selection, true),
        m_selection(&selection),
        m_quantizer(quantizer),
        m_interpretations(interpretations)
    { }

    ~InterpretCommand() override;

protected:
    void modifySegment() override;

private:
    // only used on 1st execute (cf bruteForceRedo)
    EventSelection *m_selection;

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
                                                    const std::string& type);
    int getVelocityForDynamic(const std::string& dynamic);
};


}

#endif
