// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-
/*
  Rosegarden-4 v0.1
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

#include "config.h"

#ifdef HAVE_ALSA
#include "AlsaDriver.h"
#else
#include "ArtsDriver.h"
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
Sequencer::Sequencer()
{
#ifdef HAVE_ALSA
    m_soundDriver = new AlsaDriver();
#else
    m_soundDriver = new ArtsDriver();
#endif

    m_soundDriver->initialiseMidi();
    m_soundDriver->initialiseAudio();
}


Sequencer::~Sequencer()
{
    delete m_soundDriver;
}


}


