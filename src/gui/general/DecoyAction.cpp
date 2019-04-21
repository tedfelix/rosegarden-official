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

#define RG_MODULE_STRING "[DecoyAction]"

#include "DecoyAction.h"

#include "misc/Debug.h"

namespace Rosegarden {


DecoyAction *
DecoyAction::m_instance = nullptr;

DecoyAction *
DecoyAction::getInstance()
{ 
    if (!m_instance) m_instance = new DecoyAction();
    RG_WARNING << "getInstance(): WARNING: Using decoy action";
    return m_instance;
}

DecoyAction::~DecoyAction()
{
    RG_WARNING << "dtor: ERROR: Deleting the global DecoyAction -- some class has looked up an action that did not exist, and deleted it -- a crash is highly likely now";
}

DecoyAction::DecoyAction() : QAction("Decoy Action", nullptr) { }


}
