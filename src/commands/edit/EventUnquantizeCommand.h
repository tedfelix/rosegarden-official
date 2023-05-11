
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2023 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_EVENTUNQUANTIZECOMMAND_H
#define RG_EVENTUNQUANTIZECOMMAND_H

#include "document/BasicCommand.h"
#include <QString>
#include "base/Event.h"

#include <QCoreApplication>


namespace Rosegarden
{


class Segment;
class Quantizer;
class EventSelection;


class EventUnquantizeCommand : public BasicCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::EventUnquantizeCommand)
public:
    EventUnquantizeCommand(Segment &segment,
                           timeT startTime,
                           timeT endTime,
                           std::shared_ptr<Quantizer> quantizer);
    
    EventUnquantizeCommand(EventSelection &selection,
                           std::shared_ptr<Quantizer> quantizer);

    ~EventUnquantizeCommand() override;
    
    static QString getGlobalName();
    
protected:
    void modifySegment() override;

private:
    std::shared_ptr<Quantizer> m_quantizer;
    EventSelection *m_selection;
};



}

#endif
