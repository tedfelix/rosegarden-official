/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_CONTROLLEREVENTADAPTER_H
#define RG_CONTROLLEREVENTADAPTER_H

#include "ElementAdapter.h"

namespace Rosegarden {

class ControllerEventAdapter : public Rosegarden::ElementAdapter
{
public:
    ControllerEventAdapter(Event* e) : m_event(e) {}

    virtual bool getValue(long&);
    virtual void setValue(long);
    virtual timeT getTime();
    virtual timeT getDuration();

    virtual Event* getEvent() { return m_event; }

protected:

    //--------------- Data members ---------------------------------

    Event* m_event;
};

}

#endif /*CONTROLLEREVENTADAPTER_H_*/
