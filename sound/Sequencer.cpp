// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-
/*
  Rosegarden-4
  A sequencer and musical notation editor.

  This program is Copyright 2000-2002
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


#include <iostream>

#include "config.h"
#include "Sequencer.h"
#include "MappedStudio.h"

//#include "config.h"

// If we're configured for NO_SOUND then use the DummyDriver
// otherwise if we're configured for ALSA use that, otherwise
// default to the Arts driver.
//
#ifdef NO_SOUND
#include "DummyDriver.h"
#else
#ifdef HAVE_ALSA
#include "AlsaDriver.h"
#else
#include "ArtsDriver.h"
#endif
#endif

namespace Rosegarden
{

using std::cerr;
using std::cout;
using std::endl;

// Create a driver depending on what we have enabled.
// Initialisation of the driver is performed at construction
//
//
Sequencer::Sequencer(MappedStudio *studio,
                     const std::vector<std::string> &args)
    :m_soundDriver(0)
{
#ifdef NO_SOUND
    m_soundDriver = new DummyDriver(studio);
#else
#ifdef HAVE_ALSA
    m_soundDriver = new AlsaDriver(studio);
#else
    m_soundDriver = new ArtsDriver(studio);
#endif
#endif

    // Set the args if we have any
    //
    if (args.size())
        m_soundDriver->setArgs(args);

    m_soundDriver->initialiseMidi();
    m_soundDriver->initialiseAudio();
}


Sequencer::~Sequencer()
{
    if (m_soundDriver)
    {
        std::cout << "Sequencer::~Sequencer" << std::endl;
        delete m_soundDriver;
    }
}


}


