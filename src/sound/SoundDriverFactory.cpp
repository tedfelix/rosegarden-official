/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.
    See the AUTHORS file for more details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "DummyDriver.h"

#ifdef HAVE_ALSA
#include "AlsaDriver.h"
#endif

#include "SoundDriverFactory.h"

namespace Rosegarden
{

SoundDriver *
SoundDriverFactory::createDriver(MappedStudio *studio)
{
    SoundDriver *driver = 0;
    bool initialised = false;

#ifdef HAVE_ALSA
    driver = new AlsaDriver(studio);
#else
    driver = new DummyDriver(studio);
#endif

    initialised = driver->initialise();

    if ( ! initialised ) {

        QString log = driver->getStatusLog();

        driver->shutdown();
        delete driver;

        // if the driver couldn't be initialised, then
        // fall to the DummyDriver as a last chance,
        // so GUI can still be used for notation.
        //
        driver = new DummyDriver(studio, log);
        driver->initialise();
    }
    return driver;
}


}


