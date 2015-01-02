/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "OSCMessage.h"

#include <cstdlib>
#include <cstring>

namespace Rosegarden
{

OSCMessage::~OSCMessage()
{
    clearArgs();
}

void
OSCMessage::clearArgs()
{
    while (!m_args.empty()) {
        free(m_args[0].second);
        m_args.erase(m_args.begin());
    }
}

void
OSCMessage::addArg(char type, lo_arg *arg)
{
    lo_arg *newarg = 0;

    if (type == 's') {

        size_t sz = strlen((char *)arg) + 1;
        if (sz < sizeof(lo_arg))
            sz = sizeof(lo_arg);
        newarg = (lo_arg *)malloc(sz);
        strcpy((char *)newarg, (char *)arg);

    } else {

        newarg = (lo_arg *)malloc(sizeof(lo_arg));
        memcpy((char *)newarg, (char *)arg, sizeof(lo_arg));
    }

    m_args.push_back(OSCArg(type, newarg));
}

size_t
OSCMessage::getArgCount() const
{
    return m_args.size();
}

const lo_arg *
OSCMessage::getArg(size_t i, char &type) const
{
    type = m_args[i].first;
    return m_args[i].second;
}

}

