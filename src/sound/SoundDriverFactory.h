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

#ifndef SOUND_DRIVER_FACTORY_H
#define SOUND_DRIVER_FACTORY_H

namespace Rosegarden {

class SoundDriver;

class SoundDriverFactory
{
public:
    static SoundDriver *createDriver(MappedStudio *studio);
};

}

#endif

