// -*- c-basic-offset: 4 -*-

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


// Accepts a file handle positioned somewhere in sample data (could
// be at the start) along with the necessary meta information for
// decoding (channels, bits per sample) and turns the sample data
// into peak data and generates a BWF format peak chunk file.  This
// file can exist by itself (in the case this is being generated 
// by a WAV) or be accomodated inside a BWF format file.
//
//

#include "RIFFPeakManager.h"

namespace Rosegarden
{

RIFFPeakManager::RIFFPeakManager(std::ifstream *in,
                                               std::ofstream *out,
                                               unsigned bitsPerSample,
                                               unsigned int channels,
                                               unsigned short updatePercentage):
    m_inFile(in),
    m_outFile(out),
    m_bitsPerSample(bitsPerSample),
    m_channels(channels),
    m_updatePercentage(updatePercentage)
{
}

RIFFPeakManager::~RIFFPeakManager()
{
}

// Generate the peak chunk into the out file stream.  This process will
// send updates of its progress by exception every m_updatePercentage%.
//
// Might change this to actually use signals and generate Pixmaps for
// us as well.
//
//

void
RIFFPeakManager::generate()
{
}



}

