// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
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


#ifdef HAVE_LIBMAD

#include "MP3AudioFile.h"

namespace Rosegarden
{

MP3AudioFile::MP3AudioFile(const unsigned int &id,
                           const std::string &name,
                           const std::string &fileName)
{
}


MP3AudioFile::MP3AudioFile(const std::string &fileName,
                           unsigned int channels,
                           unsigned int sampleRate,
                           unsigned int bytesPerSecond,
                           unsigned int bytesPerSample,
                           unsigned int bitsPerSample)
{
}


MP3AudioFile::~MP3AudioFile()
{
}

bool
MP3AudioFile::open()
{
}

bool
MP3AudioFile::write()
{
}

void
MP3AudioFile::close()
{
}

void
MP3AudioFile::parseHeader()
{
}

std::streampos
MP3AudioFile::getDataOffset()
{
}

}

#endif // HAVE_LIBMAD
