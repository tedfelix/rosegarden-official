
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

#ifndef RG_INTERPRETCOMMAND_H
#define RG_INTERPRETCOMMAND_H

#include "document/BasicSelectionCommand.h"
#include <map>
#include <string>
#include <QString>
#include "base/Event.h"
#include <QCoreApplication>




namespace Rosegarden
{

class Quantizer;
class Indication;
class EventSelection;
class Event;


class InterpretCommand : public BasicSelectionCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::InterpretCommand)

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

    static QString getGlobalName() { return tr("&Interpret..."); }
    
protected:
    void modifySegment() override;

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
