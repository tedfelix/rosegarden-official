// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2004
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "ScriptAPI.h"

#include "Composition.h"
#include "Segment.h"
#include "Event.h"
#include "Sets.h"

#include <map>

namespace Rosegarden
{

class ScriptRep
{
public:
    GlobalChord *chord;
    // blah
};

class ScriptContainer :
    public std::map<ScriptInterface::ScriptId, ScriptRep *> { };

ScriptInterface::ScriptInterface(Rosegarden::Composition *composition) :
    m_composition(composition),
    m_scripts(new ScriptContainer())
{
}

ScriptInterface::~ScriptInterface()
{
}


}

