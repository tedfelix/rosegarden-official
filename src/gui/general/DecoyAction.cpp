/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "DecoyAction.h"

#include <iostream>

namespace Rosegarden {

DecoyAction *
DecoyAction::m_instance = 0;

DecoyAction *
DecoyAction::getInstance()
{ 
    if (!m_instance) m_instance = new DecoyAction();
    std::cerr << "WARNING: Using decoy action" << std::endl;
    return m_instance;
}

DecoyAction::~DecoyAction()
{
    std::cerr << "ERROR: Deleting the global DecoyAction -- some class has looked up an action that did not exist, and deleted it -- a crash is highly likely now" << std::endl;
}

    DecoyAction::DecoyAction() : QAction("Decoy Action", 0) { }

}


    
