/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_CONTROLLABLE_DEVICE_H
#define RG_CONTROLLABLE_DEVICE_H

#include "ControlParameter.h"

namespace Rosegarden
{

typedef std::vector<ControlParameter> ControlList;

class Controllable
{
public:
    virtual ~Controllable() {}
    
    virtual const ControlList &getControlParameters() const = 0;
    virtual const ControlParameter *getControlParameter(int index) const = 0;
    virtual const ControlParameter *getControlParameter(const std::string &type,
                                                        MidiByte controllerNumber) const = 0;

protected:
    Controllable() { }
};

}

#endif
