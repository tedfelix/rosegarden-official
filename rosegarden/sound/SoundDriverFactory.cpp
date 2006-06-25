// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-
/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2006
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

//#ifdef NO_SOUND
#include "DummyDriver.h"
//#else
#ifdef HAVE_ALSA
#include "AlsaDriver.h"
#else
#include "ArtsDriver.h"
#endif
//#endif

#include "SoundDriver.h"
#include "SoundDriverFactory.h"

namespace Rosegarden {

SoundDriver *
SoundDriverFactory::createDriver(MappedStudio *studio)
{
    SoundDriver *driver = 0;
    bool initialised = false;
#ifdef NO_SOUND
    driver = new DummyDriver(studio);
#else
#ifdef HAVE_ALSA
    driver = new AlsaDriver(studio);
#else
    driver = new ArtsDriver(studio);
#endif
#endif

    initialised = driver->initialise();
	
    if ( ! initialised ) {
	driver->shutdown();
	delete driver;
        
	// if the driver couldn't be initialised, then
        // fall to the DummyDriver as a last chance,
        // so GUI can still be used for notation.
        //
        driver = new DummyDriver(studio);
        driver->initialise();
    }
    return driver;
}


}


